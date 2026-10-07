#include "gemm_128.h"

// 公共测试平台通过这两个宏绑定本目录的固定尺寸和顶层函数。
#define GEMM_SIZE 128
#define GEMM_TOP gemm_128
#include "../common/gemm_testbench.hpp"
