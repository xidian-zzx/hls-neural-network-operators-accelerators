$ErrorActionPreference = "Stop"
$vitisRun = "E:\software\xilinx2024\Vitis\2024.2\bin\vitis-run.bat"
$vpp = "E:\software\xilinx2024\Vitis\2024.2\bin\v++.bat"

if (-not (Test-Path -LiteralPath $vitisRun) -or
    -not (Test-Path -LiteralPath $vpp)) {
    throw "未找到 Vitis HLS 2024.2 命令行工具"
}

Push-Location $PSScriptRoot
try {
    & $vitisRun --mode hls --csim --config hls_config.cfg --work_dir hls_work_release
    if ($LASTEXITCODE -ne 0) { throw "C-sim 失败：$LASTEXITCODE" }

    & $vpp -c --mode hls --config hls_config.cfg --work_dir hls_work_release
    if ($LASTEXITCODE -ne 0) { throw "C synthesis 失败：$LASTEXITCODE" }

    & $vitisRun --mode hls --cosim --config hls_config.cfg --work_dir hls_work_release
    if ($LASTEXITCODE -ne 0) { throw "RTL co-sim 失败：$LASTEXITCODE" }
} finally {
    Pop-Location
}
