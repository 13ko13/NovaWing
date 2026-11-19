# mv1check(モデルをDxLibで読んで、フレーム・大きさ・アニメーションの一覧と、描いた画像を出す確認用プログラム)をビルドする。
# 出力: このフォルダの mv1check.exe(gitには入れない)
# 使い方:
#   mv1check.exe <モデル> <出力テキスト>                                   … 情報だけ
#   mv1check.exe <モデル> <出力テキスト> - <PNG> "x,y,z" "x,y,z"            … カメラ位置・注視点から描く(縦の画角90°、1280x720、背景は曇り空の色)
#   mv1check.exe <モデル> <出力テキスト> - <PNG> "x,y,z" "x,y,z" <時間> <アニメ名> … そのアニメーションのその時間のポーズで描く
# 3番目の引数に .mv1 のパスを渡すと MV1SaveModelToMV1File で保存もする(「-」なら保存しない)。

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$root = Split-Path (Split-Path (Split-Path $here))
$dx = Join-Path $root "DxLib_h"
$vc = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "call `"$vc`" >nul 2>nul && cd /d `"$here`" && cl /nologo /EHsc /MT /utf-8 /DUNICODE /D_UNICODE /I`"$dx`" mv1check.cpp /link /LIBPATH:`"$dx`" shell32.lib /SUBSYSTEM:WINDOWS"
if (-not (Test-Path (Join-Path $here "mv1check.exe"))) { throw "ビルドに失敗しました" }
"built: $(Join-Path $here 'mv1check.exe')"
