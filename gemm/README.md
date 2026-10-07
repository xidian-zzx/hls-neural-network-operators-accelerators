# GEMM HLS 加速器

本目录包含四个固定矩阵尺寸的整数 GEMM 加速核。64、128、256 使用 adaptive-PE v7 架构；1024 保留 v6 的 32 PE DATAFLOW 架构。

| 目录 | 矩阵规模 | PE 数 | DSP | 综合延迟 | GOPS | RTL co-sim |
|---|---:|---:|---:|---:|---:|:---:|
| `64/` | 64³ | 8 | 64 | 19.720 µs | 26.587 | PASS |
| `128/` | 128³ | 16 | 128 | 75.016 µs | 55.912 | PASS |
| `256/` | 256³ | 32 | 256 | 296.200 µs | 113.283 | PASS |
| `1024/` | 1024³ | 32 | 256 | 17.305 ms | 124.096 | PASS |

公共模板位于 `common/gemm_core.hpp`，包括 B 分块预取、A 流式加载、输出驻留计算、C 连续写回及三级 DATAFLOW。每个尺寸目录均包含独立顶层函数、测试平台、Vitis HLS 2024.2 配置和运行脚本。

运行单个尺寸：

```powershell
cd gemm/64
./run_hls_2024_2.ps1
```

脚本依次执行 C-sim、C synthesis 和 RTL co-simulation，目标器件为 `xczu15eg-ffvb1156-2-i`，目标频率为 250 MHz。

完整验证记录见 [RTL_COSIM_REPORT.md](RTL_COSIM_REPORT.md)。
