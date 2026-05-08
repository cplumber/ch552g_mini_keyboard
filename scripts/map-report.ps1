param(
    [string]$MapPath = (Join-Path $PSScriptRoot '..\build\cli\ch552g_mini_keyboard.ino.map'),
    [string]$MemPath = (Join-Path $PSScriptRoot '..\build\cli\ch552g_mini_keyboard.ino.mem'),
    [int]$Top = 12
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Resolve-ProjectPath {
    param(
        [string]$PathValue
    )

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return (Resolve-Path -LiteralPath $PathValue).Path
    }

    return (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\$PathValue")).Path
}

function Get-SectionKind {
    param(
        [string]$SectionName
    )

    switch -Regex ($SectionName) {
        '^(HOME|CSEG|CONST|XINIT|GSINIT[0-9]*|GSFINAL)$' { return 'flash' }
        '^(XSEG|XISEG|DSEG|OSEG|SSEG|BSEG|ISEG|PSEG)$' { return 'ram' }
        default { return $null }
    }
}

function Format-Bytes {
    param(
        [int]$Value
    )

    if ($Value -ge 1024) {
        return ('{0:N1} KiB' -f ($Value / 1024.0))
    }

    return ('{0} B' -f $Value)
}

$MapPath = Resolve-ProjectPath -PathValue $MapPath
$MemPath = Resolve-ProjectPath -PathValue $MemPath

if (-not (Test-Path -LiteralPath $MapPath)) {
    throw "Map file not found: $MapPath"
}

$sections = @{}
$currentSection = $null

foreach ($line in Get-Content -LiteralPath $MapPath) {
    if ($line -match '^(?<section>[A-Z0-9]+)\s+(?<addr>[0-9A-F]{8})\s+(?<size>[0-9A-F]{8})\s+=\s+(?<bytes>\d+)\.\s+bytes') {
        $sectionName = $matches.section
        if (-not $sections.ContainsKey($sectionName)) {
            $sections[$sectionName] = [ordered]@{
                Name = $sectionName
                Kind = Get-SectionKind -SectionName $sectionName
                Size = [int]$matches.bytes
                Start = [Convert]::ToInt32($matches.addr, 16)
                End = [Convert]::ToInt32($matches.addr, 16) + [Convert]::ToInt32($matches.size, 16)
                Symbols = New-Object System.Collections.Generic.List[object]
            }
        }
        else {
            $section = $sections[$sectionName]
            $section.Size = [int]$matches.bytes
            $section.Start = [Convert]::ToInt32($matches.addr, 16)
            $section.End = [Convert]::ToInt32($matches.addr, 16) + [Convert]::ToInt32($matches.size, 16)
        }

        $currentSection = $sectionName
        continue
    }

    if ($null -ne $currentSection -and $line -match '^(?<kind>[CDIBP]):\s+(?<addr>[0-9A-F]{8})\s+(?<symbol>\S+)(?:\s+(?<module>\S+))?\s*$') {
        $module = if ($matches.ContainsKey('module')) { $matches['module'] } else { $null }
        if ([string]::IsNullOrWhiteSpace($module)) {
            $module = '[unattributed]'
        }

        $sections[$currentSection].Symbols.Add([pscustomobject]@{
            Address = [Convert]::ToInt32($matches.addr, 16)
            Symbol = $matches.symbol
            Module = $module
            Section = $currentSection
            Kind = $sections[$currentSection].Kind
        })
    }
}

$memText = if (Test-Path -LiteralPath $MemPath) {
    Get-Content -LiteralPath $MemPath
} else {
    @()
}

function Get-MemLine {
    param(
        [string[]]$Lines,
        [string]$Pattern
    )

    $match = $Lines | Select-String -Pattern $Pattern | Select-Object -First 1
    if ($match) {
        return $match.Line.Trim()
    }

    return $null
}

Write-Host 'Memory Summary'
if ($memText.Count -gt 0) {
    $flashLine = Get-MemLine -Lines $memText -Pattern '^ *ROM/EPROM/FLASH'
    $xdataLine = Get-MemLine -Lines $memText -Pattern '^ *EXTERNAL RAM'
    $stackLine = Get-MemLine -Lines $memText -Pattern '^Stack starts at:'
    if ($flashLine) { Write-Host "  $flashLine" }
    if ($xdataLine) { Write-Host "  $xdataLine" }
    if ($stackLine) { Write-Host "  $stackLine" }
}
Write-Host ''

Write-Host 'Section Totals'
foreach ($kind in 'ram', 'flash') {
    Write-Host ("  {0}" -f ($kind.ToUpperInvariant()))
    $sections.Values |
        Where-Object { $_.Kind -eq $kind } |
        Sort-Object Size -Descending |
        ForEach-Object {
            Write-Host ("    {0,-8} {1,8}  {2}" -f $_.Name, $_.Size, (Format-Bytes -Value $_.Size))
        }
}
Write-Host ''

function Get-SymbolSizes {
    param(
        [object]$SectionInfo
    )

    $symbols = @($SectionInfo.Symbols | Sort-Object Address)
    $results = New-Object System.Collections.Generic.List[object]

    for ($i = 0; $i -lt $symbols.Count; $i++) {
        $current = $symbols[$i]
        if ($i -lt ($symbols.Count - 1)) {
            $size = $symbols[$i + 1].Address - $current.Address
        }
        else {
            $size = $SectionInfo.End - $current.Address
        }

        if ($size -lt 0) {
            $size = 0
        }

        $results.Add([pscustomobject]@{
            Address = $current.Address
            Size = $size
            Symbol = $current.Symbol
            Module = $current.Module
            Section = $current.Section
            Kind = $current.Kind
        })
    }

    return $results
}

$symbolRows = foreach ($section in $sections.Values) {
    Get-SymbolSizes -SectionInfo $section
}

function Show-TopGroup {
    param(
        [string]$Title,
        [object[]]$Rows,
        [int]$Limit
    )

    Write-Host $Title
    $Rows |
        Sort-Object Size -Descending |
        Select-Object -First $Limit |
        ForEach-Object {
            Write-Host ("  {0,-26} {1,8}  {2}  ({3})" -f $_.Module, $_.Size, (Format-Bytes -Value $_.Size), $_.Symbol)
        }
    Write-Host ''
}

$ramRows = $symbolRows | Where-Object { $_.Kind -eq 'ram' }
$flashRows = $symbolRows | Where-Object { $_.Kind -eq 'flash' }

Show-TopGroup -Title 'Top RAM Symbols' -Rows $ramRows -Limit $Top
Show-TopGroup -Title 'Top Flash Symbols' -Rows $flashRows -Limit $Top

Write-Host 'Top RAM Modules'
$ramRows |
    Group-Object Module |
    ForEach-Object {
        [pscustomobject]@{
            Module = $_.Name
            Size = ($_.Group | Measure-Object Size -Sum).Sum
        }
    } |
    Sort-Object Size -Descending |
    Select-Object -First $Top |
    ForEach-Object {
        Write-Host ("  {0,-26} {1,8}  {2}" -f $_.Module, $_.Size, (Format-Bytes -Value $_.Size))
    }
Write-Host ''

Write-Host 'Top Flash Modules'
$flashRows |
    Group-Object Module |
    ForEach-Object {
        [pscustomobject]@{
            Module = $_.Name
            Size = ($_.Group | Measure-Object Size -Sum).Sum
        }
    } |
    Sort-Object Size -Descending |
    Select-Object -First $Top |
    ForEach-Object {
        Write-Host ("  {0,-26} {1,8}  {2}" -f $_.Module, $_.Size, (Format-Bytes -Value $_.Size))
    }
