param(
    [string] $LibraryPath = ".\Examples\orchconductor_library_v1.example.json",
    [string] $SchemaPath = ".\Schemas\orchconductor_library.schema.json"
)

$ErrorActionPreference = "Stop"

$failureCount = 0

function Write-CheckPass {
    param([string] $Message)
    Write-Host "[PASS] $Message" -ForegroundColor Green
}

function Write-CheckFail {
    param([string] $Message)
    $script:failureCount += 1
    Write-Host "[FAIL] $Message" -ForegroundColor Red
}

function Write-CheckInfo {
    param([string] $Message)
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Assert-Equal {
    param(
        [object] $Actual,
        [object] $Expected,
        [string] $Message
    )

    if ($Actual -eq $Expected) {
        Write-CheckPass "$Message ($Actual)"
    } else {
        Write-CheckFail "$Message expected '$Expected' but got '$Actual'"
    }
}

function Test-FileExists {
    param([string] $Path)

    if (Test-Path -Path $Path -PathType Leaf) {
        Write-CheckPass "File exists: $Path"
        return $true
    }

    Write-CheckFail "Missing file: $Path"
    return $false
}

function Read-JsonFile {
    param([string] $Path)

    try {
        $json = Get-Content -Path $Path -Raw | ConvertFrom-Json
        Write-CheckPass "JSON parses: $Path"
        return $json
    } catch {
        Write-CheckFail "JSON parse failed: $Path - $($_.Exception.Message)"
        return $null
    }
}

function Get-PresetValues {
    param([object] $Library)

    $values = @()

    foreach ($sectionName in @("woodwinds", "brass", "percussion", "strings")) {
        foreach ($preset in $Library.sectionPresets.$sectionName) {
            foreach ($value in $preset.values) {
                $values += [ordered]@{
                    presetType = "section"
                    section = $sectionName
                    presetId = $preset.id
                    cc = [int] $value.cc
                    value = [int] $value.value
                }
            }
        }
    }

    foreach ($preset in $Library.combiPresets) {
        foreach ($value in $preset.values) {
            $values += [ordered]@{
                presetType = "combi"
                section = ""
                presetId = $preset.id
                cc = [int] $value.cc
                value = [int] $value.value
            }
        }
    }

    return $values
}

Write-Host ""
Write-Host "OrchConductor Library Validation" -ForegroundColor White
Write-Host "================================" -ForegroundColor White
Write-Host ""

Write-CheckInfo "Library path: $LibraryPath"
Write-CheckInfo "Schema path:  $SchemaPath"
Write-Host ""

$libraryExists = Test-FileExists $LibraryPath
$schemaExists = Test-FileExists $SchemaPath

if (-not $libraryExists -or -not $schemaExists) {
    Write-Host ""
    Write-CheckFail "Required files are missing."
    exit 1
}

$library = Read-JsonFile $LibraryPath
$schema = Read-JsonFile $SchemaPath

if ($null -eq $library -or $null -eq $schema) {
    Write-Host ""
    Write-CheckFail "Cannot continue because one or more JSON files failed to parse."
    exit 1
}

Write-Host ""
Write-Host "Top-level checks" -ForegroundColor White
Write-Host "----------------" -ForegroundColor White

Assert-Equal $library.schema "orchconductor.library" "Library schema identity"
Assert-Equal $library.schemaVersion 1 "Library schema version"
Assert-Equal $schema.'$schema' "https://json-schema.org/draft/2020-12/schema" "Formal schema draft"
Assert-Equal $schema.properties.schema.const "orchconductor.library" "Formal schema identity const"
Assert-Equal $schema.properties.schemaVersion.const 1 "Formal schema version const"

Write-Host ""
Write-Host "MIDI checks" -ForegroundColor White
Write-Host "-----------" -ForegroundColor White

Assert-Equal $library.midi.channel 1 "MIDI channel"
Assert-Equal $library.midi.ccRange.min 20 "MIDI CC min"
Assert-Equal $library.midi.ccRange.max 54 "MIDI CC max"

$reserved49 = $library.midi.reservedControllers | Where-Object { $_.cc -eq 49 }
if ($null -ne $reserved49) {
    Write-CheckPass "Reserved controller CC49 is documented"
    Assert-Equal $reserved49.name "Harp" "CC49 reserved name"
    Assert-Equal $reserved49.status "reserved" "CC49 reserved status"
    Assert-Equal $reserved49.defaultValue 0 "CC49 default value"
} else {
    Write-CheckFail "Reserved controller CC49 is not documented"
}

Write-Host ""
Write-Host "Instrument checks" -ForegroundColor White
Write-Host "-----------------" -ForegroundColor White

Assert-Equal $library.instruments.Count 35 "Instrument row count"

$harpInstrument = $library.instruments | Where-Object { $_.cc -eq 49 }
if ($null -ne $harpInstrument) {
    Write-CheckPass "Harp instrument row exists at CC49"
    Assert-Equal $harpInstrument.reserved $true "Harp instrument reserved flag"
} else {
    Write-CheckFail "Harp instrument row at CC49 is missing"
}

$instrumentCcFailures = @()
foreach ($instrument in $library.instruments) {
    if ($instrument.cc -lt 20 -or $instrument.cc -gt 54) {
        $instrumentCcFailures += "$($instrument.id): CC$($instrument.cc)"
    }
}

if ($instrumentCcFailures.Count -eq 0) {
    Write-CheckPass "All instrument CCs are in CC20-CC54"
} else {
    Write-CheckFail "Instrument CCs out of range: $($instrumentCcFailures -join ', ')"
}

Write-Host ""
Write-Host "Player profile checks" -ForegroundColor White
Write-Host "---------------------" -ForegroundColor White

Assert-Equal $library.playerProfile.defaultMaxPlayers 1 "Default max players"
Assert-Equal $library.playerProfile.reservedMaxPlayers 0 "Reserved max players"

$expectedPlayerOverrides = @{
    50 = 8
    51 = 6
    52 = 4
    53 = 4
    54 = 2
}

foreach ($cc in $expectedPlayerOverrides.Keys) {
    $override = $library.playerProfile.overrides | Where-Object { $_.cc -eq [int] $cc }
    if ($null -eq $override) {
        Write-CheckFail "Missing player override for CC$cc"
    } else {
        Assert-Equal $override.maxPlayers $expectedPlayerOverrides[$cc] "Player override CC$cc"
    }
}

Write-Host ""
Write-Host "Preset count checks" -ForegroundColor White
Write-Host "-------------------" -ForegroundColor White

Assert-Equal $library.sectionPresets.woodwinds.Count 20 "Woodwinds preset count"
Assert-Equal $library.sectionPresets.brass.Count 17 "Brass preset count"
Assert-Equal $library.sectionPresets.percussion.Count 9 "Percussion preset count"
Assert-Equal $library.sectionPresets.strings.Count 14 "Strings preset count"
Assert-Equal $library.combiPresets.Count 29 "Combi preset count"

Write-Host ""
Write-Host "Preset value checks" -ForegroundColor White
Write-Host "-------------------" -ForegroundColor White

$allPresetValues = Get-PresetValues $library

$badCcs = $allPresetValues | Where-Object { $_.cc -lt 20 -or $_.cc -gt 54 }
if ($badCcs.Count -eq 0) {
    Write-CheckPass "All preset CCs are in CC20-CC54"
} else {
    Write-CheckFail "Preset CCs out of range: $($badCcs.Count)"
    foreach ($bad in $badCcs) {
        Write-Host "  $($bad.presetId): CC$($bad.cc)" -ForegroundColor Red
    }
}

$badValues = $allPresetValues | Where-Object { $_.value -lt 0 -or $_.value -gt 127 }
if ($badValues.Count -eq 0) {
    Write-CheckPass "All preset values are in 0-127"
} else {
    Write-CheckFail "Preset values out of range: $($badValues.Count)"
    foreach ($bad in $badValues) {
        Write-Host "  $($bad.presetId): CC$($bad.cc) = $($bad.value)" -ForegroundColor Red
    }
}

$cc49Active = $allPresetValues | Where-Object { $_.cc -eq 49 -and $_.value -ne 0 }
if ($cc49Active.Count -eq 0) {
    Write-CheckPass "No preset activates reserved CC49"
} else {
    Write-CheckFail "Reserved CC49 is activated by $($cc49Active.Count) preset value(s)"
    foreach ($bad in $cc49Active) {
        Write-Host "  $($bad.presetId): CC49 = $($bad.value)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Expected partial-value checks" -ForegroundColor White
Write-Host "-----------------------------" -ForegroundColor White

$lowStrings = $library.sectionPresets.strings | Where-Object { $_.id -eq "strings.low_strings" }
if ($null -eq $lowStrings) {
    Write-CheckFail "Missing preset strings.low_strings"
} else {
    $violaPartial = $lowStrings.values | Where-Object { $_.cc -eq 52 }
    if ($null -eq $violaPartial) {
        Write-CheckFail "strings.low_strings missing CC52"
    } else {
        Assert-Equal $violaPartial.value 64 "strings.low_strings CC52 partial value"
    }
}

$englishHornLament = $library.combiPresets | Where-Object { $_.id -eq "solo.english_horn_lament" }
if ($null -eq $englishHornLament) {
    Write-CheckFail "Missing preset solo.english_horn_lament"
} else {
    $bassPartial = $englishHornLament.values | Where-Object { $_.cc -eq 54 }
    if ($null -eq $bassPartial) {
        Write-CheckFail "solo.english_horn_lament missing CC54"
    } else {
        Assert-Equal $bassPartial.value 64 "solo.english_horn_lament CC54 partial value"
    }
}

Write-Host ""
Write-Host "Summary" -ForegroundColor White
Write-Host "-------" -ForegroundColor White

if ($failureCount -eq 0) {
    Write-CheckPass "Validation completed successfully with zero failures."
    exit 0
}

Write-CheckFail "Validation completed with $failureCount failure(s)."
exit 1
