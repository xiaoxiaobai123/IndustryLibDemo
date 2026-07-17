<#
.SYNOPSIS
  演示用：证明 core.exe 依赖我们自己的动态库，且关掉的流程不在二进制里。
  读 PE 文件里的 dll 名和字符串，不需要 dumpbin。
#>
$root = Split-Path $PSScriptRoot -Parent

"{0,-10} {1,-24} {2,-14} {3}" -f "交付物", "运行时依赖的库", "含客户A项目", "含客户B项目"
"-" * 68

foreach ($v in @(@{tag = "客户A版"; dir = "build-a" },
                 @{tag = "客户B版"; dir = "build-b" },
                 @{tag = "全功能版"; dir = "build-full" })) {
    $exe = "$root\dist\$($v.dir)\bin\Release\core.exe"
    if (-not (Test-Path $exe)) { "找不到 $exe —— 先跑 tools\demo.ps1 -Fetch"; continue }

    $text = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
    $ours = @("base.dll", "algo.dll") | Where-Object { $text -match [regex]::Escape($_) }

    "{0,-10} {1,-24} {2,-14} {3}" -f $v.tag, ($ours -join ", "),
        ($text -match "customer_a"), ($text -match "customer_b")
}
""
"库本身的依赖方向（反向依赖不存在才叫分层）："
foreach ($lib in @("base.dll", "algo.dll")) {
    $p = "$root\dist\build-full\bin\Release\$lib"
    if (-not (Test-Path $p)) { continue }
    $t = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($p))
    $deps = @("base.dll", "algo.dll") | Where-Object { $_ -ne $lib -and $t -match [regex]::Escape($_) }
    "  {0,-10} -> {1}" -f $lib, $(if ($deps) { $deps -join ", " } else { "（不依赖我们任何库）" })
}
