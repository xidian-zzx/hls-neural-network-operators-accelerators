$ErrorActionPreference = "Stop"
$vitisRun = "E:\software\xilinx2024\Vitis\2024.2\bin\vitis-run.bat"

if (-not (Test-Path -LiteralPath $vitisRun)) {
    throw "未找到 Vitis HLS 2024.2：$vitisRun"
}

foreach ($size in 64, 128, 256, 1024) {
    Write-Host "=== GEMM $size：C-sim + 综合 + RTL co-sim ==="
    & (Join-Path $PSScriptRoot "$size\run_hls_2024_2.ps1")
    if ($LASTEXITCODE -ne 0) {
        throw "GEMM $size 流程失败，退出码：$LASTEXITCODE"
    }
}
