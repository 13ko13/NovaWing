# 透過PNG を単色背景に重ねて並べる / キャプチャを並べる道具（確認用）
#  -Mode tex  : -Images a.png,b.png を 4 色の背景（明るい空・海・夜・曇り）に重ねたグリッド
#  -Mode grid : -Rows "ラベル|a.png|b.png;ラベル|c.png|d.png" -ColLabels "x,y" で並べる（-Gray で白黒版も右に）
param([string]$Mode="tex",[string[]]$Images,[string]$Rows,[string]$ColLabels="",[string]$Out="out.png",[int]$Cell=256,[switch]$Gray)
Add-Type -AssemblyName System.Drawing
if($Images -and $Images.Count -eq 1){ $Images = $Images[0] -split "," }
$bgs = @(@(215,222,232),@(40,110,160),@(6,14,24),@(150,155,162))
function GrayOf([Drawing.Bitmap]$b){
  $g = New-Object Drawing.Bitmap $b.Width,$b.Height
  for($y=0;$y -lt $b.Height;$y++){ for($x=0;$x -lt $b.Width;$x++){ $c=$b.GetPixel($x,$y); $l=[int](0.299*$c.R+0.587*$c.G+0.114*$c.B); $g.SetPixel($x,$y,[Drawing.Color]::FromArgb(255,$l,$l,$l)) } }
  return $g
}
$font = New-Object Drawing.Font "Meiryo",11
if($Mode -eq "tex"){
  $canvas = New-Object Drawing.Bitmap ($Cell*$bgs.Count),($Cell*$Images.Count)
  $g=[Drawing.Graphics]::FromImage($canvas)
  for($r=0;$r -lt $Images.Count;$r++){ $img=[Drawing.Bitmap]::FromFile($Images[$r])
    for($c=0;$c -lt $bgs.Count;$c++){ $bg=$bgs[$c]; $g.FillRectangle((New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb($bg[0],$bg[1],$bg[2]))),$c*$Cell,$r*$Cell,$Cell,$Cell); $g.DrawImage($img,$c*$Cell,$r*$Cell,$Cell,$Cell) }
    $img.Dispose() }
  $canvas.Save($Out); $g.Dispose(); $canvas.Dispose(); return
}
$rowList = @($Rows -split ";" | ? { $_ })
$cols = ($rowList[0] -split "\|").Count - 1
$first = [Drawing.Bitmap]::FromFile((($rowList[0] -split "\|")[1])); $cw=$first.Width; $ch=$first.Height; $first.Dispose()
$mult = if($Gray){2}else{1}
$lw=150; $th=24
$canvas = New-Object Drawing.Bitmap ($lw + $cw*$cols*$mult),($th + $ch*$rowList.Count)
$g=[Drawing.Graphics]::FromImage($canvas); $g.Clear([Drawing.Color]::FromArgb(30,30,30))
$cl = $ColLabels -split ","
for($c=0;$c -lt $cols;$c++){ if($c -lt $cl.Count){ $g.DrawString($cl[$c],$font,[Drawing.Brushes]::White,$lw+$c*$cw+4,3) } }
if($Gray){ $g.DrawString("白黒",$font,[Drawing.Brushes]::White,$lw+$cols*$cw+4,3) }
for($r=0;$r -lt $rowList.Count;$r++){ $parts=$rowList[$r] -split "\|"
  $g.DrawString($parts[0],$font,[Drawing.Brushes]::White,4,$th+$r*$ch+4)
  for($c=0;$c -lt $cols;$c++){ $img=[Drawing.Bitmap]::FromFile($parts[$c+1]); $g.DrawImage($img,$lw+$c*$cw,$th+$r*$ch,$cw,$ch)
    if($Gray){ $gi=GrayOf $img; $g.DrawImage($gi,$lw+($cols+$c)*$cw,$th+$r*$ch,$cw,$ch); $gi.Dispose() }
    $img.Dispose() } }
$canvas.Save($Out); $g.Dispose(); $canvas.Dispose()
