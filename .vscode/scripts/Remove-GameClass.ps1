<#
.SYNOPSIS
    クラスの .h/.cpp を NovaWing.vcxproj (と .filters) から外し、ファイルをごみ箱へ移動する。
    他のファイルから #include されている場合は中断する。

.PARAMETER Path
    削除するクラスの .h または .cpp (拡張子なしでも可)。空なら -CurrentFile を使う
.PARAMETER CurrentFile
    VS Code で開いているファイル (タスクから ${file} を渡す)
#>
param(
    [string]$Path = "",
    [string]$CurrentFile = ""
)

. (Join-Path $PSScriptRoot "NovaWingProject.ps1")
Add-Type -AssemblyName Microsoft.VisualBasic

if (-not $Path.Trim()) { $Path = $CurrentFile }
$pair = Resolve-ClassPair $Path
$targetPaths = @($pair.Files | ForEach-Object { $_.FullPath })

# ---- 事前チェック: 他のファイルからの #include ----------------------------------------

# 自分自身の .cpp が .h を include しているのは参照に数えない
$references = @(Find-IncludeReferences $targetPaths $targetPaths)
if ($references.Count -gt 0) {
    Write-Host "エラー: '$($pair.Name)' は次の場所から #include されているため、削除を中断しました。" -ForegroundColor Red
    Write-References $references
    Write-Host "先にこれらの #include (と使用箇所) を取り除いてから、もう一度実行してください。"
    exit 1
}

# ---- 変更内容の表示と確認 ---------------------------------------------------------

$vcxproj = Read-ProjectXml $VcxprojPath
$filters = Read-ProjectXml $FiltersPath

Write-Host "次の変更を行います:" -ForegroundColor Cyan
foreach ($file in $pair.Files) {
    $inProject = @(Find-ProjectItems $vcxproj $file.RelPath).Count -gt 0
    $inFilters = @(Find-ProjectItems $filters $file.RelPath).Count -gt 0
    Write-Host "  削除 (ごみ箱へ): $($file.RelPath)"
    Write-Host "      vcxproj から除去: $(if ($inProject) { 'あり' } else { '登録なし' })   filters から除去: $(if ($inFilters) { 'あり' } else { '登録なし' })"
}
Confirm-OrExit "実行しますか?"

# ---- 実行 ---------------------------------------------------------------------

Invoke-WithProjectBackup {
    foreach ($file in $pair.Files) {
        Find-ProjectItems $vcxproj $file.RelPath | ForEach-Object { Remove-ProjectItem $_ }
        Find-ProjectItems $filters $file.RelPath | ForEach-Object { Remove-ProjectItem $_ }
    }
    Save-ProjectXml $vcxproj
    Save-ProjectXml $filters

    # 誤削除に備え、完全削除ではなくごみ箱へ送る
    foreach ($file in $pair.Files) {
        [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteFile(
            $file.FullPath,
            [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs,
            [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin)
    }
}

Write-Host "クラス '$($pair.Name)' を削除しました (ファイルはごみ箱にあります)。" -ForegroundColor Green
if (-not (Get-ChildItem -Path $pair.Dir -Force)) {
    Write-Host "  フォルダ '$(Get-ProjectRelativePath $pair.Dir)' が空になりました (自動では削除しません)。"
}
