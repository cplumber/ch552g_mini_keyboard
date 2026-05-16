param(
    [string]$SketchPath = (Join-Path $PSScriptRoot '..\ch552g_mini_keyboard.ino'),
    [string]$BuildPath = (Join-Path $PSScriptRoot '..\build\CH55xDuino.mcs51.ch552'),
    [string]$Fqbn = 'CH55xDuino:mcs51:ch552:clock=16internal,usb_settings=user148,upload_method=usb,bootloader_pin=p36',
    [string]$ArduinoCliPath = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Resolve-ArduinoCliPath {
    param(
        [string]$ExplicitPath
    )

    if ($ExplicitPath) {
        if (-not (Test-Path -LiteralPath $ExplicitPath)) {
            throw "arduino-cli not found at '$ExplicitPath'."
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $cmd = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $bundled = 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
    if (Test-Path -LiteralPath $bundled) {
        return $bundled
    }

    throw 'arduino-cli not found. Install Arduino IDE or put arduino-cli on PATH.'
}

function Resolve-ProjectPath {
    param(
        [string]$PathValue
    )

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return (Resolve-Path -LiteralPath $PathValue).Path
    }

    return (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\$PathValue")).Path
}

$cli = Resolve-ArduinoCliPath -ExplicitPath $ArduinoCliPath

$SketchPath = Resolve-ProjectPath -PathValue $SketchPath
$BuildPath = Resolve-ProjectPath -PathValue $BuildPath

if (-not (Test-Path -LiteralPath $SketchPath)) {
    throw "Sketch not found: $SketchPath"
}

New-Item -ItemType Directory -Force -Path $BuildPath | Out-Null

Write-Host "Using arduino-cli: $cli"
Write-Host "Sketch: $SketchPath"
Write-Host "Build path: $BuildPath"
Write-Host "FQBN: $Fqbn"

& $cli compile `
    --fqbn $Fqbn `
    --build-path $BuildPath `
    $SketchPath
