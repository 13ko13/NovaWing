<#
    New-GameClass / Remove-GameClass / Rename-GameClass で共通して使う処理。
    各スクリプトの先頭で「. (Join-Path $PSScriptRoot "NovaWingProject.ps1")」として読み込む。
#>

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$script:ProjectDir = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\..\NovaWing\NovaWing"))
$script:WorkspaceDir = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$script:VcxprojPath = Join-Path $ProjectDir "NovaWing.vcxproj"
$script:FiltersPath = "$VcxprojPath.filters"
$script:MsbuildNs = "http://schemas.microsoft.com/developer/msbuild/2003"
# vcxproj の AdditionalIncludeDirectories と同じ順
$script:IncludeDirs = @(
    [IO.Path]::GetFullPath((Join-Path $ProjectDir "..\DxLib_h")),
    $ProjectDir,
    (Join-Path $ProjectDir "Game\GameObjects\Actors")
)
$script:CppIdentifier = '^[A-Za-z_][A-Za-z0-9_]*$'

function Fail([string]$message) {
    Write-Host "エラー: $message" -ForegroundColor Red
    exit 1
}

# y と入力されたときだけ続行する。それ以外は何も変更せず終了
function Confirm-OrExit([string]$question) {
    $answer = Read-Host "$question (y/N)"
    if ($answer -notmatch '^[yY]') {
        Write-Host "中止しました。何も変更していません。" -ForegroundColor Yellow
        exit 0
    }
}

# プロジェクトフォルダからの相対パス (vcxproj と同じ「\」区切り) に変換する
function Get-ProjectRelativePath([string]$fullPath) {
    return $fullPath.Substring($ProjectDir.Length).TrimStart('\')
}

# ---- vcxproj / filters ---------------------------------------------------------

function Read-ProjectXml([string]$path) {
    $doc = New-Object System.Xml.XmlDocument
    # 空白ノードを保持し、既存のインデント・改行を崩さない
    $doc.PreserveWhitespace = $true
    $doc.Load($path)
    $manager = New-Object System.Xml.XmlNamespaceManager($doc.NameTable)
    $manager.AddNamespace("m", $MsbuildNs)
    return [pscustomobject]@{ Doc = $doc; Ns = $manager; Path = $path }
}

function Save-ProjectXml($project) {
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Encoding = New-Object System.Text.UTF8Encoding($true)   # 元ファイルと同じ BOM 付き UTF-8
    $settings.NewLineHandling = [System.Xml.NewLineHandling]::None
    $writer = [System.Xml.XmlWriter]::Create($project.Path, $settings)
    try { $project.Doc.Save($writer) } finally { $writer.Close() }
}

# Include 属性が一致する ClCompile / ClInclude 要素を返す
# ※ PowerShell 5.1 では1件だけ返すと配列でなくなり .Count が使えないため、呼び出し側で @() で包むこと
function Find-ProjectItems($project, [string]$relPath) {
    return @($project.Doc.SelectNodes("//m:ItemGroup/m:ClCompile | //m:ItemGroup/m:ClInclude", $project.Ns) |
        Where-Object { $_.GetAttribute("Include") -ieq $relPath })
}

# 指定した種類の項目を持つ ItemGroup の末尾に追加する (Visual Studio も末尾に追加する)
# .filters の場合は直前の項目と同じフィルタ (「ソース ファイル」等) に入れる
function Add-ProjectItem($project, [string]$itemType, [string]$relPath) {
    $doc = $project.Doc
    $siblings = $doc.SelectNodes("//m:ItemGroup/m:$itemType", $project.Ns)
    if ($siblings.Count -eq 0) { throw "$itemType の ItemGroup が見つかりません: $($project.Path)" }
    $last = $siblings[$siblings.Count - 1]

    # 直前の空白ノードをそのまま複製して、既存と同じインデントにする
    $indent = if ($last.PreviousSibling -and $last.PreviousSibling.NodeType -eq [System.Xml.XmlNodeType]::Whitespace) {
        $last.PreviousSibling.Value
    } else { "`r`n    " }

    $element = $doc.CreateElement($itemType, $MsbuildNs)
    $element.SetAttribute("Include", $relPath)

    $filterNode = $last.SelectSingleNode("m:Filter", $project.Ns)
    if ($filterNode) {
        $filter = $doc.CreateElement("Filter", $MsbuildNs)
        $filter.InnerText = $filterNode.InnerText
        [void]$element.AppendChild($doc.CreateWhitespace($filterNode.PreviousSibling.Value))
        [void]$element.AppendChild($filter)
        [void]$element.AppendChild($doc.CreateWhitespace($indent))
    }

    $ws = $doc.CreateWhitespace($indent)
    [void]$last.ParentNode.InsertAfter($ws, $last)
    [void]$last.ParentNode.InsertAfter($element, $ws)
}

# 要素を、その前にあるインデント用の空白ノードごと取り除く
function Remove-ProjectItem($element) {
    $parent = $element.ParentNode
    $prev = $element.PreviousSibling
    if ($prev -and $prev.NodeType -eq [System.Xml.XmlNodeType]::Whitespace) { [void]$parent.RemoveChild($prev) }
    [void]$parent.RemoveChild($element)
}

# 処理中に例外が出たら、vcxproj / filters を実行前の内容に戻してから例外を投げ直す
function Invoke-WithProjectBackup([scriptblock]$action, [scriptblock]$onRollback = {}) {
    $vcxprojBackup = [IO.File]::ReadAllBytes($VcxprojPath)
    $filtersBackup = [IO.File]::ReadAllBytes($FiltersPath)
    try {
        & $action
    }
    catch {
        & $onRollback
        [IO.File]::WriteAllBytes($VcxprojPath, $vcxprojBackup)
        [IO.File]::WriteAllBytes($FiltersPath, $filtersBackup)
        Fail "処理に失敗したため元に戻しました: $($_.Exception.Message)"
    }
}

# ---- 対象クラスの特定 ------------------------------------------------------------

# .h / .cpp / 拡張子なしのどれを渡されても、同じフォルダ・同じ名前の .h と .cpp の組を返す
# パスは絶対パス、プロジェクトフォルダからの相対、ワークスペースからの相対のどれでもよい
function Resolve-ClassPair([string]$path) {
    $path = $path.Trim().Trim('"').Replace('/', '\')
    if (-not $path) { Fail "対象ファイルが指定されていません。" }

    $candidates = if ([IO.Path]::IsPathRooted($path)) { @($path) }
        else { @((Join-Path $ProjectDir $path), (Join-Path $WorkspaceDir $path)) }
    $full = $null
    foreach ($candidate in $candidates) {
        $candidate = [IO.Path]::GetFullPath($candidate)
        $stem = [IO.Path]::ChangeExtension($candidate, $null).TrimEnd('.')
        if ((Test-Path "$stem.h") -or (Test-Path "$stem.cpp")) { $full = $stem; break }
    }
    if (-not $full) { Fail "対象の .h / .cpp が見つかりません: $path" }
    if (-not $full.StartsWith($ProjectDir + '\', [StringComparison]::OrdinalIgnoreCase)) {
        Fail "プロジェクトフォルダ ($ProjectDir) の外のファイルは扱えません: $full"
    }

    $files = @()
    foreach ($ext in ".h", ".cpp") {
        $filePath = "$full$ext"
        if (Test-Path $filePath) {
            $files += [pscustomobject]@{
                FullPath = $filePath
                RelPath  = Get-ProjectRelativePath $filePath
                ItemType = if ($ext -eq ".h") { "ClInclude" } else { "ClCompile" }
            }
        }
    }
    return [pscustomobject]@{
        Name  = [IO.Path]::GetFileName($full)
        Dir   = [IO.Path]::GetDirectoryName($full)
        Files = $files
    }
}

# ---- #include の参照検索 ---------------------------------------------------------

# コンパイラと同じ順 (インクルード元のフォルダ → 追加のインクルードディレクトリ) で解決する
function Resolve-IncludePath([string]$includingFile, [string]$include) {
    $include = $include.Replace('/', '\')
    foreach ($dir in @([IO.Path]::GetDirectoryName($includingFile)) + $IncludeDirs) {
        $candidate = [IO.Path]::GetFullPath((Join-Path $dir $include))
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

# $targetPaths のいずれかを #include しているソースの一覧を返す (x64 の中間フォルダは除く)
# ※ Find-ProjectItems と同じく、呼び出し側で @() で包むこと
function Find-IncludeReferences([string[]]$targetPaths, [string[]]$excludePaths = @()) {
    $sources = Get-ChildItem -Path $ProjectDir -Recurse -File -Include *.h, *.cpp, *.hpp, *.inl |
        Where-Object { $_.FullName -notmatch '\\x64\\' -and $excludePaths -notcontains $_.FullName }

    $references = @()
    foreach ($source in $sources) {
        $lineNo = 0
        foreach ($line in [IO.File]::ReadAllLines($source.FullName)) {
            $lineNo++
            if ($line -notmatch '^\s*#\s*include\s*"([^"]+)"') { continue }
            $resolved = Resolve-IncludePath $source.FullName $Matches[1]
            if ($resolved -and ($targetPaths | Where-Object { $_ -ieq $resolved })) {
                $references += [pscustomobject]@{
                    File     = Get-ProjectRelativePath $source.FullName
                    Line     = $lineNo
                    Text     = $line.Trim()
                    FullPath = $source.FullName
                }
            }
        }
    }
    return $references
}

function Write-References($references) {
    foreach ($ref in $references) {
        Write-Host "    $($ref.File):$($ref.Line)  $($ref.Text)"
    }
}
