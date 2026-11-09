<# : ここから下の数行は cmd 用（PowerShell からはコメントに見える）
@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ([System.IO.File]::ReadAllText('%~f0', [System.Text.Encoding]::GetEncoding(932)))"
pause
goto :eof
#>

# ここから下は PowerShell 用
# ソースと設計メモを集めて、collected_all.txt に UTF-8 で書き出す
$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path

$utf8Strict = New-Object System.Text.UTF8Encoding($false, $true)  # 壊れた UTF-8 なら例外
$sjis       = [System.Text.Encoding]::GetEncoding(932)

# ファイルの文字コードを見分けて、文字列として読む
function Read-SourceText([string]$path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)

    # BOM 付き UTF-8
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        return $utf8Strict.GetString($bytes, 3, $bytes.Length - 3)
    }
    # BOM 付き UTF-16
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
        return [System.Text.Encoding]::Unicode.GetString($bytes, 2, $bytes.Length - 2)
    }
    # BOM なし：UTF-8 として読めればそれ、読めなければ Shift-JIS
    try   { return $utf8Strict.GetString($bytes) }
    catch { return $sjis.GetString($bytes) }
}

$outPath = Join-Path $root 'collected_all.txt'
$writer  = New-Object System.IO.StreamWriter($outPath, $false, (New-Object System.Text.UTF8Encoding($true)))

try {
    # 設計メモを先頭に書き出す
    $notes = Join-Path $root 'DESIGN_NOTES.md'
    if (Test-Path -LiteralPath $notes) {
        $writer.WriteLine('===== DESIGN_NOTES.md =====')
        $writer.WriteLine((Read-SourceText $notes))
        $writer.WriteLine()
        $writer.WriteLine()
    }

    # ソースを書き出す（今までと同じく、このフォルダから下を全部）
    Get-ChildItem -Path $root -Recurse -File -Include *.h, *.cpp | ForEach-Object {
        $writer.WriteLine("===== $($_.Name) =====")
        $writer.WriteLine("Path: $($_.FullName)")
        $writer.WriteLine((Read-SourceText $_.FullName))
        $writer.WriteLine()
        $writer.WriteLine()
    }
}
finally {
    $writer.Close()
}

Write-Host 'Done.'