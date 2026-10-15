#Requires -Version 7
<#
.SYNOPSIS
    Package an already-built Release/x64 runtime as a per-user installer.
.DESCRIPTION
    Does not compile Terminal or install the resulting package. Creates a new
    staging directory for each run. User settings and build intermediates are
    never copied. Requires the official Inno Setup compiler.
#>
[CmdletBinding()]
param(
    [string]$Compiler = 'D:\Dev\sometools\InnoSetup6\ISCC.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $root 'bin\x64\Release\WindowsTerminal'
$required = @('WindowsTerminal.exe', 'OpenConsole.exe', 'TerminalApp.dll',
    'TerminalConnection.dll', 'Microsoft.Terminal.Control.dll',
    'Microsoft.Terminal.Settings.Model.dll', 'Microsoft.Terminal.Settings.Editor.dll',
    'Microsoft.Terminal.UI.dll', 'Microsoft.Terminal.UI.Markdown.dll',
    'Microsoft.UI.Xaml.dll', 'resources.pri')
foreach ($name in $required) {
    $file = Join-Path $runtime $name
    if (!(Test-Path -LiteralPath $file -PathType Leaf) -or (Get-Item -LiteralPath $file).Length -eq 0) {
        throw "Missing Release runtime: $file. Run Build-TerminalEL.ps1 -Configuration Release first."
    }
}
if (!(Test-Path -LiteralPath $Compiler -PathType Leaf)) { throw "Inno Setup compiler missing: $Compiler" }
$artifactDir = Join-Path $root 'bin\terminalEL-release\0.1.0'
$payload = Join-Path $root "obj\terminalEL-package\$(Get-Date -Format yyyyMMdd-HHmmss-fff)\payload"
New-Item -ItemType Directory -Path $payload -Force | Out-Null
New-Item -ItemType Directory -Path $artifactDir -Force | Out-Null
# Keep runtime subdirectories/resources but exclude caches, private data and
# development outputs. Reparse points are rejected rather than followed.
function Copy-Runtime([string]$Source, [string]$Destination) {
    foreach ($entry in Get-ChildItem -LiteralPath $Source -Force) {
        if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Unexpected reparse point: $($entry.FullName)" }
        if ($entry.PSIsContainer) {
            if ($entry.Name -in @('settings', '_xaml', 'Appx', 'AppPackages')) { continue }
            $target = Join-Path $Destination $entry.Name
            New-Item -ItemType Directory -Path $target -Force | Out-Null
            Copy-Runtime $entry.FullName $target
        } elseif ($entry.Name -ne 'stamp' -and $entry.Name -ne '.portable' -and
                  $entry.Extension -notin @('.pdb', '.lib', '.exp', '.obj', '.log', '.binlog', '.ilk', '.iobj', '.ipdb', '.tlog', '.lastbuildstate')) {
            Copy-Item -LiteralPath $entry.FullName -Destination $Destination
        }
    }
}
Copy-Runtime $runtime $payload
# Portable-mode marker isolates configuration from upstream unpackaged Terminal.
[IO.File]::WriteAllText((Join-Path $payload '.portable'), '')
foreach ($name in @('LICENSE', 'NOTICE.md', 'README.md')) {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $payload
}
$docs = Join-Path $payload 'doc'
New-Item -ItemType Directory -Path $docs | Out-Null
Get-ChildItem -LiteralPath (Join-Path $root 'doc') -File |
    Where-Object { $_.Name -like 'terminalEL-*.md' -or $_.Name -like 'EL-Amber-Code*.json' } |
    Copy-Item -Destination $docs
$tools = Join-Path $payload 'tools'
New-Item -ItemType Directory -Path $tools | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Show-TerminalELColors.ps1') -Destination $tools
$revision = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source revision.' }
$dirty = @(& git -C $root status --porcelain).Count -gt 0
$metadata = [ordered]@{ version = '0.1.0'; architecture = 'x64'; configuration = 'Release'; sourceCommit = $revision; sourceModified = $dirty; builtAt = (Get-Date).ToUniversalTime().ToString('o'); executableVersion = (Get-Item (Join-Path $payload 'WindowsTerminal.exe')).VersionInfo.FileVersion }
$metadata | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'terminalEL-build.json') -Encoding utf8
& $Compiler "/DPayloadDir=$payload" "/DArtifactDir=$artifactDir" (Join-Path $PSScriptRoot 'TerminalEL-Setup.iss')
if ($LASTEXITCODE -ne 0) { throw "Installer compiler failed: $LASTEXITCODE" }
$installer = Join-Path $artifactDir 'TerminalEL-0.1.0-windows-x64-setup.exe'
$hash = Get-FileHash -LiteralPath $installer -Algorithm SHA256
"$($hash.Hash.ToLowerInvariant())  $([IO.Path]::GetFileName($installer))" | Set-Content -LiteralPath (Join-Path $artifactDir 'SHA256SUMS.txt') -Encoding ascii
Write-Host "Installer: $installer"
Write-Host "Payload: $payload"
Write-Host 'Unsigned installer: Windows may show Unknown publisher. Configuration is preserved on uninstall.'
