# GEMM 64³

- 架构：v7 adaptive-PE
- PE / SIMD：8 PE × 8 lane
- DSP：64
- HLS 目标：Vitis HLS 2024.2，250 MHz，`xczu15eg-ffvb1156-2-i`
- 已知综合性能：19.720 µs，26.587 GOPS
- 验证：C-sim PASS；Verilog RTL co-sim PASS（11,914 cycles）

运行：

```powershell
./run_hls_2024_2.ps1
```
