# GEMM 128³

- 架构：v7 adaptive-PE
- PE / SIMD：16 PE × 8 lane
- DSP：128
- HLS 目标：Vitis HLS 2024.2，250 MHz，`xczu15eg-ffvb1156-2-i`
- 已知综合性能：75.016 µs，55.912 GOPS
- 验证：C-sim PASS；Verilog RTL co-sim PASS（40,840 cycles）

运行：

```powershell
./run_hls_2024_2.ps1
```
