<#
.SYNOPSIS
  本地演示：拉 runner 编好的产物 -> 起 core -> 起 Web UI -> 开浏览器。
  本机不需要任何编译器。

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Fetch          # 全功能版(3流程)
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow a         # A版(2流程)
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Stop           # 收摊
#>
param(
    [ValidateSet("a", "full")] [string]$Flow = "full",
    [switch]$Fetch,   # 从 GitHub Actions 下载最新产物（第一次用 / 想更新时加）
    [switch]$Stop     # 关掉 core 和 bridge
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

function Kill-Demo {
    Get-Process core -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-CimInstance Win32_Process -Filter "Name like '%python%'" |
        Where-Object { $_.CommandLine -like "*bridge.py*" } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

if ($Stop) { Kill-Demo; "已停止 core 和 bridge"; return }

if ($Fetch) {
    $run = gh run list --workflow build-and-verify --status success --limit 1 --json databaseId --jq '.[0].databaseId'
    if (-not $run) { throw "没有找到成功的 CI run" }
    "从 CI run $run 下载 Windows 产物..."
    Remove-Item "$root\dist" -Recurse -Force -ErrorAction SilentlyContinue
    gh run download $run -n windows-binaries -D "$root\dist"
}

$bin = "$root\dist\build-$Flow\bin\Release"
if (-not (Test-Path "$bin\core.exe")) { throw "找不到 $bin\core.exe —— 先加 -Fetch 下载产物" }

Kill-Demo
Start-Sleep -Milliseconds 300

"启动 core ($Flow 版)  ->  新窗口"
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Set-Location '$bin'; .\core.exe"
Start-Sleep -Seconds 1

"启动 Web UI bridge   ->  新窗口"
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Set-Location '$root'; python ui\web\bridge.py"
Start-Sleep -Seconds 2

Start-Process "http://127.0.0.1:8080"
""
"浏览器: http://127.0.0.1:8080"
"tkinter UI（可同时开）: python ui\ui_python.py"
"收摊: powershell -File tools\demo.ps1 -Stop"
