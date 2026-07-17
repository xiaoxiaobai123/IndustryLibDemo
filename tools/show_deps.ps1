<#
.SYNOPSIS
  演示用：证明 core.exe 依赖我们自己的动态库，且关掉的流程不在二进制里。
  读 PE 文件里的 dll 名和字符串，不需要 dumpbin。
#>
$root = Split-Path $PSScriptRoot -Parent

foreach ($v in @(@{tag = "A版      "; dir = "build-a" }, @{tag = "全功能版 "; dir = "build-full" })) {
    $exe = "$root\dist\$($v.dir)\bin\Release\core.exe"
    if (-not (Test-Path $exe)) { "找不到 $exe —— 先跑 tools\demo.ps1 -Fetch"; continue }

    $bytes = [IO.File]::ReadAllBytes($exe)
    $text = [Text.Encoding]::ASCII.GetString($bytes)

    $ours = @("base.dll", "algo.dll") | Where-Object { $text -match [regex]::Escape($_) }
    $hasB = $text -match "customer_b"

    "{0} 依赖: {1,-22} 含 customer_b: {2}" -f $v.tag, ($ours -join ", "), $hasB
}
