import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'tools' / 'n3d_frame_diff.py'
spec = importlib.util.spec_from_file_location('frame_diff', SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class FrameDiffTests(unittest.TestCase):
    def test_identity(self):
        r = mod.analyze(bytes(64000), bytes(768), bytes(64000), bytes(768))
        self.assertTrue(r['zero_diff'])
        self.assertFalse(r['parity_verified'])

    def test_pixel_location(self):
        a = bytearray(64000)
        a[321] = 9
        a[63999] = 1
        r = mod.analyze(bytes(64000), bytes(768), a, bytes(768))
        self.assertEqual(r['frame']['mismatch_count'], 2)
        self.assertEqual((r['frame']['first_difference']['x'],r['frame']['first_difference']['y']), (1,1))

    def test_palette(self):
        a = bytearray(768)
        a[767] = 255
        r = mod.analyze(bytes(64000), bytes(768), bytes(64000), a)
        self.assertEqual(r['dac']['first_difference']['index'], 255)
        self.assertEqual(r['dac']['first_difference']['channel'], 'B')
        self.assertEqual(r['invalid_rgb6_components']['actual'], 1)

    def test_size_rejected(self):
        with self.assertRaises(ValueError):
            mod.analyze(bytes(63999), bytes(768), bytes(64000), bytes(768))

    def test_cli(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            for n, length in [('ef',64000),('af',64000),('ed',768),('ad',768)]:
                (d/n).write_bytes(bytes(length))
            cmd = [sys.executable,str(SCRIPT), '--expected-frame',str(d/'ef'),
                   '--actual-frame',str(d/'af'),'--expected-dac',str(d/'ed'),
                   '--actual-dac',str(d/'ad'),'--json-out',str(d/'report.json')]
            r = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            self.assertTrue(json.loads((d/'report.json').read_text())['zero_diff'])
            (d/'af').write_bytes(bytes([1])+bytes(63999))
            self.assertEqual(subprocess.run(cmd, capture_output=True).returncode,1)
            (d/'af').write_bytes(b'broken')
            self.assertEqual(subprocess.run(cmd, capture_output=True).returncode,2)

if __name__ == '__main__':
    unittest.main()
