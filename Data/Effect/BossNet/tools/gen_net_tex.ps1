# 網のレーザー用テクスチャを作る。実行: powershell -NoProfile -ExecutionPolicy Bypass -File gen_net_tex.ps1
# 出力先: ../Texture/
# 線のテクスチャは、横(U)=線の長手方向で同じ、縦(V)=線の太さ方向の断面。端(V=0,1)で alpha=0。
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot '..\Texture'
New-Item -ItemType Directory -Force $outDir | Out-Null
$outDir = (Resolve-Path $outDir).Path

function Save-Tex([int]$w, [int]$h, [scriptblock]$alphaFn, [string]$name) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $u = ($x + 0.5) / $w
            $v = ($y + 0.5) / $h
            $a = [double](& $alphaFn $u $v)
            if ($a -lt 0) { $a = 0 } elseif ($a -gt 1) { $a = 1 }
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int][math]::Round($a * 255), 255, 255, 255))
        }
    }
    $bmp.Save((Join-Path $outDir $name), [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "saved $name"
}

function Smooth([double]$e0, [double]$e1, [double]$x) {
    $t = ($x - $e0) / ($e1 - $e0)
    if ($t -lt 0) { $t = 0 } elseif ($t -gt 1) { $t = 1 }
    return $t * $t * (3 - 2 * $t)
}

# 縁(t=1)で必ず0になる断面。t = 中心からの距離(0〜1)
# Flat: ほぼ一様で縁だけ柔らかい(暗い層・本体の層用)
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; 1 - (Smooth 0.72 1.0 $t) } 'NB_Flat.png'
# Soft: なだらかな光(外側のにじみ用)
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; $f = 1 - $t; [math]::Pow($f, 1.6) } 'NB_Soft.png'
# Core: 細く鋭い芯
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; [math]::Exp(-5.0 * $t * $t) * (1 - (Smooth 0.6 1.0 $t)) } 'NB_Core.png'

# Head: 先端の光(丸い芯 + 十字の光条)。端で0
Save-Tex 128 128 {
    param($u, $v)
    $x = ($u - 0.5) * 2; $y = ($v - 0.5) * 2
    $r = [math]::Sqrt($x * $x + $y * $y)
    $edge = 1 - (Smooth 0.7 1.0 $r)
    $glow = [math]::Exp(-9 * $r * $r)
    $ax = [math]::Abs($x); $ay = [math]::Abs($y)
    $star = 0.55 * ([math]::Exp(-$ax * 3) * [math]::Exp(-$ay * 28) + [math]::Exp(-$ay * 3) * [math]::Exp(-$ax * 28))
    ($glow + $star) * $edge
} 'NB_Head.png'
