<#
.SYNOPSIS
  本地演示：拉 runner 编好的产物 -> 起 core -> 起 Web UI -> 开浏览器。
  本机不需要任何编译器。

  重复运行是安全的：每次会先把上一次开的窗口和进程全部收干净再起新的。

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Fetch          # 下载产物 + 跑全功能版
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow a         # 客户A：苹果外观检测
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Flow b         # 客户B：角度测量
  powershell -ExecutionPolicy Bypass -File tools\demo.ps1 -Stop           # 收摊
#>
param(
    # a    = 客户A的交付物（苹果外观检测项目）
    # b    = 客户B的交付物（角度测量项目）
    # full = 内部全功能版（回归测试用）
    [ValidateSet("a", "b", "full")] [string]$Flow = "full",
    [switch]$Fetch,      # 从 GitHub Actions 下载最新产物（第一次用 / 想更新时加）
    [switch]$NoBrowser,  # 不自动开浏览器
    [switch]$Stop        # 关掉 core、bridge 和它们的窗口
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent

# 我们开的窗口 PID 记在这里，收摊时按 PID 精确回收 —— 不会误伤你自己的窗口。
# （Win11 上窗口归 conhost 所有，powershell.exe 的 MainWindowTitle 是空的，
#   所以按标题找窗口这条路走不通，只能记 PID。）
$PIDFILE = Join-Path $env:TEMP "industrydemo-windows.txt"

function Kill-Demo {
    # 1) 上次开的宿主窗口。不收的话每跑一次留两个空窗口，一个约 130MB，
    #    跑十几次就能把内存吃光把整机拖死。
    if (Test-Path $PIDFILE) {
        Get-Content $PIDFILE | Where-Object { $_ -match '^\d+$' } | ForEach-Object {
            Stop-Process -Id ([int]$_) -Force -ErrorAction SilentlyContinue
        }
        Remove-Item $PIDFILE -Force -ErrorAction SilentlyContinue
    }
    # 2) 业务进程（窗口被杀时子进程通常已经跟着走，这里兜个底）
    Get-Process core -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-CimInstance Win32_Process -Filter "Name like '%python%'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -like "*bridge.py*" } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

function Start-InWindow([string]$title, [string]$cmd) {
    $p = Start-Process powershell -PassThru -ArgumentList "-NoExit", "-Command",
        "`$Host.UI.RawUI.WindowTitle='IndustryDemo - $title'; $cmd"
    Add-Content -Path $PIDFILE -Value $p.Id
}

if ($Stop) { Kill-Demo; "已停止 core / bridge 及其窗口"; return }

if ($Fetch) {
    $run = gh run list --workflow build-and-verify --status success --limit 1 --json databaseId --jq '.[0].databaseId'
    if (-not $run) { throw "没有找到成功的 CI run" }
    "从 CI run $run 下载 Windows 产物..."
    Remove-Item "$root\dist" -Recurse -Force -ErrorAction SilentlyContinue
    gh run download $run -n windows-binaries -D "$root\dist"
}

$bin = "$root\dist\build-$Flow\bin\Release"
if (-not (Test-Path "$bin\core.exe")) { throw "找不到 $bin\core.exe —— 先加 -Fetch 下载产物" }

# 之前已经有 bridge 在跑，就说明浏览器多半开着了：页面会自己重连，不必再开一个标签
$hadBridge = [bool](Get-NetTCPConnection -State Listen -LocalPort 8080 -ErrorAction SilentlyContinue)

Kill-Demo
Start-Sleep -Milliseconds 500

"启动 core ($Flow 版)  ->  新窗口"
Start-InWindow "core ($Flow)" "Set-Location '$bin'; .\core.exe"
Start-Sleep -Seconds 1

"启动 Web UI bridge   ->  新窗口"
Start-InWindow "bridge" "Set-Location '$root'; python ui\web\bridge.py"
Start-Sleep -Seconds 2

if ($NoBrowser) {
    ""
} elseif ($hadBridge) {
    ""
    "浏览器标签已开着，会自动重连 —— 没给你再开一个"
} else {
    Start-Process "http://127.0.0.1:8080"
    ""
}
"浏览器: http://127.0.0.1:8080"
"tkinter UI（可同时开）: python ui\ui_python.py"
"收摊: powershell -File tools\demo.ps1 -Stop"
