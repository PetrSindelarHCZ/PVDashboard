#!/usr/bin/env python3
"""Prepare validated PVDashboard firmware release assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
from pathlib import Path

MAX_FIRMWARE_SIZE = 1_310_720
VERSION_PATTERN = re.compile(r'^#define\s+FIRMWARE_VERSION\s+"([^"]+)"', re.MULTILINE)


def read_version(header: Path) -> str:
    match = VERSION_PATTERN.search(header.read_text(encoding="utf-8"))
    if not match:
        raise ValueError(f"FIRMWARE_VERSION was not found in {header}")
    return match.group(1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--version-header", type=Path, default=Path("include/Version.h"))
    parser.add_argument("--output", type=Path, default=Path("dist"))
    parser.add_argument("--expected-version", default="")
    parser.add_argument("--platformio-core", default="6.2.0")
    parser.add_argument("--platform", default="platformio/espressif32@7.1.3")
    parser.add_argument("--environment", default="esp32dev")
    args = parser.parse_args()

    version = read_version(args.version_header)
    if args.expected_version and version != args.expected_version:
        raise ValueError(
            f"Firmware version {version} does not match release version {args.expected_version}"
        )

    firmware = args.firmware.read_bytes()
    size = len(firmware)
    if size <= 0 or size > MAX_FIRMWARE_SIZE:
        raise ValueError(
            f"Firmware size {size} is outside the allowed range 1..{MAX_FIRMWARE_SIZE}"
        )

    if firmware[0] != 0xE9:
        raise ValueError("Firmware does not start with the ESP32 image magic byte 0xE9")

    digest = hashlib.sha256(firmware).hexdigest()
    args.output.mkdir(parents=True, exist_ok=True)
    output_firmware = args.output / "firmware.bin"
    shutil.copyfile(args.firmware, output_firmware)
    (args.output / "firmware.bin.sha256").write_text(digest + "\n", encoding="ascii")

    manifest = {
        "schemaVersion": 1,
        "firmware": {
            "name": "firmware.bin",
            "version": version,
            "size": size,
            "maximumSize": MAX_FIRMWARE_SIZE,
            "sha256": digest,
        },
        "build": {
            "environment": args.environment,
            "platform": args.platform,
            "platformioCore": args.platformio_core,
            "gitCommit": os.environ.get("GITHUB_SHA", ""),
        },
    }
    (args.output / "dashboard-manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    print(f"Prepared firmware {version}: {size}/{MAX_FIRMWARE_SIZE} bytes, SHA-256 {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
