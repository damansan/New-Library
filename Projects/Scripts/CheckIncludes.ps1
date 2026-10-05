# include の層ルールを検査する
# 使い方: CheckIncludes.ps1 -Root <New-Libraryのパス> -Project <プロジェクト名>
param(
    [string]$Root,
    [string]$Project
)

# 層ごとに、include してよい層(段階1で決めたルール)
$allowed = @{
    'Engine'   = @('Engine')
    'Game'     = @('Engine', 'Game')
    'Tool'     = @('Engine', 'Tool')
    'GameTool' = @('Engine', 'Game', 'Tool', 'GameTool')
    'App'      = @('Engine', 'Game', 'Tool', 'GameTool', 'App')
    'Launcher' = @()
}

# プロジェクト名 → 検査する層
$projectToLayer = @{
    'Engine'    = 'Engine'
    'Game'      = 'Game'
    'Tool'      = 'Tool'
    'GameTool'  = 'GameTool'
    'EditorApp' = 'App'
    'GameApp'   = 'App'
    'Launcher'  = 'Launcher'
}

$layer = $projectToLayer[$Project]
if (-not $layer) { exit 0 }  # 対象外のプロジェクト(外部ライブラリなど)

$codeDir = Join-Path $Root 'Source\Code'
$layerDir = Join-Path $codeDir $layer

# 検査するファイル
$targets = @()
if (Test-Path $layerDir) {
    $targets += Get-ChildItem $layerDir -Recurse -File -Include *.h, *.hpp, *.cpp
}
# main.cpp など Source\Code 直下のファイルは App の層として扱う
if ($layer -eq 'App') {
    $targets += Get-ChildItem (Join-Path $codeDir '*') -File -Include *.h, *.hpp, *.cpp
}

$errorCount = 0
foreach ($file in $targets) {
    $lineNo = 0
    foreach ($line in [IO.File]::ReadAllLines($file.FullName)) {
        $lineNo++
        if ($line -match '^\s*#\s*include\s*"([A-Za-z]+)/') {
            $target = $Matches[1]
            # 層の名前への include で、許されていないものを違反とする
            if ($allowed.ContainsKey($target) -and -not ($allowed[$layer] -contains $target)) {
                # エラー一覧に表示される形式: ファイル(行): error コード: メッセージ
                Write-Output ("{0}({1}): error LAYER001: {2} の層から {3} の層は include できません: {4}" -f $file.FullName, $lineNo, $layer, $target, $line.Trim())
                $errorCount++
            }
        }
    }
}

if ($errorCount -gt 0) { exit 1 }
exit 0