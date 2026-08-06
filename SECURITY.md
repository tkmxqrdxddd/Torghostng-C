# Security Policy

## Scope

TorghostNG modifies system network configuration while running as root.
The scope of this policy is the tool itself: privilege handling, shell
command construction, file handling (`/etc/tor/torrc`, `/etc/resolv.conf`),
and the packaging (`.deb`/`.rpm`).

## Reporting a vulnerability

Do **not** open a public issue. Email the maintainer at
<tkmxqrd@gmail.com> (or open a private advisory via
[GitHub Security Advisories](https://github.com/tkmxqrdxddd/Torghostng-C/security/advisories)).

Please include:

- the affected version,
- a minimal reproduction,
- impact assessment.

## Supported versions

Security fixes are backported to the latest minor release. Versions
older than the last minor are not supported.

## Known limitations (not vulnerabilities)

- The tool is not an anonymity guarantee; see the README FAQ.
- UDP traffic other than DNS, and IPv6 when explicitly re-enabled, bypass
  the proxy.
- `systemd-resolved` can intercept DNS before Tor (documented in the
  README troubleshooting section).
