#Requires -Version 7
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

<#
.SYNOPSIS
    Build terminalEL and prepare its unpackaged runtime layout.
.DESCRIPTION
    Defaults to incremental Debug/x64. Uses VS's developer environment and the
    repository's existing project graph. Stops on the first failed stage and
    retains text/binary logs under obj/terminalEL-build-logs.
    -Plan only prints the stages; it does not restore, build, or create files.
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateRange(1, 64)]
    [int]$Jobs = 4,
    [string]$VisualStudioPath,
    [switch]$Run,
    [switch]$Plan
)

$ErrorActionPreference = 'Stop'
# Inspect native exit codes ourselves, including when the caller enables this preference.
$PSNativeCommandUseErrorActionPreference = $false
$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root "bin\x64\$Configuration\WindowsTerminal"
$exe = Join-Path $output 'WindowsTerminal.exe'

if (!$VisualStudioPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'vswhere.exe not found. Supply -VisualStudioPath.' }
    $VisualStudioPath = & $vswhere -latest -products '*' -version '[18.6,)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or !$VisualStudioPath) { throw 'VS 2026 >= 18.6 with C++ tools was not found.' }
}
$msbuild = Join-Path $VisualStudioPath 'MSBuild\Current\Bin\amd64\MSBuild.exe'
$devShell = Join-Path $VisualStudioPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
foreach ($path in @($msbuild, $devShell)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required tool missing: $path" }
}

$stages = @(
    @{ Name = '01-OpenConsole'; Project = 'src\host\exe\Host.EXE.vcxproj'; Target = 'Build'; Restore = $true },
    @{ Name = '02-WindowsTerminal'; Project = 'src\cascadia\WindowsTerminal\WindowsTerminal.vcxproj'; Target = 'Build'; Restore = $true },
    @{ Name = '03-RuntimeLayout'; Project = 'src\cascadia\WindowsTerminal\WindowsTerminal.vcxproj'; Target = '_WTPrepareUnpackagedLayoutForRun'; Restore = $false }
)
$common = @(
    '/nologo', "/m:$Jobs", '/nr:false', '/v:minimal',
    "/p:Configuration=$Configuration", '/p:Platform=x64',
    "/p:SolutionDir=$root\", '/p:BuildProjectReferences=true',
    '/p:RestorePackagesConfig=true', '/p:MsBuildCacheEnabled=false'
)

Write-Host "Repository: $root"
Write-Host "MSBuild:    $msbuild"
Write-Host "Target:     $Configuration / x64; $Jobs parallel build nodes"
if ($Plan) {
    foreach ($stage in $stages) {
        Write-Host "$($stage.Name): $($stage.Project) /t:$($stage.Target) restore=$($stage.Restore)"
    }
    Write-Host "Verify layout: $output"
    Write-Host "Launch after success: $Run"
    return
}

# Never terminate a shell that might be hosting this build. Only reject a known
# running instance from this exact output directory; installed Terminal is fine.
foreach ($process in @(Get-Process WindowsTerminal -ErrorAction SilentlyContinue)) {
    $processPath = $null
    try { $processPath = $process.Path } catch { }
    if ($processPath -and [string]::Equals($processPath, $exe, [StringComparison]::OrdinalIgnoreCase)) {
        throw "This build's WindowsTerminal.exe is running (PID $($process.Id)). Close it, then run this command from another terminal. No process was terminated."
    }
}

Import-Module $devShell
Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null

$logRoot = Join-Path $root 'obj\terminalEL-build-logs'
$logDir = Join-Path $logRoot (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
Write-Host "Logs: $logDir"

foreach ($stage in $stages) {
    Write-Host "`n=== $($stage.Name) ===" -ForegroundColor Cyan
    $arguments = @((Join-Path $root $stage.Project)) + $common + @(
        "/t:$($stage.Target)",
        "/bl:$logDir\$($stage.Name).binlog;ProjectImports=None",
        "/flp:LogFile=$logDir\$($stage.Name).log;Verbosity=normal;Encoding=UTF-8"
    )
    if ($stage.Restore) { $arguments += '/restore' }
    & $msbuild @arguments
    $stageExitCode = $LASTEXITCODE
    if ($stageExitCode -ne 0) {
        $textLog = Join-Path $logDir "$($stage.Name).log"
        if (Test-Path -LiteralPath $textLog) {
            Write-Host "`nFirst diagnostic errors from $textLog" -ForegroundColor Red
            # Include both English and localized MSBuild/compiler diagnostics.
            Select-String -LiteralPath $textLog -Pattern '(?i)(?:fatal\s+)?error\s+[A-Z]+\d+|错误\s+[A-Z]+\d+' |
                Select-Object -First 12 | ForEach-Object { Write-Host $_.Line }
        }
        throw "$($stage.Name) failed (exit $stageExitCode). Logs: $logDir"
    }
}

$required = @(
    'WindowsTerminal.exe', 'TerminalApp.dll', 'TerminalConnection.dll',
    'Microsoft.Terminal.Control.dll', 'Microsoft.Terminal.Settings.Model.dll',
    'Microsoft.Terminal.Settings.Editor.dll', 'Microsoft.Terminal.UI.dll',
    'Microsoft.Terminal.UI.Markdown.dll', 'Microsoft.UI.Xaml.dll',
    'resources.pri', 'OpenConsole.exe'
)
foreach ($name in $required) {
    $path = Join-Path $output $name
    if (!(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -eq 0) {
        throw "Build finished but runtime layout is incomplete: $path. Logs: $logDir"
    }
}
Write-Host "`nBuild and runtime layout completed: $exe" -ForegroundColor Green
Write-Host "Logs: $logDir"
if ($Run) {
    Start-Process -FilePath $exe -WorkingDirectory $output -ArgumentList '-w', 'new'
}
