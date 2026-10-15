# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

<#
.SYNOPSIS
    Read-only terminalEL build and unpackaged-layout preflight. Does not run MSBuild.
.DESCRIPTION
    Checks local tools and existing files only. Does not restore packages, clean,
    compile, launch Terminal, or change environment/configuration. PASS is not a
    guarantee that C++/WinRT, XAML, or shader compilation will succeed.
#>
[CmdletBinding()]
param(
    [string]$VisualStudioPath,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('x64', 'ARM64', 'Win32')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$checks = [System.Collections.Generic.List[object]]::new()
function Add-Check([string]$Status, [string]$Check, [string]$Detail) {
    $checks.Add([pscustomobject]@{ Status = $Status; Check = $Check; Detail = $Detail })
}
function Test-File([string]$Check, [string]$Path, [string]$MissingStatus = 'FAIL') {
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        Add-Check 'PASS' $Check $Path
    } else {
        Add-Check $MissingStatus $Check "Missing: $Path"
    }
}

if (!$VisualStudioPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $VisualStudioPath = & $vswhere -latest -products '*' -version '[18.6,)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    }
}
if ($VisualStudioPath) {
    Test-File 'MSBuild (x64)' (Join-Path $VisualStudioPath 'MSBuild\Current\Bin\amd64\MSBuild.exe')
    Test-File 'VC toolset selection' (Join-Path $VisualStudioPath 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt')
    Test-File 'XAML build targets' (Join-Path $VisualStudioPath 'MSBuild\Microsoft\WindowsXaml\v18.0\Microsoft.Windows.UI.Xaml.CPP.Targets') 'WARN'
} else {
    Add-Check 'FAIL' 'Visual Studio' 'VS 2026 >= 18.6 with C++ tools was not discovered. Use -VisualStudioPath for an explicit installation.'
}

$pwsh = Get-Command pwsh.exe -ErrorAction SilentlyContinue
if ($pwsh) {
    Add-Check 'PASS' 'PowerShell 7 command' $pwsh.Source
} else {
    Add-Check 'FAIL' 'PowerShell 7 command' 'pwsh.exe is required by build/resource scripts.'
}

$kits = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue
if ($kits -and $kits.KitsRoot10) {
    $sdk = $kits.KitsRoot10
    Test-File 'Windows SDK headers' (Join-Path $sdk 'Include\10.0.26100.0\um\Windows.h')
    $sdkArch = if ($Platform -eq 'Win32') { 'x86' } else { $Platform.ToLowerInvariant() }
    Test-File 'Windows SDK libraries' (Join-Path $sdk "Lib\10.0.26100.0\um\$sdkArch\kernel32.lib")
    Test-File 'Shader compiler' (Join-Path $sdk 'bin\10.0.26100.0\x64\fxc.exe')
    Test-File 'Resource index tool' (Join-Path $sdk 'bin\10.0.26100.0\x64\makepri.exe')
    Add-Check 'INFO' 'SDK servicing version' 'README requires 10.0.26100.8249 or newer; the 10.0.26100.0 folder name alone does not establish the servicing revision.'
} else {
    Add-Check 'FAIL' 'Windows SDK' 'KitsRoot10 was not found in the Windows Kits registry key.'
}

Test-File '.NET Framework 4.7.2 reference assemblies (tests)' (Join-Path ${env:ProgramFiles(x86)} 'Reference Assemblies\Microsoft\Framework\.NETFramework\v4.7.2\mscorlib.dll') 'WARN'
foreach ($project in @('src\cascadia\WpfTerminalControl', 'src\tools\TerminalStress', 'src\cascadia\WpfTerminalTestNetCore')) {
    Test-File "NuGet assets: $project" (Join-Path $root "$project\obj\project.assets.json") 'WARN'
}
Add-Check 'INFO' 'Restore scope' 'Assets presence does not validate package versions. Full solution includes .NET/test projects; NuGet.Config uses only TerminalDependencies. Historical restore failures required a one-off additional nuget.org source.'

$out = Join-Path $root "bin\$Platform\$Configuration\WindowsTerminal"
foreach ($name in @('WindowsTerminal.exe', 'TerminalApp.dll', 'Microsoft.Terminal.Control.dll', 'Microsoft.Terminal.Settings.Model.dll', 'Microsoft.Terminal.Settings.Editor.dll', 'Microsoft.UI.Xaml.dll', 'resources.pri', 'OpenConsole.exe')) {
    Test-File "Unpackaged layout: $name" (Join-Path $out $name) 'WARN'
}
Add-Check 'INFO' 'Unpackaged layout' 'Command-line Build does not automatically run _WTPrepareUnpackagedLayoutForRun. Missing merged resources.pri or Microsoft.UI.Xaml.dll can prevent XAML activation. See doc/terminalEL-status.md.'

$checks
