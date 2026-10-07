# Vitis HLS 2024.2 RTL co-simulation 报告

## 验证环境

- 工具：Vitis HLS / Vivado Simulator 2024.2
- RTL：Verilog
- 器件：`xczu15eg-ffvb1156-2-i`
- 目标时钟：250 MHz（4.000 ns）
- 综合估计时钟：四组均为 2.920 ns
- 验证日期：2026-10-07

## 结果

| Size | 架构 | PE | C-sim | RTL co-sim | 综合周期 | 综合延迟 | RTL 仿真周期 |
|---:|---|---:|:---:|:---:|---:|---:|---:|
| 64³ | v7 adaptive-PE | 8 | PASS | PASS | 4,930 | 19.720 µs | 11,914 |
| 128³ | v7 adaptive-PE | 16 | PASS | PASS | 18,754 | 75.016 µs | 40,840 |
| 256³ | v7 adaptive-PE | 32 | PASS | PASS | 74,050 | 296.200 µs | 150,922 |
| 1024³ | v6 DATAFLOW | 32 | PASS | PASS | 4,326,370 | 17.305 ms | 4,342,318 |

64、128、256 对输出矩阵执行全量逐元素校验。1024 完整执行全部 RTL 计算，并对覆盖首尾、中心和各分位位置的 49 个输出点执行 64 位参考点积校验。

综合阶段保留 `HLS 200-656` 潜在 DATAFLOW deadlock 预警；四组 XSIM 均完成 1/1 事务并通过 C 后检查，仿真中未触发 deadlock detector。
