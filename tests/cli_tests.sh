#!/usr/bin/env bash
# CLI-level tests for torghostng. Safe to run without root:
# no system modifications are performed by the tested commands.

set -u

BIN=${1:-./torghostng}

if [[ ! -x "$BIN" ]]; then
    echo "error: $BIN not found (run 'make' first)"
    exit 1
fi

OUT=$(mktemp)
ERR=$(mktemp)
trap 'rm -f "$OUT" "$ERR"' EXIT

pass=0
fail=0

check() {
    local desc=$1 want=$2
    shift 2
    "$BIN" "$@" >"$OUT" 2>"$ERR"
    local got=$?
    if [[ $got -eq $want ]]; then
        pass=$((pass + 1))
        echo "PASS: $desc"
    else
        fail=$((fail + 1))
        echo "FAIL: $desc (exit $got, expected $want)"
    fi
}

contains() {
    local desc=$1 file=$2 needle=$3
    if grep -q -- "$needle" "$file"; then
        pass=$((pass + 1))
        echo "PASS: $desc"
    else
        fail=$((fail + 1))
        echo "FAIL: $desc (missing '$needle' in $file)"
    fi
}

either() {
    local desc=$1 a=$2 b=$3
    shift 3
    "$BIN" "$@" >"$OUT" 2>"$ERR"
    local got=$?
    if [[ $got -eq $a || $got -eq $b ]]; then
        pass=$((pass + 1))
        echo "PASS: $desc (exit $got)"
    else
        fail=$((fail + 1))
        echo "FAIL: $desc (exit $got, expected $a or $b)"
    fi
}

echo "== argument handling =="
check "--help exits 0" 0 --help
contains "--help prints usage" "$OUT" "Usage:"
check "-h short help exits 0" 0 -h
check "--version exits 0" 0 --version
contains "--version prints name" "$OUT" "TorGhostNG"
check "-v short version exits 0" 0 -v
check "no arguments exits 1" 1
check "unknown option exits 2" 2 --bogus
contains "unknown option reported" "$ERR" "Unknown option"
check "invalid exit node exits 2" 2 --start 123
check "too many arguments exits 2" 2 --start de extra

echo "== runtime (non-destructive) =="
either "--renew terminates (0 or 1)" 0 1 --renew
contains "--renew reports action" "$OUT" "Renewing"
either "--check terminates (0 or 1)" 0 1 --check
contains "--check reports action" "$OUT" "Checking"

echo ""
echo "passed: $pass, failed: $fail"
[[ $fail -eq 0 ]]
