# 2面のボスのモデルを最初から作り直して、ゲームの Data/Model/Boss2.mv1 に置くまでを通しで行う。
# 前提: Blender(MCPのサーバーが起動済み)で build/bake/export の3つのスクリプトを実行し、
#       Tools/Boss2Model/export/Boss2.fbx ができていること。
# このスクリプトは、その FBX を DxLib のモデルビューアで .mv1 に変換して Data/Model/Boss2.mv1 に置く(画像は Data/Model/Boss2.fbm/)。
# 変換のときだけ、モデルビューアの「法線を作り直す」(NormalRemake)を切る(角ばった面を丸くしないため)。

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot)
$vdir = "C:\Users\SakamotoKou\Desktop\DxLib_VC3_24f\DxLib_VC\Tool\DxLibModelViewer"
$ini = Join-Path $vdir "Setting.ini"
$fbx = Join-Path $PSScriptRoot "export\Boss2.fbx"
$mv1 = [IO.Path]::ChangeExtension($fbx, ".mv1")
$dst = Join-Path $root "Data\Model\Boss2.mv1"

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
"converted: $dst"
