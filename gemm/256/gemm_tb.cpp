#include "gemm_256.h"

// 公共测试平台通过这两个宏绑定本目录的固定尺寸和顶层函数。
#define GEMM_SIZE 256
#define GEMM_TOP gemm_256
#include "../common/gemm_testbench.hpp"
