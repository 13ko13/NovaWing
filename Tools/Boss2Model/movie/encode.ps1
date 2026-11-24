# Usage: powershell -File encode.ps1 -Frames <folder of f0000.png...>
# Makes Data/Movie/BossAppear2.mp4 and .ogv (Theora + silent vorbis; DxLib logs an error without an audio track).
param([Parameter(Mandatory = $true)][string]$Frames)
$ErrorActionPreference = "Stop"
$ff = (Get-Command ffmpeg -ErrorAction SilentlyContinue).Source
if (-not $ff) {
    $ff = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ffmpeg.exe | Select-Object -First 1 -ExpandProperty FullName
}
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$dst = Join-Path $root "Data\Movie"
$in = Join-Path $Frames "f%04d.png"
& $ff -y -framerate 30 -i $in -f lavfi -i "anullsrc=r=44100:cl=stereo" -shortest -c:v libx264 -pix_fmt yuv420p -crf 16 -preset slow (Join-Path $dst "BossAppear2.mp4")
& $ff -y -framerate 30 -i $in -f lavfi -i "anullsrc=r=44100:cl=stereo" -shortest -c:v libtheora -q:v 8 -pix_fmt yuv420p -c:a libvorbis -q:a 0 (Join-Path $dst "BossAppear2.ogv")
