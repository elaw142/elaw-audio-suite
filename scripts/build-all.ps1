$ErrorActionPreference = "Stop"
$Plugins = @("Shear", "Tilt", "Clamp", "Drift", "Rift")

foreach ($Plugin in $Plugins) {
    & (Join-Path $PSScriptRoot "build-plugin.ps1") -Plugin $Plugin
}
