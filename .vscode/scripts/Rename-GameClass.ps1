<#
.SYNOPSIS
    クラスの .h/.cpp のファイル名を変更し、NovaWing.vcxproj (と .filters) のパスも更新する。
    ファイル名だけが対象。クラス名・コンストラクタ名・#include 文の中身は変更しない。

.PARAMETER Path
    リネームするクラスの .h または .cpp (拡張子なしでも可)。空なら -CurrentFile を使う
.PARAMETER NewName
    新しいファイル名 (拡張子なし)
.PARAMETER CurrentFile
    VS Code で開いているファイル (タスクから ${file} を渡す)
#>
param(
    [string]$Path = "",
    [Parameter(Mandatory = $true)][string]$NewName,
    [string]$CurrentFile = ""
)

. (Join-Path $PSScriptRoot "NovaWingProject.ps1")

if (-not $Path.Trim()) { $Path = $CurrentFile }
$pair = Resolve-ClassPair $Path
$NewName = $NewName.Trim()

if ($NewName -notmatch $CppIdentifier) { Fail "新しい名前 '$NewName' はファイル名(クラス名)として使えません。英数字と _ のみ、先頭は数字以外にしてください。" }
if ($NewName -ceq $pair.Name) { Fail "新しい名前が今の名前と同じです。" }

$renames = @()
foreach ($file in $pair.Files) {
    $newFullPath = Join-Path $pair.Dir ($NewName + [IO.Path]::GetExtension($file.FullPath))
    # 大文字小文字だけの変更 (Foo → foo) は同じファイルを指すので存在チェックしない
    if ((Test-Path $newFullPath) -and ($newFullPath -ine $file.FullPath)) { Fail "既にファイルが存在します: $newFullPath" }
    $renames += [pscustomobject]@{
        Old = $file
        NewFullPath = $newFullPath
        NewRelPath = Get-ProjectRelativePath $newFullPath
    }
}

$vcxproj = Read-ProjectXml $VcxprojPath
$filters = Read-ProjectXml $FiltersPath
foreach ($r in $renames) {
    if (@(Find-ProjectItems $vcxproj $r.NewRelPath).Count -gt 0 -and $r.NewRelPath -ine $r.Old.RelPath) {
        Fail "vcxproj に既に登録されています: $($r.NewRelPath)"
    }
}

# 旧ファイルを #include している箇所 (リネーム後に解決できなくなる箇所)。リネーム前に調べておく
$references = @(Find-IncludeReferences @($pair.Files | ForEach-Object { $_.FullPath }))

# ---- 変更内容の表示と確認 ---------------------------------------------------------

Write-Host "次の変更を行います (ファイル名のみ。クラス名や #include 文は変更しません):" -ForegroundColor Cyan
foreach ($r in $renames) {
    Write-Host "  $($r.Old.RelPath)  →  $($r.NewRelPath)"
}
Write-Host "  NovaWing.vcxproj / NovaWing.vcxproj.filters のパスも更新します。"
if ($references.Count -gt 0) {
    Write-Host "  ※ 次の #include はリネーム後に手動で直す必要があります:" -ForegroundColor Yellow
    Write-References $references
}
Confirm-OrExit "実行しますか?"

# ---- 実行 ---------------------------------------------------------------------

$done = New-Object System.Collections.Generic.List[object]
Invoke-WithProjectBackup {
    foreach ($r in $renames) {
        foreach ($project in $vcxproj, $filters) {
            Find-ProjectItems $project $r.Old.RelPath | ForEach-Object { $_.SetAttribute("Include", $r.NewRelPath) }
        }
    }
    Save-ProjectXml $vcxproj
    Save-ProjectXml $filters

    foreach ($r in $renames) {
        Rename-Item -LiteralPath $r.Old.FullPath -NewName ([IO.Path]::GetFileName($r.NewFullPath))
        $done.Add($r)
    }
} {
    # 途中で失敗したら、リネーム済みのファイルを元の名前に戻す
    foreach ($r in $done) { Rename-Item -LiteralPath $r.NewFullPath -NewName ([IO.Path]::GetFileName($r.Old.FullPath)) }
}

Write-Host "'$($pair.Name)' → '$NewName' にファイル名を変更しました。" -ForegroundColor Green
if ($references.Count -gt 0) {
    # 参照元がリネームしたファイル自身 (.cpp が自分の .h を include) なら新しい名前で表示する
    foreach ($ref in $references) {
        $renamed = $renames | Where-Object { $_.Old.FullPath -ieq $ref.FullPath }
        if ($renamed) { $ref.File = $renamed.NewRelPath }
    }
    Write-Host "警告: 次の箇所は旧ファイル名を #include したままです (自動修正していません):" -ForegroundColor Yellow
    Write-References $references
}
Write-Host "クラス名・コンストラクタ名などは変更していません。必要なら手動で直してください。"
