param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Shear", "Tilt", "Clamp", "Drift", "Rift")]
    [string] $Plugin
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$PluginRoot = Join-Path $RepoRoot "plugins\$Plugin"
$Projucer = "D:\Programs\JUCE\Projucer.exe"
$MsBuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64\MSBuild.exe"
$WebView2Version = "1.0.3485.44"
$WebView2FolderName = "Microsoft.Web.WebView2.$WebView2Version"
$PackageRoot = Join-Path $PluginRoot "Builds\VisualStudio2026\packages"
$PackageDir = Join-Path $PackageRoot $WebView2FolderName

if (-not (Test-Path $PackageDir)) {
    $SeedPackage = Join-Path $RepoRoot "plugins\Shear\Builds\VisualStudio2026\packages\$WebView2FolderName"

    if (Test-Path $SeedPackage) {
        New-Item -ItemType Directory -Force -Path $PackageRoot | Out-Null
        Copy-Item -LiteralPath $SeedPackage -Destination $PackageRoot -Recurse -Force
    } else {
        New-Item -ItemType Directory -Force -Path $PackageRoot | Out-Null
        $Nupkg = Join-Path $env:TEMP "$WebView2FolderName.nupkg"
        $Uri = "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/$WebView2Version/microsoft.web.webview2.$WebView2Version.nupkg"
        Invoke-WebRequest -Uri $Uri -OutFile $Nupkg
        Expand-Archive -LiteralPath $Nupkg -DestinationPath $PackageDir -Force
    }
}

Push-Location (Join-Path $PluginRoot "web-ui")
npm run build
Pop-Location

& $Projucer --resave (Join-Path $PluginRoot "$Plugin.jucer")
if ($LASTEXITCODE -ne 0) {
    throw "Projucer resave failed for $Plugin"
}

& $MsBuild (Join-Path $PluginRoot "Builds\VisualStudio2026\$Plugin.sln") /t:"$Plugin - VST3" /p:Configuration=Release /p:Platform=x64 /m
if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed for $Plugin"
}

$Vst3 = Join-Path $PluginRoot "Builds\VisualStudio2026\x64\Release\VST3\$Plugin.vst3"
if (-not (Test-Path $Vst3)) {
    throw "Expected VST3 was not produced: $Vst3"
}

$WebDist = Join-Path $PluginRoot "web-ui\dist"
$BundledWebUi = Join-Path $Vst3 "Contents\Resources\web-ui"
if (Test-Path $BundledWebUi) {
    Remove-Item -LiteralPath $BundledWebUi -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $BundledWebUi | Out-Null
Copy-Item -Path (Join-Path $WebDist "*") -Destination $BundledWebUi -Recurse -Force

Write-Host "Built $Vst3"
