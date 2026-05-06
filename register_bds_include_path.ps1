param(
    [Parameter(Mandatory = $true)]
    [string] $PathToAdd,

    [Parameter(Mandatory = $true)]
    [string] $BdsVersion,

    [switch] $DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Normalize-PathString {
    param([string] $Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return ''
    }

    $trimmed = $Value.Trim().Trim('"')
    $full = [System.IO.Path]::GetFullPath($trimmed)
    return $full.TrimEnd('\')
}

function Split-PathList {
    param([string] $Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return @()
    }

    return $Value.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries) |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -ne '' }
}

function Merge-PathEntry {
    param(
        [string] $ExistingValue,
        [string] $EntryToAdd
    )

    $existingItems = Split-PathList -Value $ExistingValue
    $normalizedEntry = Normalize-PathString -Value $EntryToAdd
    $found = $false

    foreach ($item in $existingItems) {
        if ((Normalize-PathString -Value $item) -ieq $normalizedEntry) {
            $found = $true
            break
        }
    }

    if (-not $found) {
        $existingItems = @($normalizedEntry) + $existingItems
    }

    return @{
        AlreadyPresent = $found
        Value = ($existingItems -join ';')
    }
}

$normalizedPath = Normalize-PathString -Value $PathToAdd
if (-not (Test-Path -LiteralPath $normalizedPath)) {
    throw "Path not found: $normalizedPath"
}

$platforms = @(
    @{ Name = 'Win32'; ValueName = 'IncludePath_Clang32' },
    @{ Name = 'Win64'; ValueName = 'IncludePath' },
    @{ Name = 'Win64x'; ValueName = 'IncludePath' }
)

foreach ($platform in $platforms) {
    $keyPath = "HKCU:\Software\Embarcadero\BDS\$BdsVersion\C++\Paths\$($platform.Name)"
    $valueName = $platform.ValueName

    if (-not (Test-Path -LiteralPath $keyPath)) {
        if ($DryRun) {
            Write-Host "[DRY-RUN] Would create key $keyPath"
        } else {
            New-Item -Path $keyPath -Force | Out-Null
        }
    }

    $existingValue = ''
    if (Test-Path -LiteralPath $keyPath) {
        $item = Get-ItemProperty -LiteralPath $keyPath -ErrorAction SilentlyContinue
        if ($null -ne $item) {
            $candidate = $item.PSObject.Properties[$valueName]
            if ($null -ne $candidate) {
                $existingValue = [string] $candidate.Value
            }
        }
    }

    $merge = Merge-PathEntry -ExistingValue $existingValue -EntryToAdd $normalizedPath
    if ($merge.AlreadyPresent) {
        Write-Host "[OK] $($platform.Name) $valueName already contains $normalizedPath"
        continue
    }

    if ($DryRun) {
        Write-Host "[DRY-RUN] Would set $keyPath :: $valueName"
        Write-Host "          $($merge.Value)"
    } else {
        New-ItemProperty -LiteralPath $keyPath -Name $valueName -Value $merge.Value -PropertyType String -Force | Out-Null
        Write-Host "[OK] Updated $($platform.Name) $valueName"
    }
}
