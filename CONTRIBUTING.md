# Contributing to TorghostNG

Thanks for considering a contribution. This project is small and
deliberately simple; keeping it that way is a goal.

## Getting started

```bash
make            # build
make test       # CLI test suite (no root needed)
./packaging/scripts/test-packages.sh   # package validation
```

Requires `gcc`, `libcurl` development headers, and optionally `dpkg-deb` /
`rpmbuild` for package targets.

## Code style

- C99, compiled with `-Wall -Wextra -pedantic` — zero warnings required.
- One module per concern (`src/`), functions grouped by module with a
  `module_` prefix (`net_`, `tor_`, `check_`).
- All operations that can fail return `int` (`0`/`-1`); callers must
  propagate failures and the orchestrator (`main.c`) must roll back.
- Never interpolate user input into shell commands. Commands passed to
  `run_command()` are fixed strings; user input is validated and written
  to files instead.
- No comments unless they explain a non-obvious decision.

## What to change

- **Bugs first.** If a behavior is wrong, a regression test goes with the
  fix.
- **Behavior changes** (e.g. DNS approach, iptables rules) must be
  documented in the README and mentioned in `CHANGELOG.md`.
- Keep backward compatibility with the existing CLI (`-s/-x/-r/-c/-h/-v`
  and long forms).
- New features need tests in `tests/cli_tests.sh` unless they require root
  (those are covered by manual testing / CI package validation).

## Version bumps

`VERSION` lives in `src/torghostng.h`; the RPM spec mirrors it in
`packaging/torghostng.spec`. `make test-packages` checks they match — keep
them in sync.

## Releasing

Maintainers: bump the version, update `CHANGELOG.md`, then

```bash
git tag v1.1.0
git push origin v1.1.0
```

CI builds the `.deb` and `.rpm`, runs the tests, and attaches both to a
GitHub Release.

## Reporting issues

Include the output of `torghostng --version`, your distribution, and the
relevant command output with `-v`-style logging if available. For security
issues see [SECURITY.md](SECURITY.md).
