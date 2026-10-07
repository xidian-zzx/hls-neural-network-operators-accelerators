#include "gemm_64.h"

namespace gemm_64_config {

// v7 adaptive-PE 配置：8 个输出 PE，每个 PE 内含 8 路 SIMD。
// 因此计算阵列并行执行 64 次 16 位乘法，对应 64 个 DSP。
constexpr int MATRIX_SIZE = 64;
constexpr int PE_NUM = 8;
constexpr int K_WORDS = MATRIX_SIZE / gemm_common::LANES;
constexpr int A_WORDS = MATRIX_SIZE * K_WORDS;
constexpr int B_WORDS = K_WORDS * MATRIX_SIZE;
constexpr int C_ELEMENTS = MATRIX_SIZE * MATRIX_SIZE;

} // namespace gemm_64_config

// =============================================================================
// AXI 顶层
//
// A、B、C 使用三个独立 m_axi bundle，使读 A、读 B、写 C 可以映射到
// 独立 AXI 通道；最大 burst 长度 256，最多允许 16 个未完成请求。
// s_axilite 仅承担启动、完成和返回控制。
// =============================================================================
void gemm_64(
    const gemm_common::packed_t *A,
    const gemm_common::packed_t *B,
    gemm_common::out_t *C) {

#pragma HLS INTERFACE m_axi port=A offset=slave bundle=A_BUS \
    depth=gemm_64_config::A_WORDS \
    max_read_burst_length=256 num_read_outstanding=16

#pragma HLS INTERFACE m_axi port=B offset=slave bundle=B_BUS \
    depth=gemm_64_config::B_WORDS \
    max_read_burst_length=256 num_read_outstanding=16

#pragma HLS INTERFACE m_axi port=C offset=slave bundle=C_BUS \
    depth=gemm_64_config::C_ELEMENTS \
    max_write_burst_length=256 num_write_outstanding=16

#pragma HLS INTERFACE s_axilite port=return bundle=control

    gemm_common::gemm_fixed<64, 64, 64, 8>(A, B, C);
}
