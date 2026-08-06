#!/bin/bash

set -e

echo "TorGhostNG Installation Script"
echo "=============================="

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root"
    exit 1
fi

echo "Checking dependencies..."

if ! command -v gcc &> /dev/null; then
    echo "Error: gcc is not installed"
    exit 1
fi

if ! pkg-config --exists libcurl 2>/dev/null; then
    echo "Error: libcurl development files not found"
    echo "Install with: apt-get install libcurl4-openssl-dev (Debian/Ubuntu)"
    echo "              or: yum install libcurl-devel (RHEL/CentOS)"
    exit 1
fi

echo "Building TorGhostNG..."
make clean
make

echo "Running test suite..."
make test

echo "Installing TorGhostNG..."
make install

echo ""
echo "Installation complete!"
echo "Run 'torghostng --help' for usage information."
