#ifndef GEMM_128_H
#define GEMM_128_H

#include "../common/gemm_core.hpp"

// 128×128 固定尺寸 GEMM 顶层接口。
// A、B 为 8×int16 打包的 128 位数据字，C 为 int64 输出矩阵。
void gemm_128(
    const gemm_common::packed_t *A,
    const gemm_common::packed_t *B,
    gemm_common::out_t *C);

#endif
