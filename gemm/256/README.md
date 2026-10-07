# GEMM 256³

- 架构：v7 adaptive-PE
- PE / SIMD：32 PE × 8 lane
- DSP：256
- HLS 目标：Vitis HLS 2024.2，250 MHz，`xczu15eg-ffvb1156-2-i`
- 已知综合性能：296.200 µs，113.283 GOPS
- 验证：C-sim PASS；Verilog RTL co-sim PASS（150,922 cycles）

运行：

```powershell
./run_hls_2024_2.ps1
```
