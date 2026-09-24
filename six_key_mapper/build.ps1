param(
    [string]$BuildPath = (Join-Path $PSScriptRoot '..\build\six_key_mapper'),
    [string]$ArduinoCliPath = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
# Keep generated source outside Arduino's output directory: arduino-cli clears
# parts of --build-path before compiling.
$stageRoot = Join-Path $projectRoot 'build\_mapper_stage\six_key_mapper'
$stageParentPath = Join-Path $projectRoot 'build\_mapper_stage'
New-Item -ItemType Directory -Force -Path $stageParentPath | Out-Null
$stageParent = (Resolve-Path $stageParentPath).Path

# The stage is generated on every build. Verify its fixed location before
# clearing it so stale copied sources cannot be compiled on a later build.
$stageRoot = [System.IO.Path]::GetFullPath($stageRoot)
$stagePrefix = $stageParent.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $stageRoot.StartsWith($stagePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing to clear a mapper stage outside build/_mapper_stage.'
}
if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}

# Arduino compiles source files only under the sketch directory.  Stage the
# existing shared CH552 USB support here; the normal 3-key sketch is untouched.
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
# Arduino requires the primary .ino filename to match its containing directory.
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'six_key_mapper.ino') -Destination (Join-Path $stageRoot 'six_key_mapper.ino') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mapper.h') -Destination $stageRoot -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mapper.cpp') -Destination $stageRoot -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mapper_stubs.cpp') -Destination $stageRoot -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mapper_led.h') -Destination $stageRoot -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'mapper_led.cpp') -Destination $stageRoot -Force
New-Item -ItemType Directory -Force -Path (Join-Path $stageRoot 'src') | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\userUsbHidKeyboardMouse') -Destination (Join-Path $stageRoot 'src') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\neo') -Destination (Join-Path $stageRoot 'src') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'neo_config.h') -Destination (Join-Path $stageRoot 'src\neo\config.h') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\util.h') -Destination (Join-Path $stageRoot 'src') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\util.cpp') -Destination (Join-Path $stageRoot 'src') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\macro_config.h') -Destination (Join-Path $stageRoot 'src') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src\led.h') -Destination (Join-Path $stageRoot 'src') -Force

# P1.5 pull-down is deliberate: it preserves the SW2-at-power-up bootloader
# route of the six-key reference PCB.  Do not substitute the main project's
# p36 FQBN for this mapper image.
$fqbn = 'CH55xDuino:mcs51:ch552:clock=16internal,usb_settings=user148,upload_method=usb,bootloader_pin=p15'
& (Join-Path $projectRoot 'scripts\build.ps1') `
    -SketchPath (Join-Path $stageRoot 'six_key_mapper.ino') `
    -BuildPath $BuildPath `
    -Fqbn $fqbn `
    -ArduinoCliPath $ArduinoCliPath
