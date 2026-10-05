# Preview-only dummy jet (.efkmodel v5) to judge size/placement. Nose = -Z, tail = +Z, wings along X.
param([string]$Out)

$verts = New-Object System.Collections.Generic.List[object]
$faces = New-Object System.Collections.Generic.List[int]

function Add-Quad($p0, $p1, $p2, $p3) {
  # normal from (p1-p0) x (p3-p0)
  $ux = $p1[0]-$p0[0]; $uy = $p1[1]-$p0[1]; $uz = $p1[2]-$p0[2]
  $vx = $p3[0]-$p0[0]; $vy = $p3[1]-$p0[1]; $vz = $p3[2]-$p0[2]
  $nx = $uy*$vz-$uz*$vy; $ny = $uz*$vx-$ux*$vz; $nz = $ux*$vy-$uy*$vx
  $l = [Math]::Sqrt($nx*$nx+$ny*$ny+$nz*$nz); if ($l -eq 0) { $l = 1 }
  $n = @(($nx/$l), ($ny/$l), ($nz/$l))
  $base = $verts.Count
  foreach ($p in @($p0, $p1, $p2, $p3)) { $verts.Add(@($p[0], $p[1], $p[2], $n[0], $n[1], $n[2])) }
  $faces.AddRange([int[]]@($base, ($base+1), ($base+2), $base, ($base+2), ($base+3)))
}

function Add-Box($x0, $x1, $y0, $y1, $z0, $z1) {
  Add-Quad @($x0,$y1,$z0) @($x1,$y1,$z0) @($x1,$y1,$z1) @($x0,$y1,$z1)   # top
  Add-Quad @($x0,$y0,$z1) @($x1,$y0,$z1) @($x1,$y0,$z0) @($x0,$y0,$z0)   # bottom
  Add-Quad @($x0,$y0,$z0) @($x1,$y0,$z0) @($x1,$y1,$z0) @($x0,$y1,$z0)   # front
  Add-Quad @($x1,$y0,$z1) @($x0,$y0,$z1) @($x0,$y1,$z1) @($x1,$y1,$z1)   # back
  Add-Quad @($x0,$y0,$z1) @($x0,$y0,$z0) @($x0,$y1,$z0) @($x0,$y1,$z1)   # left
  Add-Quad @($x1,$y0,$z0) @($x1,$y0,$z1) @($x1,$y1,$z1) @($x1,$y1,$z0)   # right
}

Add-Box -0.25 0.25 -0.15 0.2 -2.0 1.2     # fuselage
Add-Box -1.6 1.6 -0.03 0.03 -0.3 0.6      # wings (tips at x = +-1.6)
Add-Box -0.03 0.03 0.2 0.75 0.5 1.2       # tail fin

$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter $ms
$bw.Write([int]5); $bw.Write([single]1.0); $bw.Write([int]1); $bw.Write([int]1)
$bw.Write([int]$verts.Count)
foreach ($v in $verts) {
  $bw.Write([single]$v[0]); $bw.Write([single]$v[1]); $bw.Write([single]$v[2])
  $bw.Write([single]$v[3]); $bw.Write([single]$v[4]); $bw.Write([single]$v[5])
  $bw.Write([single]0); $bw.Write([single]1); $bw.Write([single]0)
  $bw.Write([single]1); $bw.Write([single]0); $bw.Write([single]0)
  $bw.Write([single]0.5); $bw.Write([single]0.5)
  $bw.Write([byte]255); $bw.Write([byte]255); $bw.Write([byte]255); $bw.Write([byte]255)
}
$bw.Write([int]($faces.Count / 3))
foreach ($f in $faces) { $bw.Write([int]$f) }
$bw.Flush()
[System.IO.File]::WriteAllBytes($Out, $ms.ToArray())
"wrote $Out ($($verts.Count) verts)"
