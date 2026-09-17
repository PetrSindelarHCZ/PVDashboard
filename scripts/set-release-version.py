#!/usr/bin/env python3
"""Validate a calendar release tag and write include/Version.h.

Version format: 1.YY.dayOfYear.releaseNumber
Example: 1.26.260.1 for 2026-09-17 (Europe/Prague), first release of the day.
"""

from __future__ import annotations

import argparse
import datetime as dt
import re
import subprocess
from pathlib import Path
from zoneinfo import ZoneInfo

VERSION_RE = re.compile(r"^1\.(\d{2})\.(\d{1,3})\.(\d+)$")


def git_tags(pattern: str) -> list[str]:
    result = subprocess.run(
        ["git", "tag", "--list", pattern],
        check=True,
        text=True,
        capture_output=True,
    )
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tag", required=True, help="Release tag, with or without leading v")
    parser.add_argument("--version-header", type=Path, default=Path("include/Version.h"))
    parser.add_argument("--timezone", default="Europe/Prague")
    args = parser.parse_args()

    version = args.tag[1:] if args.tag.startswith("v") else args.tag
    match = VERSION_RE.fullmatch(version)
    if not match:
        raise ValueError(
            f"Invalid release version {version!r}; expected 1.YY.dayOfYear.releaseNumber"
        )

    yy, day_of_year, release_number = map(int, match.groups())
    if release_number < 1:
        raise ValueError("releaseNumber must be >= 1")

    now = dt.datetime.now(ZoneInfo(args.timezone))
    expected_yy = now.year % 100
    expected_day = now.timetuple().tm_yday

    if yy != expected_yy or day_of_year != expected_day:
        raise ValueError(
            f"Version date 1.{yy:02d}.{day_of_year} does not match "
            f"today in {args.timezone}: 1.{expected_yy:02d}.{expected_day}"
        )

    prefix = f"v1.{yy:02d}.{day_of_year}."
    previous_numbers: list[int] = []
    for tag in git_tags(prefix + "*"):
        tag_version = tag[1:] if tag.startswith("v") else tag
        tag_match = VERSION_RE.fullmatch(tag_version)
        if not tag_match:
            continue
        number = int(tag_match.group(3))
        if tag != args.tag and tag != f"v{version}":
            previous_numbers.append(number)

    expected_release = max(previous_numbers, default=0) + 1
    if release_number != expected_release:
        raise ValueError(
            f"releaseNumber {release_number} is not next for today; expected {expected_release}"
        )

    args.version_header.write_text(
        '#pragma once\n\n'
        '#define FIRMWARE_NAME       "Home Dashboard ESP32"\n'
        f'#define FIRMWARE_VERSION    "{version}"\n'
        '#define FIRMWARE_BUILD_DATE "release " FIRMWARE_VERSION\n',
        encoding="utf-8",
    )

    print(f"Validated release version {version} and updated {args.version_header}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
