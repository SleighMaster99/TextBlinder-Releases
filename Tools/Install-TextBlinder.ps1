# Install-TextBlinder.ps1
# Copies the built TextBlinder.dll to <Notepad++>\plugins\TextBlinder\.
#
# - Build TextBlinder.sln (x64) first.
# - Close Notepad++ first. It locks the DLL while it runs.
# - Writing under C:\Program Files needs an elevated PowerShell.
#
# Example:
#   .\Tools\Install-TextBlinder.ps1
#   .\Tools\Install-TextBlinder.ps1 -Configuration Debug -NppDir 'D:\Apps\Notepad++'

param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [string]$NppDir = 'C:\Program Files\Notepad++'
)

$ErrorActionPreference = 'Stop'

function Fail([string]$Message) {
    Write-Host $Message -ForegroundColor Red
    exit 1
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$dll = Join-Path $repoRoot "x64\$Configuration\TextBlinder.dll"
if (-not (Test-Path -LiteralPath $dll)) {
    Fail "Build output not found: $dll`nBuild TextBlinder.sln (x64 $Configuration) first."
}

if (-not (Test-Path -LiteralPath (Join-Path $NppDir 'notepad++.exe'))) {
    Fail "notepad++.exe not found in $NppDir`nSet the Notepad++ folder with -NppDir."
}

$running = Get-Process -Name 'notepad++' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($NppDir, [StringComparison]::OrdinalIgnoreCase) }
if ($running) {
    Fail 'Notepad++ is running. Close it and run this script again.'
}

$target = Join-Path $NppDir 'plugins\TextBlinder'
try {
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Copy-Item -LiteralPath $dll -Destination $target -Force
}
catch [System.UnauthorizedAccessException] {
    Fail "Access denied: $target`nRun PowerShell as administrator."
}
catch [System.IO.IOException] {
    Fail "Copy failed: $($_.Exception.Message)`nIf Notepad++ is running, close it and try again."
}

Write-Host "Installed: $(Join-Path $target 'TextBlinder.dll')"
