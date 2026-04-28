param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$BuildDir = "",
    [string]$QtRoot = "C:\qt\6.8.0\msvc2022_64",
    [string]$NsisRoot = "C:\Program Files (x86)\NSIS",
    [string]$InstallDir = "C:\Program Files\hdgnss"
)

$ErrorActionPreference = "Stop"

function Get-ProjectVersion {
    param([string]$CMakeFile)

    $projectLine = Select-String -Path $CMakeFile -Pattern 'project\(GnssView VERSION ([0-9]+\.[0-9]+\.[0-9]+)' | Select-Object -First 1
    if (-not $projectLine) {
        throw "Unable to determine project version from $CMakeFile"
    }
    return $projectLine.Matches[0].Groups[1].Value
}

function Get-AppBuildDir {
    param(
        [string]$BuildDir,
        [string]$Configuration
    )

    $candidates = @(
        (Join-Path $BuildDir $Configuration),
        $BuildDir
    )

    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate "GnssView.exe")) {
            return $candidate
        }
    }

    throw "Build output not found. Checked: $($candidates -join ', ')"
}

function Invoke-NativeChecked {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot "build"
}

$appBuildDir = Get-AppBuildDir -BuildDir $BuildDir -Configuration $Configuration
$exePath = Join-Path $appBuildDir "GnssView.exe"
$windeployqt = Join-Path $QtRoot "bin\windeployqt.exe"
$makensis = Join-Path $NsisRoot "makensis.exe"
$stagingDir = Join-Path $BuildDir "installer\stage\$Configuration"
$outputDir = Join-Path $BuildDir "installer\output"
$version = Get-ProjectVersion -CMakeFile (Join-Path $repoRoot "CMakeLists.txt")
$installerScript = Join-Path $repoRoot "installer\GnssView.nsi"
$packageSuffix = if ($Configuration -eq "Release") { "" } else { "-" + $Configuration.ToLowerInvariant() }
$outputExe = Join-Path $outputDir ("GnssView-" + $version + "-windows-x64-setup" + $packageSuffix + ".exe")

if (-not (Test-Path $exePath)) {
    throw "Build output not found: $exePath"
}
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt not found: $windeployqt"
}
if (-not (Test-Path $makensis)) {
    throw "makensis not found: $makensis"
}

if (Test-Path $stagingDir) {
    Remove-Item -LiteralPath $stagingDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Copy-Item -LiteralPath $exePath -Destination $stagingDir
if (Test-Path (Join-Path $appBuildDir "qml")) {
    Copy-Item -LiteralPath (Join-Path $appBuildDir "qml") -Destination $stagingDir -Recurse
}
if (Test-Path (Join-Path $appBuildDir "assets")) {
    Copy-Item -LiteralPath (Join-Path $appBuildDir "assets") -Destination $stagingDir -Recurse
}

$iconIco = Join-Path $stagingDir "assets\gnssview-icon.ico"
$appIco = Join-Path $stagingDir "assets\GnssView.ico"
if (Test-Path (Join-Path $repoRoot "assets\GnssView.ico")) {
    Copy-Item -LiteralPath (Join-Path $repoRoot "assets\GnssView.ico") -Destination $iconIco -Force
    Copy-Item -LiteralPath (Join-Path $repoRoot "assets\GnssView.ico") -Destination $appIco -Force
} elseif (-not (Test-Path $iconIco)) {
    Add-Type -AssemblyName System.Drawing
    $iconSource = Join-Path $stagingDir "assets\GnssView.iconset\icon_256x256.png"
    if (-not (Test-Path $iconSource)) {
        $iconSource = Join-Path $stagingDir "assets\GnssView.iconset\icon_128x128.png"
    }
    if (-not (Test-Path $iconSource)) {
        throw "No icon source found to generate installer icon."
    }
    $bitmap = [System.Drawing.Bitmap]::FromFile($iconSource)
    $icon = [System.Drawing.Icon]::FromHandle($bitmap.GetHicon())
    $stream = [System.IO.File]::Create($iconIco)
    $icon.Save($stream)
    $stream.Dispose()
    $icon.Dispose()
    $bitmap.Dispose()
    Copy-Item -LiteralPath $iconIco -Destination $appIco -Force
}

Invoke-NativeChecked -FilePath $windeployqt -Arguments @(
    "--dir", $stagingDir,
    "--qmldir", (Join-Path $repoRoot "src\ui\qml"),
    "--verbose", "0",
    (Join-Path $stagingDir "GnssView.exe")
)

$makensisArgs = @(
    "/DAPP_NAME=GnssView",
    "/DAPP_VERSION=$version",
    "/DCOMPANY_NAME=hdgnss",
    "/DINSTALL_DIR=$InstallDir",
    "/DSTAGING_DIR=$stagingDir",
    "/DOUTPUT_EXE=$outputExe",
    $installerScript
)

Invoke-NativeChecked -FilePath $makensis -Arguments $makensisArgs

if (-not (Test-Path $outputExe)) {
    throw "Installer was not created: $outputExe"
}

Write-Host "Installer created: $outputExe"
