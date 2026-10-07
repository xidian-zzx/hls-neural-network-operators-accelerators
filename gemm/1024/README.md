# GEMM 1024³

- 架构：v6 DATAFLOW
- PE / SIMD：32 PE × 8 lane
- DSP：256
- HLS 目标：Vitis HLS 2024.2，250 MHz，`xczu15eg-ffvb1156-2-i`
- 已知综合性能：17.305 ms，124.096 GOPS
- 验证：C-sim PASS；Verilog RTL co-sim PASS（4,342,318 cycles）
- 说明：C-sim 使用 `-O3` 加速软件模型，不改变综合 RTL

运行：

```powershell
./run_hls_2024_2.ps1
```
