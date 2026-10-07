#ifndef GEMM_SIZE
#error "请在包含本文件前定义 GEMM_SIZE"
#endif

#ifndef GEMM_TOP
#error "请在包含本文件前定义 GEMM_TOP"
#endif

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace gemm_common;

constexpr int TEST_SIZE = GEMM_SIZE;
constexpr int TEST_K_WORDS = TEST_SIZE / LANES;
constexpr int TEST_A_WORDS = TEST_SIZE * TEST_K_WORDS;
constexpr int TEST_B_WORDS = TEST_K_WORDS * TEST_SIZE;
constexpr int TEST_C_WORDS = TEST_SIZE * TEST_SIZE;

static packed_t a_packed[TEST_A_WORDS];
static packed_t b_packed[TEST_B_WORDS];
static out_t c_output[TEST_C_WORDS];
static std::vector<data_t> a_reference(TEST_C_WORDS);
static std::vector<data_t> bt_reference(TEST_C_WORDS);

// 使用幅值较小的确定性整数模式，既覆盖正负数，也避免参考计算溢出。
static data_t make_a(int row, int k) {
    return (data_t)(((row * 3 + k * 5) % 9) - 4);
}

static data_t make_b_transposed(int column, int k) {
    return (data_t)(((column * 7 + k * 11) % 7) - 3);
}

static void prepare_inputs() {
    for (int row = 0; row < TEST_SIZE; ++row) {
        for (int k = 0; k < TEST_SIZE; ++k) {
            a_reference[row * TEST_SIZE + k] = make_a(row, k);
        }
    }

    for (int column = 0; column < TEST_SIZE; ++column) {
        for (int k = 0; k < TEST_SIZE; ++k) {
            bt_reference[column * TEST_SIZE + k] =
                make_b_transposed(column, k);
        }
    }

    // A_packed[row][K/8]：同一数据字包含连续 8 个 K 元素。
    for (int row = 0; row < TEST_SIZE; ++row) {
        for (int kt = 0; kt < TEST_K_WORDS; ++kt) {
            packed_t word = 0;
            for (int lane = 0; lane < LANES; ++lane) {
                word.range(lane * 16 + 15, lane * 16) =
                    a_reference[row * TEST_SIZE + kt * LANES + lane];
            }
            a_packed[row * TEST_K_WORDS + kt] = word;
        }
    }

    // B_packed[K/8][column]：连续 column 地址支持面板突发读取。
    for (int kt = 0; kt < TEST_K_WORDS; ++kt) {
        for (int column = 0; column < TEST_SIZE; ++column) {
            packed_t word = 0;
            for (int lane = 0; lane < LANES; ++lane) {
                word.range(lane * 16 + 15, lane * 16) =
                    bt_reference[column * TEST_SIZE + kt * LANES + lane];
            }
            b_packed[kt * TEST_SIZE + column] = word;
        }
    }

    std::fill(c_output, c_output + TEST_C_WORDS, (out_t)0);
}

static bool check_one(int row, int column) {
    int64_t expected = 0;
    for (int k = 0; k < TEST_SIZE; ++k) {
        expected +=
            (int64_t)a_reference[row * TEST_SIZE + k] *
            (int64_t)bt_reference[column * TEST_SIZE + k];
    }

    const int64_t actual = (int64_t)c_output[row * TEST_SIZE + column];
    if (actual != expected) {
        std::cout << "Mismatch (" << row << ", " << column << ")"
                  << " RTL=" << actual << " REF=" << expected << "\n";
        return false;
    }
    return true;
}

static bool check_outputs() {
    // 64/128/256 全矩阵校验；1024 采用覆盖边界和中心的代表点校验，
    // 以控制软件参考模型和 RTL co-sim 的后处理时间。
    if (TEST_SIZE <= 256) {
        for (int row = 0; row < TEST_SIZE; ++row) {
            for (int column = 0; column < TEST_SIZE; ++column) {
                if (!check_one(row, column)) {
                    return false;
                }
            }
        }
        return true;
    }

    const int positions[7] = {
        0, TEST_SIZE / 8, TEST_SIZE / 4, TEST_SIZE / 2,
        3 * TEST_SIZE / 4, 7 * TEST_SIZE / 8, TEST_SIZE - 1
    };
    for (int i = 0; i < 7; ++i) {
        for (int j = 0; j < 7; ++j) {
            if (!check_one(positions[i], positions[j])) {
                return false;
            }
        }
    }
    return true;
}

int main() {
    prepare_inputs();
    GEMM_TOP(a_packed, b_packed, c_output);
    const bool passed = check_outputs();
    std::cout << "GEMM " << TEST_SIZE << " RTL/C model check: "
              << (passed ? "PASS" : "FAIL") << "\n";
    return passed ? 0 : 1;
}
