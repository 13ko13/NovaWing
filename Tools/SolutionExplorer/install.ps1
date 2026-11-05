# VSCode 用「ソリューション エクスプローラー」拡張を、このPCのVSCodeに入れる(家・学校どちらでも)
# 使い方: PowerShell でこのファイルを実行 → VSCode で「Developer: Reload Window」
# 拡張のソースを直したら、もう一度実行すれば上書きされる

$source = $PSScriptRoot
$package = Get-Content (Join-Path $source 'package.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$destination = Join-Path $env:USERPROFILE ".vscode\extensions\$($package.publisher).$($package.name)-$($package.version)"

New-Item -ItemType Directory -Force $destination | Out-Null
Copy-Item (Join-Path $source '*.js'), (Join-Path $source 'package.json') $destination -Force

Write-Host "インストールしました: $destination"
Write-Host "VSCode で Ctrl+Shift+P → 'Developer: Reload Window' を実行してください"
