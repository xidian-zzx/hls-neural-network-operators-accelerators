#ifndef GEMM_1024_H
#define GEMM_1024_H

#include "../common/gemm_core.hpp"

// 1024×1024 固定尺寸 GEMM 顶层接口。
// A、B 为 8×int16 打包的 128 位数据字，C 为 int64 输出矩阵。
void gemm_1024(
    const gemm_common::packed_t *A,
    const gemm_common::packed_t *B,
    gemm_common::out_t *C);

#endif
