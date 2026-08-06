# Changelog

All notable changes to this project are documented here. Format based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-08-06

### Added

- Modular source layout (`src/`) with per-module headers
- Full DNS through Tor: `DNSPort 127.0.0.1:5353`, DNAT of UDP/TCP port 53,
  `resolv.conf` backup/restore
- Exit node selection (`--start DE`) with `ExitNodes`/`StrictNodes`
- Real connection verification through Tor with exit IP reporting
- Dedicated `TORGHOSTNG` iptables chain; teardown never flushes the NAT
  table (Docker/VPN rules are safe)
- Automatic rollback on startup failure and on SIGINT/SIGTERM
- `.deb` and `.rpm` packaging with CI autobuild and release upload
- CLI test suite (`make test`) and local package validation script
- Man page (`man 1 torghostng`), CONTRIBUTING.md, SECURITY.md

### Fixed

- `--renew` sent a literal `$(pidof tor)` to `kill` (shell syntax that was
  never expanded); now uses `pkill -HUP -x tor`
- `--check` never displayed the IP (response body was discarded)
- Tor verification connected directly to the API instead of through Tor
- Exit node argument was accepted but ignored
- DNS was redirected to a TCP SOCKS port (UDP 53 → 9050), which broke DNS
- `--stop` flushed the entire NAT table, removing other tools' rules
- Duplicate iptables rules after repeated starts
- All operations silently swallowed failures (no rollback existed)

### Changed

- Exit codes: `0` success, `1` failure, `2` usage error, `130` interrupted
- Makefile uses `pkg-config` for libcurl and `-MMD` dependency tracking
- `make deb` falls back to a self-contained packer when `dpkg-deb` is
  unavailable; `make rpm` uses `--nodeps` for non-RPM distros

## [1.0.0] - 2026

Initial C rewrite of torghost.
