# Run-GuiTest.ps1
# Builds the TextBlinder GUI test driver and runs it on an isolated desktop.
# It does not change the installed Notepad++ or its settings.
# It does not use the user's screen, keyboard, mouse or clipboard.
#
# Steps:
#   1. Copy the installed Notepad++ to x64\GuiTest\npp-portable (portable mode, no updater).
#   2. Build x64\GuiTest\GuiTest.exe with the .NET Framework C# compiler.
#   3. Run the mode. "full" installs x64\Release\TextBlinder.dll with
#      Settings > Import > Import plugin(s), then checks the plugin.
#
# Build TextBlinder.sln (x64 Release) first.
#
# Examples:
#   .\Tools\GuiTest\Run-GuiTest.ps1                  # full check
#   .\Tools\GuiTest\Run-GuiTest.ps1 -Mode stress     # close and reopen 60 times
#   .\Tools\GuiTest\Run-GuiTest.ps1 -Mode showcase   # screenshots in x64\GuiTest\work

param(
    [ValidateSet('full', 'stress', 'showcase')]
    [string]$Mode = 'full',
    [string]$NppDir = 'C:\Program Files\Notepad++',
    [switch]$FreshCopy
)

$ErrorActionPreference = 'Stop'

function Fail([string]$Message) {
    Write-Host $Message -ForegroundColor Red
    exit 1
}

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$out = Join-Path $repo 'x64\GuiTest'
$portable = Join-Path $out 'npp-portable'
$work = Join-Path $out 'work'
$dll = Join-Path $repo 'x64\Release\TextBlinder.dll'

if (-not (Test-Path -LiteralPath $dll)) {
    Fail "Build output not found: $dll`nBuild TextBlinder.sln (x64 Release) first."
}
if (-not (Test-Path -LiteralPath (Join-Path $NppDir 'notepad++.exe'))) {
    Fail "notepad++.exe not found in $NppDir`nSet the Notepad++ folder with -NppDir."
}

# 1. Portable copy of Notepad++. doLocalConf.xml keeps its settings inside the copy.
if ($FreshCopy -and (Test-Path -LiteralPath $portable)) {
    Remove-Item -LiteralPath $portable -Recurse -Force
}
if (-not (Test-Path -LiteralPath (Join-Path $portable 'notepad++.exe'))) {
    Write-Host "Copying $NppDir to $portable"
    robocopy $NppDir $portable /E /NFL /NDL /NJH /NJS /NP /XD (Join-Path $NppDir 'updater') | Out-Null
    if ($LASTEXITCODE -ge 8) { Fail "robocopy failed ($LASTEXITCODE)." }
    Set-Content -LiteralPath (Join-Path $portable 'doLocalConf.xml') -Value '' -NoNewline
    # Use the user's UI language and shortcuts. The originals are only read.
    foreach ($name in 'nativeLang.xml', 'shortcuts.xml') {
        $src = Join-Path $env:APPDATA "Notepad++\$name"
        if (Test-Path -LiteralPath $src) { Copy-Item -LiteralPath $src -Destination $portable }
    }
}

# 2. Build the driver.
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $csc)) { Fail "C# compiler not found: $csc" }
$exe = Join-Path $out 'GuiTest.exe'
$sources = (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.cs').FullName
& $csc -nologo "-out:$exe" -r:System.Drawing.dll -r:System.Windows.Forms.dll $sources
if ($LASTEXITCODE -ne 0) { Fail 'Driver build failed.' }

# 3. Run on an isolated desktop. The log is UTF-8.
New-Item -ItemType Directory -Force -Path $work | Out-Null
[Console]::OutputEncoding = [Text.Encoding]::UTF8
& $exe $Mode $work $portable $dll
exit $LASTEXITCODE
