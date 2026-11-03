#!/usr/bin/env python3
"""Compare supplied 320x200 indexed frames + 768-byte RGB6 DAC snapshots.

Does not capture emulator state or independently prove game parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

def compare(a, b, palette=False):
    first = None
    count = 0
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            count += 1
            if first is None:
                first = {'offset': i, 'expected': x, 'actual': y}
                if palette:
                    first.update(index=i//3, channel='RGB'[i%3])
                else:
                    first.update(x=i%320, y=i//320)
    return {'mismatch_count': count, 'first_difference': first,
            'expected_sha256': hashlib.sha256(a).hexdigest(),
            'actual_sha256': hashlib.sha256(b).hexdigest()}

def analyze(ef, ed, af, ad):
    for label, buf, length in [('expected_frame', ef, 64000), ('expected_dac', ed, 768),
                               ('actual_frame', af, 64000), ('actual_dac', ad, 768)]:
        if len(buf) != length:
            raise ValueError(f'{label}: expected {length} bytes, got {len(buf)}')
    frame, dac = compare(ef, af), compare(ed, ad, True)
    return {'schema': 'n3d-frame-diff-v1', 'source': 'supplied_bytes_only',
            'frame': frame, 'dac': dac,
            'invalid_rgb6_components': {'expected': sum(x>63 for x in ed),
                                        'actual': sum(x>63 for x in ad)},
            'zero_diff': frame['mismatch_count'] == 0 and dac['mismatch_count'] == 0,
            'parity_verified': False,
            'note': 'Zero difference alone does not prove synchronized original/remake execution.'}

def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    for field in ('expected_frame', 'expected_dac', 'actual_frame', 'actual_dac'):
        p.add_argument('--' + field.replace('_','-'), required=True, type=Path)
    p.add_argument('--json-out', type=Path)
    a = p.parse_args(argv)
    try:
        r = analyze(a.expected_frame.read_bytes(), a.expected_dac.read_bytes(),
                    a.actual_frame.read_bytes(), a.actual_dac.read_bytes())
        output = json.dumps(r, indent=2, sort_keys=True) + '\n'
        if a.json_out:
            a.json_out.write_text(output, encoding='utf-8')
        else:
            print(output, end='')
        return 0 if r['zero_diff'] else 1
    except (OSError, ValueError) as e:
        print(f'ERROR: {e}', file=sys.stderr)
        return 2

if __name__ == '__main__':
    sys.exit(main())
