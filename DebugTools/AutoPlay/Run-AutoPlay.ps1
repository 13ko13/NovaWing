<#
.SYNOPSIS
    AIテストプレイを起動する(Debugビルドの NovaWing.exe を --autoplay 付きで起動し、終わったらレポートを開く)

.PARAMETER Runs
    何回プレイするか
.PARAMETER Speed
    速度。1, 2, 4, 8 ... の倍率、または max(最大速度)
.PARAMETER Mode
    visible(ゲーム画面を表示) / hidden(画面を出さない)
.PARAMETER Skill
    AIの腕前。beginner / normal / expert
.PARAMETER Seed
    乱数の種(同じ値なら同じ流れを再現しやすい)。省略すると毎回変わる
.PARAMETER MaxMinutes
    1回のプレイの上限(ゲーム内の分)。超えたら時間切れとしてやり直す
.PARAMETER NoReport
    終わった後にレポートを開かない

.EXAMPLE
    .\DebugTools\AutoPlay\Run-AutoPlay.ps1 -Runs 20 -Speed max -Mode hidden -Skill beginner
#>
param(
    [int]$Runs = 10,
    [string]$Speed = "4",
    [ValidateSet("visible", "hidden")][string]$Mode = "visible",
    [ValidateSet("beginner", "normal", "expert")][string]$Skill = "normal",
    [int]$Seed = -1,
    [int]$MaxMinutes = 15,
    [switch]$NoReport
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$toolDir = $PSScriptRoot
$repoDir = [IO.Path]::GetFullPath((Join-Path $toolDir "..\.."))
$exePath = Join-Path $repoDir "x64\Debug\NovaWing.exe"
$logDir = Join-Path $toolDir "Logs"
$statusPath = Join-Path $logDir "status.txt"
$reportPath = Join-Path $toolDir "PlaytestReport.html"

if (-not (Test-Path $exePath)) {
    Write-Host "Debugビルドの exe が見つかりません: $exePath" -ForegroundColor Red
    Write-Host "先に Debug でビルドしてください。"
    exit 1
}
if ($Speed -ne "max" -and -not ($Speed -match '^\d+$' -and [int]$Speed -ge 1)) {
    Write-Host "Speed は 1 以上の整数か max を指定してください。" -ForegroundColor Red
    exit 1
}

New-Item -ItemType Directory -Force $logDir | Out-Null
if (Test-Path $statusPath) { Remove-Item $statusPath }

$arguments = @("--autoplay", "--runs=$Runs", "--speed=$Speed", "--mode=$Mode", "--skill=$Skill",
    "--max-minutes=$MaxMinutes", "`"--out=$logDir`"")
if ($Seed -ge 0) { $arguments += "--seed=$Seed" }

Write-Host "AIテストプレイを開始します (回数:$Runs 速度:$Speed 画面:$Mode 腕前:$Skill)" -ForegroundColor Green
if ($Mode -eq "hidden") {
    Write-Host "画面は出ません。中断するときは Ctrl+C を押してください。"
}
else {
    Write-Host "ゲーム画面で ESC を押すと中断できます。"
}

$process = Start-Process -FilePath $exePath -ArgumentList $arguments -WorkingDirectory $repoDir -PassThru
try {
    $lastLine = ""
    while (-not $process.HasExited) {
        Start-Sleep -Milliseconds 1000
        if (Test-Path $statusPath) {
            $status = @{}
            Get-Content $statusPath -Encoding UTF8 | ForEach-Object {
                $pair = $_ -split '=', 2
                if ($pair.Count -eq 2) { $status[$pair[0]] = $pair[1] }
            }
            $line = "  {0}/{1}回目  終了:{2}回  クリア:{3}回  Z:{4}  HP:{5}" -f `
                $status["run"], $status["runs"], $status["done"], $status["clears"], $status["z"], $status["hp"]
            if ($line -ne $lastLine) {
                Write-Host $line
                $lastLine = $line
            }
        }
    }
}
finally {
    # Ctrl+C で止められたときはゲームも終了させる
    if (-not $process.HasExited) {
        Write-Host "中断しました。ゲームを終了します。" -ForegroundColor Yellow
        Stop-Process -Id $process.Id -Force
    }
}

Write-Host "終了しました。ログ: $logDir" -ForegroundColor Green
if (-not $NoReport) {
    Start-Process $reportPath
}
