param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Shear", "Tilt", "Clamp", "Drift", "Rift")]
    [string] $Plugin,

    [Parameter(Mandatory = $true)]
    [string] $Version
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$PluginRoot = Join-Path $RepoRoot "plugins\$Plugin"
$Vst3 = Join-Path $PluginRoot "Builds\VisualStudio2026\x64\Release\VST3\$Plugin.vst3"
$WebDist = Join-Path $PluginRoot "web-ui\dist"

if (-not (Test-Path $Vst3) -or -not (Test-Path $WebDist)) {
    & (Join-Path $PSScriptRoot "build-plugin.ps1") -Plugin $Plugin
}

$BundledWebUi = Join-Path $Vst3 "Contents\Resources\web-ui"
if (Test-Path $BundledWebUi) {
    Remove-Item -LiteralPath $BundledWebUi -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $BundledWebUi | Out-Null
Copy-Item -Path (Join-Path $WebDist "*") -Destination $BundledWebUi -Recurse -Force

$ReleaseDir = Join-Path $RepoRoot "releases"
$Stage = Join-Path $ReleaseDir "staging\$Plugin"
$Zip = Join-Path $ReleaseDir "$Plugin-v$Version-windows-vst3.zip"

if (Test-Path $Stage) {
    Remove-Item -LiteralPath $Stage -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $Stage | Out-Null
Copy-Item -LiteralPath $Vst3 -Destination $Stage -Recurse
Copy-Item -LiteralPath (Join-Path $RepoRoot "LICENSE") -Destination (Join-Path $Stage "LICENSE")
Copy-Item -LiteralPath (Join-Path $PluginRoot "README.md") -Destination (Join-Path $Stage "README.md")

if (Test-Path $Zip) {
    Remove-Item -LiteralPath $Zip -Force
}

Compress-Archive -Path (Join-Path $Stage "*") -DestinationPath $Zip
Write-Host "Created $Zip"
