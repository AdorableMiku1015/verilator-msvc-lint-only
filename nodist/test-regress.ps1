# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
#
# Run the regression tests this lint-only checkout can satisfy, against the
# local build. Replaces the "make test" workflow that went away with the
# autoconf build.
#
#   nodist/test-regress.ps1                      # every lint-only test
#   nodist/test-regress.ps1 t/t_lint_implicit.py  # selected tests
#   nodist/test-regress.ps1 --dist                # distribution rule tests
#
# Build first, from a Visual Studio x64 developer prompt:
#   cmake --preset win64-release
#   cmake --build --preset win64-release -j

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$env:VERILATOR_ROOT = $root.Replace('\', '/')

# bin/verilator is a Perl wrapper, so Perl must be on PATH; the one shipped
# with MSYS2 is enough.
$msys = 'C:\msys64\usr\bin'
if ((Test-Path $msys) -and (($env:PATH -split ';') -notcontains $msys)) {
    $env:PATH = "$env:PATH;$msys"
}
if (-not (Get-Command perl -ErrorAction SilentlyContinue)) {
    throw "perl was not found on PATH; install MSYS2 or Git for Windows, or set the path to bin/verilator in VERILATOR_BIN"
}

$binary = Join-Path $root 'bin\verilator_bin.exe'
if (-not (Test-Path $binary)) {
    throw "$binary does not exist; build with 'cmake --build --preset win64-release' first"
}

Push-Location (Join-Path $root 'test_regress')
try {
    if ($args.Count -gt 0) {
        & python driver.py @args
    } else {
        & python driver.py --vlt
    }
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
