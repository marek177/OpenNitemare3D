#!/usr/bin/env python3
"""
Extract renderer lookup tables from the checked NITE3W Win16 1.10 EXE.

Outputs:
- N3D_TRIG_Q10.BIN              1440 bytes
- N3D_VISIBILITY_OCTANTS.BIN      32 bytes
- renderer_tables_manifest.json

Reference EXE SHA-256:
12fe5168783446275802e0e947898261b5eca6b88288f3a895fc1faa4c544481

Automatic data segment file offset:
0x2C040
"""

from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path

EXPECTED_SHA256 = "12fe5168783446275802e0e947898261b5eca6b88288f3a895fc1faa4c544481"
DATA_SEGMENT_FILE_OFFSET = 0x2C040

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def s16(data: bytes, off: int) -> int:
    p = DATA_SEGMENT_FILE_OFFSET + off
    return struct.unpack_from("<h", data, p)[0]

def sin_q10(data: bytes, deg: int) -> int:
    a = deg % 360
    if a < 90:
        return s16(data, 0x25E + 2*a)
    if a < 180:
        return s16(data, 0x3C6 - 2*a)
    if a < 270:
        return -s16(data, 0x0F6 + 2*a)
    return -s16(data, 0x52E - 2*a)

def cos_q10(data: bytes, deg: int) -> int:
    a = deg % 360
    if a < 90:
        return s16(data, 0x314 + 2*a)
    if a < 180:
        return -s16(data, 0x47C - 2*a)
    if a < 270:
        return -s16(data, 0x1AC + 2*a)
    return s16(data, 0x5E4 - 2*a)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("output_directory")
    ap.add_argument("--allow-other-hash", action="store_true")
    args = ap.parse_args()

    exe_path = Path(args.exe)
    output = Path(args.output_directory)
    output.mkdir(parents=True, exist_ok=True)

    data = exe_path.read_bytes()
    digest = sha256(data)

    if digest != EXPECTED_SHA256 and not args.allow_other_hash:
        raise SystemExit(
            "EXE hash mismatch.\n"
            f"expected: {EXPECTED_SHA256}\n"
            f"actual  : {digest}")

    sins = [sin_q10(data, a) for a in range(360)]
    coss = [cos_q10(data, a) for a in range(360)]

    trig = struct.pack("<" + "h"*720, *(sins+coss))

    anchors = {
        "sin_0":sins[0],
        "sin_45":sins[45],
        "sin_90":sins[90],
        "sin_180":sins[180],
        "sin_270":sins[270],
        "cos_0":coss[0],
        "cos_45":coss[45],
        "cos_90":coss[90],
        "cos_180":coss[180],
        "cos_270":coss[270],
    }

    expected = {
        "sin_0":0,
        "sin_45":724,
        "sin_90":1024,
        "sin_180":0,
        "sin_270":-1024,
        "cos_0":1024,
        "cos_45":724,
        "cos_90":0,
        "cos_180":-1024,
        "cos_270":0,
    }

    if anchors != expected:
        raise SystemExit(
            "Q10 anchors failed:\n" +
            json.dumps(anchors, indent=2))

    flag_offsets = [0x4C6,0x4CE,0x4D6,0x4DE]
    vis = bytearray()

    vis_rows = []

    for off in flag_offsets:
        p = DATA_SEGMENT_FILE_OFFSET + off
        row = data[p:p+8]

        if len(row) != 8:
            raise SystemExit(
                f"visibility row at DS:{off:04X} truncated")

        vis.extend(row)
        vis_rows.append(list(row))

    trig_path = output / "N3D_TRIG_Q10.BIN"
    vis_path = output / "N3D_VISIBILITY_OCTANTS.BIN"

    trig_path.write_bytes(trig)
    vis_path.write_bytes(vis)

    manifest = {
        "schema":"n3d-renderer-tables-v1",
        "exe":str(exe_path),
        "exe_sha256":digest,
        "data_segment_file_offset":DATA_SEGMENT_FILE_OFFSET,
        "trig":{
            "path":str(trig_path),
            "bytes":len(trig),
            "sha256":sha256(trig),
            "anchors":anchors,
        },
        "visibility_octants":{
            "path":str(vis_path),
            "bytes":len(vis),
            "sha256":sha256(bytes(vis)),
            "layout":"orientation-major: [ori0 octants0..7][ori1][ori2][ori3]",
            "source_ds_offsets":[
                "04C6","04CE","04D6","04DE"
            ],
            "raw_rows":vis_rows,
        },
    }

    (output/"renderer_tables_manifest.json").write_text(
        json.dumps(manifest, indent=2),
        encoding="utf-8")

    print(json.dumps(manifest, indent=2))

if __name__ == "__main__":
    main()