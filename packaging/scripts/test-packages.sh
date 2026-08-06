#!/usr/bin/env bash
# Local verification of package builds (CI mirror, runnable without
# dpkg-deb/rpmbuild where possible).
#
# Usage: test-packages.sh [project_dir]
#
# Checks:
#   1. make test            - CLI test suite
#   2. version consistency  - src/torghostng.h vs RPM spec
#   3. make deb + validation - archive structure, control fields,
#                              data layout, no setuid
#   4. make rpm (only if rpmbuild is available)

set -u

cd "${1:-$(cd "$(dirname "$0")/../.." && pwd)}"

fail=0
step() { echo; echo "== $*"; }

check_rc() {
    local desc=$1 rc=$2
    if [[ $rc -eq 0 ]]; then
        echo "PASS: $desc"
    else
        fail=$((fail + 1))
        echo "FAIL: $desc (exit $rc)"
    fi
}

step "CLI test suite"
make test >/dev/null 2>&1
check_rc "make test" $?

step "Version consistency (header vs RPM spec)"
HEADER_V=$(sed -n 's/.*#define VERSION "\([^"]*\)".*/\1/p' src/torghostng.h)
SPEC_V=$(sed -n 's/^%global pkg_version \(.*\)/\1/p' packaging/torghostng.spec)
if [[ "$HEADER_V" == "$SPEC_V" ]]; then
    echo "PASS: version $HEADER_V matches header and RPM spec"
else
    fail=$((fail + 1))
    echo "FAIL: header version $HEADER_V != spec version $SPEC_V"
fi

step "Building .deb package"
make deb >/tmp/tg-deb-build.log 2>&1
check_rc "make deb" $?

DEB=$(ls build/torghostng_*.deb 2>/dev/null | head -1)
if [[ -z "$DEB" ]]; then
    fail=$((fail + 1))
    echo "FAIL: no .deb produced in build/"
else
    echo "PASS: produced $DEB"

    TMP=$(mktemp -d)
    trap 'rm -rf "$TMP"' EXIT

    step "Validating .deb structure"
    bsdtar -xf "$DEB" -C "$TMP" 2>/dev/null
    for member in debian-binary control.tar.gz data.tar.xz; do
        if [[ -f "$TMP/$member" ]]; then
            echo "PASS: archive member $member"
        else
            fail=$((fail + 1))
            echo "FAIL: missing archive member $member"
        fi
    done

    [[ "$(cat "$TMP/debian-binary")" == "2.0" ]] && \
        echo "PASS: debian-binary is format 2.0" || {
        fail=$((fail + 1)); echo "FAIL: debian-binary != 2.0"; }

    step "Validating control file"
    bsdtar -xOf "$TMP/control.tar.gz" ./control >"$TMP/control" 2>/dev/null
    for field in "Package: torghostng" "Version: $HEADER_V" "Architecture: amd64"; do
        grep -q "^$field" "$TMP/control" && echo "PASS: control $field" || {
            fail=$((fail + 1)); echo "FAIL: control missing '$field'"; }
    done
    grep -q "^Depends:" "$TMP/control" && echo "PASS: control has Depends" || {
        fail=$((fail + 1)); echo "FAIL: control missing Depends"; }
    bsdtar -xOf "$TMP/control.tar.gz" ./md5sums >"$TMP/md5sums" 2>/dev/null
    if grep -q "usr/bin/torghostng" "$TMP/md5sums" 2>/dev/null; then
        echo "PASS: md5sums present"
    else
        fail=$((fail + 1))
        echo "FAIL: md5sums missing or empty"
    fi

    step "Validating data payload"
    bsdtar -xf "$TMP/data.tar.xz" -C "$TMP" 2>/dev/null
    for file in usr/bin/torghostng usr/share/man/man1/torghostng.1; do
        if [[ -f "$TMP/$file" ]]; then
            echo "PASS: data contains $file"
        else
            fail=$((fail + 1))
            echo "FAIL: data missing $file"
        fi
    done

    if [[ -x "$TMP/usr/bin/torghostng" ]]; then
        "$TMP/usr/bin/torghostng" --version >/dev/null 2>&1
        check_rc "packaged binary runs" $?
    else
        fail=$((fail + 1))
        echo "FAIL: packaged binary is not executable"
    fi

    if [[ -x "$TMP/usr/bin/torghostng" && -u "$TMP/usr/bin/torghostng" ]]; then
        fail=$((fail + 1))
        echo "FAIL: packaged binary has setuid bit"
    else
        echo "PASS: packaged binary is not setuid"
    fi
fi

step "Building .rpm package"
if command -v rpmbuild >/dev/null 2>&1; then
    make rpm >/tmp/tg-rpm-build.log 2>&1
    check_rc "make rpm" $?
    RPM=$(find build/rpm/RPMS -name '*.rpm' 2>/dev/null | head -1)
    if [[ -n "$RPM" ]]; then
        rpm -qpl "$RPM"
        check_rc "rpm -qpl $RPM" $?
    fi
else
    echo "SKIP: rpmbuild not installed (rpm build is verified in CI on Ubuntu)"
fi

echo
if [[ $fail -eq 0 ]]; then
    echo "All package checks passed."
else
    echo "$fail check(s) failed."
fi
exit $((fail > 0))
