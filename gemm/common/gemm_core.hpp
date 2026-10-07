#ifndef GEMM_COMMON_CORE_HPP
#define GEMM_COMMON_CORE_HPP

#include <ap_int.h>
#include <hls_stream.h>

// =============================================================================
// 固定尺寸 GEMM 的公共 HLS 模板核
//
// 数学定义：C[M][N] = A[M][K] × B[K][N]
// 数据格式：
//   1. A、B 元素均为 16 位有符号整数；
//   2. 每个 128 位 AXI 数据字打包 8 个元素，即 8 路 SIMD；
//   3. C 使用 64 位有符号整数，保留完整点积结果。
//
// 计算结构：
//   B 面板预取 → A 流式读取 → PE 阵列计算 → C 连续写回
//                      \________ DATAFLOW ________/
//
// PE 数由各尺寸顶层在编译期指定。小矩阵减少 PE 可显著降低 DSP/BRAM，
// 1024 尺寸保持 v6 的 32 PE 配置。
// =============================================================================
namespace gemm_common {

constexpr int LANES = 8;
constexpr int WORD_BITS = 128;

using data_t = ap_int<16>;
using packed_t = ap_uint<WORD_BITS>;
using acc_t = ap_int<56>;
using out_t = ap_int<64>;

// 从一个 128 位打包字中提取指定的 16 位有符号元素。
// INLINE 可使该操作直接并入乘加数据通路，不形成额外层级。
static data_t unpack_lane(packed_t word, int lane) {
#pragma HLS INLINE
    return (data_t)word.range(lane * 16 + 15, lane * 16);
}

// -----------------------------------------------------------------------------
// 阶段 0：预取一个 B 列面板。
//
// 外存布局为 B_packed[K/8][N]。每次读取 PE 个连续 AXI 数据字，
// 对应 PE 个输出列；第一维完全分区后，每个 PE 拥有独立片上存储银行，
// 计算阶段能够在同一周期读取全部 PE 的 B 数据。
// -----------------------------------------------------------------------------
template <int N, int K_WORDS, int PE>
static void preload_b_panel(
    const packed_t *B,
    packed_t b_local[PE][K_WORDS],
    int column_base) {
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=b_local complete dim=1

    for (int kt = 0; kt < K_WORDS; ++kt) {
        for (int pe = 0; pe < PE; ++pe) {
#pragma HLS PIPELINE II=1
            b_local[pe][kt] = B[kt * N + column_base + pe];
        }
    }
}

// -----------------------------------------------------------------------------
// DATAFLOW 阶段 1：顺序读取整个 A 矩阵。
//
// A 已按 A_packed[row][K/8] 连续存放。扁平循环形成连续地址，便于 HLS
// 推断长 AXI burst；FIFO 将外存读取节拍与计算节拍解耦。
// -----------------------------------------------------------------------------
template <int M, int K_WORDS>
static void load_a_rows(
    const packed_t *A,
    hls::stream<packed_t> &a_fifo) {
#pragma HLS INLINE off

    constexpr int TOTAL_A_WORDS = M * K_WORDS;
    for (int index = 0; index < TOTAL_A_WORDS; ++index) {
#pragma HLS PIPELINE II=1
        a_fifo.write(A[index]);
    }
}

// -----------------------------------------------------------------------------
// DATAFLOW 阶段 2：输出驻留 PE 阵列。
//
// 每个时钟周期读入一个 A 打包字：
//   PE 个输出列 × 8 个 SIMD lane = PE×8 次 16 位乘法。
// acc[pe] 在完整 K 维归约期间驻留寄存器；K 归约完成后，所有 PE 并行
// 写入独立结果 FIFO。循环完全展开保证 PE 和 lane 维的硬件并行度。
// -----------------------------------------------------------------------------
template <int M, int K_WORDS, int PE>
static void compute_rows(
    hls::stream<packed_t> &a_fifo,
    const packed_t b_local[PE][K_WORDS],
    hls::stream<out_t> c_fifo[PE]) {
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=b_local complete dim=1
#pragma HLS ARRAY_PARTITION variable=c_fifo complete dim=1

    for (int row = 0; row < M; ++row) {
        acc_t accumulators[PE];
#pragma HLS ARRAY_PARTITION variable=accumulators complete dim=1

        for (int pe = 0; pe < PE; ++pe) {
#pragma HLS UNROLL
            accumulators[pe] = 0;
        }

        for (int kt = 0; kt < K_WORDS; ++kt) {
#pragma HLS PIPELINE II=1
            const packed_t a_word = a_fifo.read();

            for (int pe = 0; pe < PE; ++pe) {
#pragma HLS UNROLL
                const packed_t b_word = b_local[pe][kt];
                acc_t lane_sum = 0;

                for (int lane = 0; lane < LANES; ++lane) {
#pragma HLS UNROLL
                    const data_t a_value = unpack_lane(a_word, lane);
                    const data_t b_value = unpack_lane(b_word, lane);
                    lane_sum += (ap_int<32>)a_value * b_value;
                }

                accumulators[pe] += lane_sum;
            }
        }

        for (int pe = 0; pe < PE; ++pe) {
#pragma HLS UNROLL
            c_fifo[pe].write((out_t)accumulators[pe]);
        }
    }
}

// -----------------------------------------------------------------------------
// DATAFLOW 阶段 3：写回一个 C 列面板。
//
// 先并行收集所有 PE 的结果，再顺序写入同一行的连续列地址，形成 PE 拍
// AXI write burst。矩阵尺寸均为 PE 的整数倍，无需边界条件分支。
// -----------------------------------------------------------------------------
template <int M, int N, int PE>
static void store_c_rows(
    hls::stream<out_t> c_fifo[PE],
    out_t *C,
    int column_base) {
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=c_fifo complete dim=1

    for (int row = 0; row < M; ++row) {
        out_t row_result[PE];
#pragma HLS ARRAY_PARTITION variable=row_result complete dim=1

        for (int pe = 0; pe < PE; ++pe) {
#pragma HLS UNROLL
            row_result[pe] = c_fifo[pe].read();
        }

        for (int pe = 0; pe < PE; ++pe) {
#pragma HLS PIPELINE II=1
            C[row * N + column_base + pe] = row_result[pe];
        }
    }
}

// -----------------------------------------------------------------------------
// 处理一个 B/C 列面板。
//
// A 读取、PE 计算和 C 写回通过 DATAFLOW 并发运行。A_FIFO_DEPTH 取两行
// A 数据，吸收 AXI 突发与计算之间的短期抖动；每个 C FIFO 可缓存两行。
// -----------------------------------------------------------------------------
template <int M, int N, int K_WORDS, int PE, int A_FIFO_DEPTH>
static void process_panel(
    const packed_t *A,
    const packed_t b_local[PE][K_WORDS],
    out_t *C,
    int column_base) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW
#pragma HLS ARRAY_PARTITION variable=b_local complete dim=1

    hls::stream<packed_t> a_fifo("a_fifo");
#pragma HLS STREAM variable=a_fifo depth=A_FIFO_DEPTH
#pragma HLS BIND_STORAGE variable=a_fifo type=fifo impl=bram

    hls::stream<out_t> c_fifo[PE];
#pragma HLS ARRAY_PARTITION variable=c_fifo complete dim=1
#pragma HLS STREAM variable=c_fifo depth=2

    load_a_rows<M, K_WORDS>(A, a_fifo);
    compute_rows<M, K_WORDS, PE>(a_fifo, b_local, c_fifo);
    store_c_rows<M, N, PE>(c_fifo, C, column_base);
}

// -----------------------------------------------------------------------------
// 固定尺寸 GEMM 总控。
//
// N 维按 PE 列切分为多个面板。每个面板只预取一次 B，随后让 A load、
// compute、C store 并行处理全部 M 行。模板参数在综合期完全确定，循环边界
// 和片上数组规模均可静态展开。
// -----------------------------------------------------------------------------
template <int M, int N, int K, int PE>
static void gemm_fixed(
    const packed_t *A,
    const packed_t *B,
    out_t *C) {
#pragma HLS INLINE off

    static_assert(K % LANES == 0, "K must be divisible by 8");
    static_assert(N % PE == 0, "N must be divisible by PE");

    constexpr int K_WORDS = K / LANES;
    constexpr int N_PANELS = N / PE;
    constexpr int A_FIFO_DEPTH = 2 * K_WORDS;

    packed_t b_local[PE][K_WORDS];
#pragma HLS ARRAY_PARTITION variable=b_local complete dim=1
#pragma HLS BIND_STORAGE variable=b_local type=ram_2p impl=bram

    for (int panel = 0; panel < N_PANELS; ++panel) {
        const int column_base = panel * PE;
        preload_b_panel<N, K_WORDS, PE>(B, b_local, column_base);
        process_panel<M, N, K_WORDS, PE, A_FIFO_DEPTH>(
            A, b_local, C, column_base);
    }
}

} // namespace gemm_common

#endif
