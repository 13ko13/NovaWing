# ロケット弾のモデルを作り、mv1 に変換して、ゲームの Data/Model/Rocket/ に置く。
# 1. Blender(バックグラウンド)で build_rocket.py → export/Rocket.fbx と画像4枚
# 2. モデルビューアで FBX → mv1(自機の make_all.ps1 と同じく「法線を作り直す」(NormalRemake)を切る)
# 3. Rocket.mv1 と画像4枚を Data/Model/Rocket/ にコピー

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot)
$blender = @("C:\Program Files\Blender Foundation\Blender 5.2\blender.exe",
             "C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
$vdir = @("C:\Users\SakamotoKou\Desktop\DxLib_VC3_24f\DxLib_VC\Tool\DxLibModelViewer",
          "C:\DxLib_VC\Tool\DxLibModelViewer") | Where-Object { Test-Path $_ } | Select-Object -First 1
$ini = Join-Path $vdir "Setting.ini"
$fbx = Join-Path $PSScriptRoot "export\Rocket.fbx"
$mv1 = [IO.Path]::ChangeExtension($fbx, ".mv1")
$dstDir = Join-Path $root "Data\Model\Rocket"
$images = "Rocket_Albedo.png", "Rocket_Normal.png", "Rocket_Metallic.png", "Rocket_Emission.png"

if (Test-Path $fbx) { Remove-Item $fbx }
Start-Process -FilePath $blender -ArgumentList "-b --factory-startup --python `"$(Join-Path $PSScriptRoot 'build_rocket.py')`"" -Wait -WindowStyle Hidden
if (-not (Test-Path $fbx)) { throw "FBX の書き出しに失敗しました" }

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
New-Item -ItemType Directory -Force $dstDir | Out-Null
Copy-Item $mv1 (Join-Path $dstDir "Rocket.mv1") -Force
foreach ($n in $images) { Copy-Item (Join-Path $PSScriptRoot "export\$n") (Join-Path $dstDir $n) -Force }
"converted: $(Join-Path $dstDir 'Rocket.mv1')"
