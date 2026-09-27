<#
.SYNOPSIS
    Visual Studio の「クラスの追加」相当。.h/.cpp の雛形を生成し、NovaWing.vcxproj (と .filters) に登録する。

.PARAMETER ClassName
    クラス名 (C++ の識別子)
.PARAMETER BaseClass
    基底クラス名 (省略可)
.PARAMETER Folder
    配置先フォルダ。プロジェクトフォルダ (NovaWing\NovaWing) からの相対パス。例: Game/UI
.PARAMETER BaseHeader
    基底クラスのヘッダー。プロジェクトフォルダからの相対パス (例: Game/UI/UIBase.h)。
    省略時は「基底クラス名.h」をプロジェクト内から探し、1つだけ見つかればそれを使う。
.PARAMETER NoOpen
    生成後に .h を VS Code で開かない
#>
param(
    [Parameter(Mandatory = $true)][string]$ClassName,
    [string]$BaseClass = "",
    [Parameter(Mandatory = $true)][string]$Folder,
    [string]$BaseHeader = "",
    [switch]$NoOpen
)

. (Join-Path $PSScriptRoot "NovaWingProject.ps1")

# ---- 入力チェック ----------------------------------------------------------

$ClassName = $ClassName.Trim()
$BaseClass = $BaseClass.Trim()
$BaseHeader = $BaseHeader.Trim()

if ($ClassName -notmatch $CppIdentifier) { Fail "クラス名 '$ClassName' は C++ の識別子として不正です。" }
if ($BaseClass -and $BaseClass -notmatch $CppIdentifier) { Fail "基底クラス名 '$BaseClass' は C++ の識別子として不正です。" }
if (-not $BaseClass -and $BaseHeader) { Fail "基底ヘッダーが指定されていますが、基底クラス名がありません。" }

# 配置先フォルダ: vcxproj と同じ「\」区切りの相対パスに揃える
$relFolder = $Folder.Trim().Replace('/', '\').Trim('\')
if ([IO.Path]::IsPathRooted($relFolder)) { Fail "配置先フォルダはプロジェクトフォルダからの相対パスで指定してください。" }
$targetDir = [IO.Path]::GetFullPath((Join-Path $ProjectDir $relFolder)).TrimEnd('\')
if ($targetDir -ine $ProjectDir -and -not $targetDir.StartsWith($ProjectDir + '\', [StringComparison]::OrdinalIgnoreCase)) {
    Fail "配置先フォルダがプロジェクトフォルダの外を指しています: $Folder"
}

$hPath = Join-Path $targetDir "$ClassName.h"
$cppPath = Join-Path $targetDir "$ClassName.cpp"
$relH = Get-ProjectRelativePath $hPath
$relCpp = Get-ProjectRelativePath $cppPath

foreach ($path in $hPath, $cppPath) {
    if (Test-Path $path) { Fail "既にファイルが存在します (上書きしません): $path" }
}

# 基底クラスのヘッダー: 既存コードに合わせ「/」区切りのプロジェクト相対パスで include する
$baseInclude = ""
if ($BaseClass) {
    if ($BaseHeader) {
        $headerRel = $BaseHeader.Replace('/', '\').Trim('\')
        if (-not (Test-Path (Join-Path $ProjectDir $headerRel))) {
            Fail "基底ヘッダーが見つかりません: $(Join-Path $ProjectDir $headerRel)"
        }
    }
    else {
        $found = @(Get-ChildItem -Path $ProjectDir -Recurse -File -Filter "$BaseClass.h" |
            Where-Object { $_.FullName -notmatch '\\x64\\' })
        if ($found.Count -eq 0) { Fail "'$BaseClass.h' がプロジェクト内に見つかりません。基底ヘッダーのパスを指定してください。" }
        if ($found.Count -gt 1) {
            $list = ($found | ForEach-Object { Get-ProjectRelativePath $_.FullName }) -join ", "
            Fail "'$BaseClass.h' が複数見つかりました ($list)。基底ヘッダーのパスを指定してください。"
        }
        $headerRel = Get-ProjectRelativePath $found[0].FullName
    }
    $baseInclude = $headerRel.Replace('\', '/')
}

$vcxproj = Read-ProjectXml $VcxprojPath
$filters = Read-ProjectXml $FiltersPath
foreach ($relPath in $relCpp, $relH) {
    if (@(Find-ProjectItems $vcxproj $relPath).Count -gt 0) { Fail "vcxproj に既に登録されています: $relPath" }
}

# ---- 雛形の生成 ---------------------------------------------------------------

$h = New-Object System.Collections.Generic.List[string]
$h.Add("#pragma once")
if ($BaseClass) { $h.Add("#include `"$baseInclude`"") }
$h.Add("")
$h.Add("/// <summary>")
$h.Add("/// TODO: クラスの説明")
$h.Add("/// </summary>")
$h.Add($(if ($BaseClass) { "class $ClassName : public $BaseClass" } else { "class $ClassName" }))
$h.Add("{")
$h.Add("public:")
$h.Add("`t$ClassName();")
$h.Add("`tvirtual ~$ClassName();")
$h.Add("")
$h.Add("private:")
$h.Add("")
$h.Add("protected:")
$h.Add("};")

$cpp = New-Object System.Collections.Generic.List[string]
$cpp.Add("#include `"$ClassName.h`"")
$cpp.Add("")
if ($BaseClass) {
    $cpp.Add("$ClassName::$ClassName() :")
    $cpp.Add("`t//TODO: 基底クラスのコンストラクタに必要な引数を渡す")
    $cpp.Add("`t$BaseClass()")
}
else {
    $cpp.Add("$ClassName::$ClassName()")
}
$cpp.Add("{")
$cpp.Add("}")
$cpp.Add("")
$cpp.Add("$ClassName::~$ClassName()")
$cpp.Add("{")
$cpp.Add("}")

# ---- 書き込み (途中で失敗したら元に戻す) ----------------------------------------

$utf8Bom = New-Object System.Text.UTF8Encoding($true)   # 既存ソースと同じ BOM 付き UTF-8 / CRLF
$createdDir = -not (Test-Path $targetDir)
Invoke-WithProjectBackup {
    if ($createdDir) { New-Item -ItemType Directory -Path $targetDir | Out-Null }
    [IO.File]::WriteAllText($hPath, (($h -join "`r`n") + "`r`n"), $utf8Bom)
    [IO.File]::WriteAllText($cppPath, (($cpp -join "`r`n") + "`r`n"), $utf8Bom)

    Add-ProjectItem $vcxproj "ClCompile" $relCpp
    Add-ProjectItem $vcxproj "ClInclude" $relH
    Add-ProjectItem $filters "ClCompile" $relCpp
    Add-ProjectItem $filters "ClInclude" $relH
    Save-ProjectXml $vcxproj
    Save-ProjectXml $filters
} {
    foreach ($path in $hPath, $cppPath) { if (Test-Path $path) { Remove-Item $path } }
    if ($createdDir -and (Test-Path $targetDir)) { Remove-Item $targetDir }
}

Write-Host "クラス '$ClassName' を追加しました。" -ForegroundColor Green
Write-Host "  $relH"
Write-Host "  $relCpp"
if ($BaseClass) { Write-Host "  基底クラス: $BaseClass (#include `"$baseInclude`")" }
if ($createdDir) { Write-Host "  フォルダを新規作成: $(Get-ProjectRelativePath $targetDir)" }
Write-Host "  NovaWing.vcxproj / NovaWing.vcxproj.filters に登録済み"

if (-not $NoOpen -and (Get-Command code -ErrorAction SilentlyContinue)) {
    & code -r $hPath
}
