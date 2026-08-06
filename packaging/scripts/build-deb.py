#!/usr/bin/env python3
"""Minimal self-contained .deb builder (fallback when dpkg-deb is absent).

Usage: build-deb.py <stage_dir> <output.deb>

Produces a Debian binary package (format 2.0):
  ar archive with members: debian-binary, control.tar.gz, data.tar.xz
"""

import gzip
import hashlib
import io
import os
import stat
import sys
import tarfile
import time


def ar_write(path, members):
    """members: list of (name, bytes)."""
    with open(path, "wb") as fp:
        fp.write(b"!<arch>\n")
        for name, data in members:
            if len(name) > 16:
                raise ValueError("ar member name too long: %r" % name)
            header = (
                name.ljust(16)
                + str(int(time.time())).ljust(12)
                + "0".ljust(6)
                + "0".ljust(6)
                + "100644".ljust(8)
                + str(len(data)).ljust(10)
                + "`\n"
            ).encode("ascii")
            fp.write(header)
            fp.write(data)
            if len(data) % 2 == 1:
                fp.write(b"\n")


def tar_add_all(tar, root, prefix, exclude):
    for dirpath, dirnames, filenames in os.walk(root):
        for name in list(dirnames):
            full = os.path.join(dirpath, name)
            if os.path.relpath(full, root) == exclude:
                dirnames.remove(name)
        for name in filenames:
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, root)
            info = tar.gettarinfo(full, arcname=os.path.join(prefix, rel))
            info.uid = 0
            info.gid = 0
            info.uname = "root"
            info.gname = "root"
            with open(full, "rb") as src:
                tar.addfile(info, src)


def md5sums(stage):
    lines = []
    for dirpath, _, filenames in os.walk(stage):
        for name in filenames:
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, stage)
            if rel.startswith("DEBIAN/"):
                continue
            with open(full, "rb") as fp:
                digest = hashlib.md5(fp.read()).hexdigest()
            lines.append("%s  %s" % (digest, rel))
    return ("\n".join(sorted(lines)) + "\n").encode("ascii")


def build_control_tar(stage):
    with open(os.path.join(stage, "DEBIAN", "control"), "rb") as fp:
        control = fp.read()
    sums = md5sums(stage)

    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:gz", format=tarfile.GNU_FORMAT) as tar:
        info = tarfile.TarInfo("./control")
        info.size = len(control)
        info.mode = 0o644
        info.uid = info.gid = 0
        tar.addfile(info, io.BytesIO(control))
        info = tarfile.TarInfo("./md5sums")
        info.size = len(sums)
        info.mode = 0o644
        info.uid = info.gid = 0
        tar.addfile(info, io.BytesIO(sums))
    return buf.getvalue()


def build_data_tar(stage):
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:xz", format=tarfile.GNU_FORMAT) as tar:
        tar_add_all(tar, stage, ".", exclude="DEBIAN")
    return buf.getvalue()


def main():
    if len(sys.argv) != 3:
        print("usage: build-deb.py <stage_dir> <output.deb>", file=sys.stderr)
        return 2

    stage = sys.argv[1]
    out = sys.argv[2]

    if not os.path.isdir(os.path.join(stage, "DEBIAN")):
        print("error: %s is not a staged package (missing DEBIAN/)" % stage,
              file=sys.stderr)
        return 1

    control_tar = build_control_tar(stage)
    data_tar = build_data_tar(stage)
    ar_write(out, [
        ("debian-binary", b"2.0\n"),
        ("control.tar.gz", control_tar),
        ("data.tar.xz", data_tar),
    ])
    print("built %s (%d bytes)" % (out, os.path.getsize(out)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
