# Torghostng-C

Remake of torghostng in C.

This project belongs to [SusmithKrishnan](https://github.com/SusmithKrishnan/torghost).
It was rewritten in C, refactored into modules with proper error handling,
rollback on failure, and a safer networking approach.

## Features

- Redirects TCP traffic through Tor (SOCKS5 `127.0.0.1:9050`)
- Full DNS resolution through Tor:
  - `DNSPort` added to `/etc/tor/torrc`
  - UDP/TCP DNS queries DNAT'd to Tor's DNS port
  - `/etc/resolv.conf` backed up and switched to route via Tor
- Optional exit node selection (e.g. `--start DE`)
- Idempotent, self-contained iptables setup via a dedicated `TORGHOSTNG`
  chain — your other firewall rules (Docker, VPN, ...) are never touched
- Automatic rollback of every change if startup fails or is interrupted
- Tor connection verification through Tor itself (`check.torproject.org`),
  including the exit IP

## Dependencies

- `gcc`
- `libcurl-dev` (or `libcurl-devel` on RHEL-based systems)

## Building

```bash
make
```

## Testing

```bash
make test
```

Runs CLI-level tests (argument parsing, exit codes). Safe to run without
root — no system modifications are made.

## Packaging

Build a `.deb` (Debian/Ubuntu) or `.rpm` (RHEL/Fedora/SUSE) package:

```bash
make deb        # -> build/torghostng_<version>_<arch>.deb
make rpm        # -> build/rpm/RPMS/<arch>/torghostng-<version>.rpm (needs rpmbuild)
make dist       # -> build/torghostng-<version>.tar.gz source archive
```

`make deb` uses `dpkg-deb` when available and falls back to a self-contained
packer otherwise, so it works on any distro. Packages ship the binary in
`/usr/bin/`, a man page, and proper runtime dependencies (`tor`, `iptables`,
`procps`, `libcurl`).

Verify package builds locally without root:

```bash
./packaging/scripts/test-packages.sh
```

This checks the CLI suite, version consistency, the `.deb` archive
structure/`control` fields, and runs a full `.rpm` build when `rpmbuild` is
installed.

### CI

`.github/workflows/ci.yml` runs on every push/PR:
- `build` — compile + `make test`
- `deb` — build and validate the `.deb`, uploaded as an artifact
- `rpm` — build and validate the `.rpm`, uploaded as an artifact
- `release` — on `v*` tags, attaches both packages to a GitHub Release

## Installation

### Using install script

```bash
git clone https://github.com/tkmxqrdxddd/Torghostng-C
cd Torghostng-C
sudo chmod +x install.sh
sudo ./install.sh
```

### Using Make

```bash
sudo make install
```

## Usage

| Command            | Description |
| ------------------ | ----------- |
| `torghostng --help`  | Prints usage |
| `torghostng --version` | Display version |
| `torghostng -s`     | Start the Tor proxy |
| `torghostng -s DE`  | Start with German exit node |
| `torghostng -x`     | Stop the Tor proxy and restore all settings |
| `torghostng -r`     | Renew Tor circuit |
| `torghostng -c`     | Check Tor connection and exit IP |

Exit node codes are 2-letter ISO country codes (case-insensitive, e.g. `DE`,
`us`). When set, `ExitNodes` and `StrictNodes 1` are added to the Tor config.

## Examples

```bash
# Start with default exit node
sudo torghostng --start

# Start with specific exit node (Germany)
sudo torghostng --start DE

# Renew Tor circuit
sudo torghostng --renew

# Check connection
torghostng --check

# Stop Tor proxy
sudo torghostng --stop
```

## How it works

`--start` performs the following steps, rolling back everything if any step
fails:

1. Disable IPv6 via `sysctl`
2. Append a `TORGHOSTNG-CONFIG-BEGIN/END` block to `/etc/tor/torrc`
   (`DNSPort 127.0.0.1:5353`, optional `ExitNodes`/`StrictNodes`)
3. Back up `/etc/resolv.conf` and point DNS at a resolver that is DNAT'd
   into Tor
4. Start the `tor` service
5. Create the `TORGHOSTNG` iptables chain in `nat`:
   - `RETURN` for loopback, SSH (port 22) and Tor's own SOCKS port
   - `REDIRECT` remaining TCP to port 9050
   - `DNAT` UDP/TCP port 53 to Tor's `DNSPort`
6. Verify the connection through Tor

`--stop` reverses all changes (restores `resolv.conf`, removes only the
`TORGHOSTNG` chain, re-enables IPv6, strips the torrc block) and restarts
network services. The Tor daemon itself is left running.

## Notes

- Must be run as root for network configuration changes.
- If you use `systemd-resolved`, its stub listener (`127.0.0.53`) may
  intercept DNS before it reaches Tor; stop it or configure it to hand
  DNS off to `127.0.0.1` while the proxy is active.
- iptables `DNAT` requires the `iptables` legacy/nft userspace tools to be
  installed on your distribution.

## Project layout

```
src/
  main.c    CLI parsing, signal handling, orchestration, rollback
  core.c    TorghostNG state and lifecycle
  util.c    logging, command execution, file helpers
  net.c     IPv6, iptables chain, DNS configuration
  tor.c     torrc edits, service control, circuit renewal
  check.c   IP and Tor connection checks (libcurl)
packaging/
  deb/            .deb control template
  torghostng.spec RPM spec file
  man/            man page
  scripts/        self-contained .deb packer + local package tests
tests/      CLI test suite (make test)
```
