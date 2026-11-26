param([string]$OutputPath = (Join-Path $PSScriptRoot 'WeirdWords.exe'))

$ErrorActionPreference = 'Stop'

$compiler = (Get-Command g++.exe).Source
$resourceCompiler = (Get-Command windres.exe).Source
$sourceDirectory = Join-Path $PSScriptRoot 'src'
$resourceObject = [IO.Path]::GetTempFileName()

try {
    & $resourceCompiler --include-dir $sourceDirectory --input (Join-Path $sourceDirectory 'app.rc') --output-format coff --output $resourceObject
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }

    & $compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -municode -mwindows -static -s `
        (Join-Path $sourceDirectory 'main.cpp') $resourceObject -o $OutputPath -lgdiplus -ldwmapi -lgdi32
    if ($LASTEXITCODE -ne 0) { throw 'C++ compilation failed.' }

    Get-Item -LiteralPath $OutputPath
} finally {
    Remove-Item -LiteralPath $resourceObject -ErrorAction SilentlyContinue
}
