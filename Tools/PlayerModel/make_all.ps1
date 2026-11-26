# 自機の FBX を mv1 に変換して、ゲームの Data/Model/Player.mv1 と Data/Model/Player.fbm/ に置く。
# 前提: Blender で build_player.py → bake_player.py → export_player.py を実行し、
#       Tools/PlayerModel/export/Player.fbx と export/Player.fbm/ ができていること。
# 画像の名前は前の自機(StarSparrow_*.png)と同じなので、ゲームのコードは変えなくてよい。
# 変換のときだけ、モデルビューアの「法線を作り直す」(NormalRemake)を切る(2面のボスの make_all.ps1 と同じ)。

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot)
$vdir = @("C:\Users\SakamotoKou\Desktop\DxLib_VC3_24f\DxLib_VC\Tool\DxLibModelViewer",
          "C:\DxLib_VC\Tool\DxLibModelViewer") | Where-Object { Test-Path $_ } | Select-Object -First 1
$ini = Join-Path $vdir "Setting.ini"
$fbx = Join-Path $PSScriptRoot "export\Player.fbx"
$fbm = Join-Path $PSScriptRoot "export\Player.fbm"
$mv1 = [IO.Path]::ChangeExtension($fbx, ".mv1")
$dst = Join-Path $root "Data\Model\Player.mv1"
$dstFbm = Join-Path $root "Data\Model\Player.fbm"
$images = "StarSparrow_White.png", "StarSparrow_Normal.png", "StarSparrow_MetallicSmoothness.png", "StarSparrow_Emission.png"

$orig = [IO.File]::ReadAllBytes($ini)
try {
    $txt = [Text.Encoding]::Default.GetString($orig) -replace "NormalRemake=1", "NormalRemake=0"
    [IO.File]::WriteAllBytes($ini, [Text.Encoding]::Default.GetBytes($txt))
    if (Test-Path $mv1) { Remove-Item $mv1 }
    $p = Start-Process -FilePath (Join-Path $vdir "DxLibModelViewer_64bit.exe") -ArgumentList "`"$fbx`"" -WorkingDirectory $vdir -PassThru
    $ok = $false
    for ($i = 0; $i -lt 60; $i++) {
        Start-Sleep -Milliseconds 500
        if (Test-Path $mv1) { Start-Sleep -Seconds 2; $ok = $true; break }
    }
    if (-not $p.HasExited) { $p.Kill(); $p.WaitForExit() }
}
finally {
    Start-Sleep -Milliseconds 500
    [IO.File]::WriteAllBytes($ini, $orig)
}
if (-not $ok) { throw "mv1 への変換に失敗しました" }
Copy-Item $mv1 $dst -Force
New-Item -ItemType Directory -Force $dstFbm | Out-Null
foreach ($n in $images) { Copy-Item (Join-Path $fbm $n) (Join-Path $dstFbm $n) -Force }
"converted: $dst"
