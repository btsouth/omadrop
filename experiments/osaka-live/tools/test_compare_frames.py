#!/usr/bin/env python3
"""Small regression captures for the strict fidelity gate."""
import csv
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from PIL import Image

TOOL = Path(__file__).with_name("compare-frames.py")
FIELDS = ["frame", "seconds", "brightness", "change", "firework_at", "full_show"]

class StrictComparison(unittest.TestCase):
    def test_strict_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            base, candidate = root / "base", root / "candidate"
            base.mkdir(); candidate.mkdir()
            rows = [dict(zip(FIELDS, values)) for values in
                    [("1", ".2", ".5", "0", "-1", "0"),
                     ("2", ".4", ".6", ".1", "-1", "0")]]
            def capture(path, data=rows, pixel=(20, 30, 40)):
                Image.new("RGB", (2, 2), pixel).save(path / "frame.png")
                with (path / "frames.csv").open("w") as stream:
                    writer = csv.DictWriter(stream, fieldnames=FIELDS)
                    writer.writeheader(); writer.writerows(data)
            def run(strict=True):
                return subprocess.run([sys.executable, str(TOOL),
                    *(["--require-identical"] if strict else []),
                    str(base), str(candidate), str(root / "comparison")],
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            capture(base); capture(candidate)
            self.assertEqual(run().returncode, 0)
            capture(candidate, pixel=(21, 30, 40))
            self.assertEqual(run().returncode, 1)
            self.assertEqual(run(False).returncode, 0)
            for field in FIELDS:
                with self.subTest(field=field):
                    changed = [dict(row) for row in rows]
                    changed[0][field] = "99"
                    capture(candidate, changed)
                    self.assertNotEqual(run().returncode, 0)
            capture(candidate, [])
            self.assertNotEqual(run().returncode, 0)

if __name__ == "__main__":
    unittest.main()
