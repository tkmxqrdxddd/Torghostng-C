# TorghostNG

[![CI](https://github.com/tkmxqrdxddd/Torghostng-C/actions/workflows/ci.yml/badge.svg)](https://github.com/tkmxqrdxddd/Torghostng-C/actions/workflows/ci.yml)

TorghostNG is a small command-line tool that routes all TCP traffic of your
machine through the [Tor](https://www.torproject.org/) network and resolves
DNS through Tor as well. It is a complete rewrite in C of the original
[torghost](https://github.com/SusmithKrishnan/torghost) shell script.

It configures the required system state (iptables, `/etc/resolv.conf`,
`/etc/tor/torrc`, IPv6 sysctls), verifies that the connection actually goes
through Tor, and rolls back **every** change automatically on failure or
when stopped. Your other firewall rules are never touched.

## Features

- Redirects all TCP traffic through Tor's SOCKS5 proxy (`127.0.0.1:9050`)
- Full DNS resolution through Tor (`DNSPort`, `resolv.conf`, DNAT rules)
- Optional exit node country selection, e.g. `--start DE`
- Verifies the connection through Tor itself and reports the exit IP
- Dedicated `TORGHOSTNG` iptables chain — idempotent, and removal never
  touches rules owned by Docker, VPNs, or other tools
- Automatic rollback of every change if startup fails or is interrupted
  (SIGINT/SIGTERM)
- Packages for Debian/Ubuntu (`.deb`) and RHEL/Fedora/SUSE (`.rpm`),
  built automatically by CI on every push and attachable to releases
- No root shell scripting in the hot path — commands are fixed strings,
  user input is validated and never interpolated into shell commands

## Requirements

| Requirement | Purpose |
| ----------- | ------- |
| Linux with iptables/nftables (`iptables` userspace tool) | traffic redirection |
| `tor` service installed and enabled | the proxy itself |
| `procps`/`procps-ng` (`pkill`) | circuit renewal |
| `libcurl` | IP/connection checks |
| root | system configuration changes |

The binary itself only needs `libcurl` at runtime; the others are enforced
as package dependencies when installing the `.deb`/`.rpm`.

## Installation

### From a release

Download `torghostng_<version>_<arch>.deb` or `torghostng-<version>.<arch>.rpm`
from the [Releases](https://github.com/tkmxqrdxddd/Torghostng-C/releases)
page and install it:

```bash
# Debian / Ubuntu
sudo apt install ./torghostng_1.1.0_amd64.deb

# RHEL / Fedora / openSUSE
sudo dnf install ./torghostng-1.1.0-1.x86_64.rpm
```

### From source

```bash
git clone https://github.com/tkmxqrdxddd/Torghostng-C
cd Torghostng-C
make            # build
sudo make install
```

Or use `sudo ./install.sh`, which checks dependencies, builds, tests, and
installs.

## Usage

```
torghostng -s [EXIT_NODE]   Start the Tor proxy
torghostng -x               Stop the proxy, restore everything
torghostng -r               Renew the Tor circuit (new exit IP)
torghostng -c               Check Tor connection and print exit IP
torghostng -h | -v          Help / version
```

Long options (`--start`, `--stop`, `--renew`, `--check`, `--help`,
`--version`) are equivalent. `EXIT_NODE` is a 2-letter ISO country code,
case-insensitive (`DE`, `de`, `us`, ...). When set, `ExitNodes {CC}` and
`StrictNodes 1` are added to the Tor configuration.

### Examples

```bash
# Start with a random exit node
sudo torghostng --start

# Start, exit only from Germany
sudo torghostng --start DE

# Verify the circuit, then switch to a fresh one
torghostng --check
sudo torghostng --renew

# Stop and restore all system settings
sudo torghostng --stop
```

Exit codes: `0` success, `1` operation failed, `2` usage error, `130`
interrupted.

## How it works

`--start` performs these steps and rolls everything back if any of them
fails:

1. Disable IPv6 via `sysctl` (Tor can't carry IPv6 exit traffic here).
2. Append a marked block to `/etc/tor/torrc`:
   `DNSPort 127.0.0.1:5353` plus optional `ExitNodes {CC}` / `StrictNodes 1`.
3. Back up `/etc/resolv.conf` in memory and point it at a resolver that is
   DNAT'd into Tor's DNS port.
4. Start the `tor` service.
5. Create the `TORGHOSTNG` chain in the `nat` table:

   | Rule | Effect |
   | ---- | ------ |
   | `-o lo -j RETURN` | leave loopback alone |
   | `--dport 22 -j RETURN` | keep SSH working |
   | `--dport 9050 -j RETURN` | allow Tor's own SOCKS port |
   | `tcp --dport 53 -j DNAT 127.0.0.1:5353` | TCP DNS via Tor |
   | `tcp -j REDIRECT --to-ports 9050` | all other TCP via Tor |
   | `udp --dport 53 -j DNAT 127.0.0.1:5353` | UDP DNS via Tor |

6. Verify through Tor (`check.torproject.org`) and report the exit IP.

`--stop` reverses all of it: restores `resolv.conf`, removes only the
`TORGHOSTNG` chain and its OUTPUT hook, re-enables IPv6, strips the marked
block from `torrc`, and restarts network services. The Tor daemon is left
running (stopping it is a service-management decision, not the tool's).

## Troubleshooting

| Symptom | Likely cause / fix |
| ------- | ------------------ |
| `--check` fails, `curl` can't reach the check API | Tor isn't running or not started by `--start`; start it with `sudo systemctl start tor` |
| DNS fails while the proxy is active | `systemd-resolved`'s stub at `127.0.0.53` intercepts lookups before Tor's DNSPort; `sudo systemctl stop systemd-resolved` (or configure it to forward to `127.0.0.1`) and restart the proxy |
| `iptables` errors on start | the `iptables` userspace tool isn't installed; on nftables-only distros install `iptables` (the nft backend) |
| `--renew` reports failure | `pkill` can't find a process named `tor` — the service isn't running |
| The tool warns "should be run as root" | you're not root; network changes require it |
| Sites still see a real IP | you're connecting over IPv6 (should be disabled by `--start`), or a proxy/VPN config bypasses the iptables rules |

## FAQ

**Is this a guarantee of anonymity?**
No. Tor provides anonymity for the traffic it carries; it does not protect
against malware, browser fingerprinting, or misconfiguration. Prefer the
official [Tor Browser](https://www.torproject.org/) for sensitive work.

**Does it handle UDP other than DNS?**
No. Tor is TCP-based. UDP (games, VoIP, QUIC) is neither redirected nor
dropped, and will still use your real network path. Disable QUIC in your
browser if it matters.

**What if I run it twice?**
`--start` is idempotent — the chain is flushed and rebuilt, the torrc block
replaced, and repeated `--stop` is a no-op.

**What happens on Ctrl+C during `--start`?**
Changes made so far are rolled back automatically.

## Development

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
tests/            CLI test suite (make test)
```

```bash
make            # build (requires libcurl dev headers)
make test       # CLI tests, safe without root
make deb        # build/torghostng_<version>_<arch>.deb
make rpm        # build/rpm/RPMS/... (needs rpmbuild)
./packaging/scripts/test-packages.sh   # full local package validation
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines and
[CHANGELOG.md](CHANGELOG.md) for the release history.

## CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) builds and tests on
every push/PR, builds and validates `.deb` and `.rpm` packages, and — on
`v*` tags — attaches both to a GitHub Release. See
[Security](SECURITY.md) for the disclosure policy.

## License

GPL-3.0. This project is a rewrite of
[torghost](https://github.com/SusmithKrishnan/torghost) by SusmithKrishnan.
