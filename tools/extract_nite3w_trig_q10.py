#!/usr/bin/env python3
"""
Extract the exact NITE3W 1.10 360-degree Q10 sine/cosine tables used by
FUN_1010_ECAC / FUN_1010_ECF8 into N3D_TRIG_Q10.BIN.

Reference Win16 1.10:
SHA-256 12fe5168783446275802e0e947898261b5eca6b88288f3a895fc1faa4c544481
NE automatic data segment 10 file offset: 0x2C040

Output:
    360 little-endian int16 sine values
    360 little-endian int16 cosine values
Total: 1440 bytes
"""

from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path

EXPECTED_SHA256 = "12fe5168783446275802e0e947898261b5eca6b88288f3a895fc1faa4c544481"
DEFAULT_DATA_SEGMENT_FILE_OFFSET = 0x2C040

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def s16(data: bytes, base: int, off: int) -> int:
    p = base + off
    if p < 0 or p + 2 > len(data):
        raise ValueError(f"read outside EXE at 0x{p:X}")
    return struct.unpack_from("<h", data, p)[0]

def orig_sin_q10(data: bytes, base: int, deg: int) -> int:
    a = deg % 360
    if a < 90:
        return s16(data, base, 0x25E + 2*a)
    if a < 180:
        return s16(data, base, 0x3C6 - 2*a)
    if a < 270:
        return -s16(data, base, 0x0F6 + 2*a)
    return -s16(data, base, 0x52E - 2*a)

def orig_cos_q10(data: bytes, base: int, deg: int) -> int:
    a = deg % 360
    if a < 90:
        return s16(data, base, 0x314 + 2*a)
    if a < 180:
        return -s16(data, base, 0x47C - 2*a)
    if a < 270:
        return -s16(data, base, 0x1AC + 2*a)
    return s16(data, base, 0x5E4 - 2*a)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("output")
    ap.add_argument(
        "--data-segment-file-offset",
        type=lambda s:int(s,0),
        default=DEFAULT_DATA_SEGMENT_FILE_OFFSET)
    ap.add_argument(
        "--allow-other-hash",
        action="store_true",
        help="Allow extraction from an EXE whose SHA-256 is not the checked 1.10 hash.")
    args = ap.parse_args()

    exe_path = Path(args.exe)
    out_path = Path(args.output)
    data = exe_path.read_bytes()
    digest = sha256(data)

    if digest != EXPECTED_SHA256 and not args.allow_other_hash:
        raise SystemExit(
            "EXE hash mismatch.\n"
            f"expected: {EXPECTED_SHA256}\n"
            f"actual  : {digest}\n"
            "Use --allow-other-hash only when you intentionally audited another build.")

    base = args.data_segment_file_offset
    sin_values = [orig_sin_q10(data, base, a) for a in range(360)]
    cos_values = [orig_cos_q10(data, base, a) for a in range(360)]

    checks = {
        "sin_0": sin_values[0],
        "sin_45": sin_values[45],
        "sin_90": sin_values[90],
        "sin_180": sin_values[180],
        "sin_270": sin_values[270],
        "cos_0": cos_values[0],
        "cos_45": cos_values[45],
        "cos_90": cos_values[90],
        "cos_180": cos_values[180],
        "cos_270": cos_values[270],
    }

    if not (
        checks["sin_0"] == 0 and
        checks["sin_45"] == 724 and
        checks["sin_90"] == 1024 and
        checks["sin_180"] == 0 and
        checks["sin_270"] == -1024 and
        checks["cos_0"] == 1024 and
        checks["cos_45"] == 724 and
        checks["cos_90"] == 0 and
        checks["cos_180"] == -1024 and
        checks["cos_270"] == 0
    ):
        raise SystemExit(
            "Extracted trig table failed cardinal/45-degree sanity checks:\n" +
            json.dumps(checks, indent=2))

    payload = struct.pack(
        "<" + "h"*720,
        *(sin_values + cos_values))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(payload)

    manifest = {
        "schema":"n3d-trig-q10-v1",
        "exe":str(exe_path),
        "exe_sha256":digest,
        "data_segment_file_offset":base,
        "output":str(out_path),
        "output_bytes":len(payload),
        "output_sha256":sha256(payload),
        "checks":checks,
    }

    manifest_path = out_path.with_suffix(out_path.suffix + ".json")
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    print(json.dumps(manifest, indent=2))

if __name__ == "__main__":
    main()
