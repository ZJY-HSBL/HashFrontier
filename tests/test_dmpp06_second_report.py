"""DMPP06 second-level cloud report parser rejects missing, repeated and invalid outcomes."""
import csv
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from report_dmpp06_second_level import audit

class ValidationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture=[]  # Synthetic parser fixture only, never reported as actual experiment results.
        for bits in (6,8):
            for tiles in (16,32,64):
                for n in (4096,8192,16384):
                    for seed in range(1,5):
                        capacity=(1<<bits)*(tiles+tiles//2+tiles//4+tiles//8)
                        accepted=min(n,capacity//2)
                        cls.fixture.append(dict(component='dmpp06_second_tile',bucket_bits=str(bits),first_tiles=str(tiles),
                            keys=str(n),seed=str(seed),levels='4',inserted=str(accepted),overflow=str(n-accepted),
                            erased=str((accepted+3)//4),hit_ops='10000',miss_ops='10000',code_capacity=str(capacity),
                            valid='1',insert_ns_attempt='45.3',hit_ns_op='30.4',miss_ns_op='29.1',erase_ns_op='50.6'))
    def test_complete(self):self.assertEqual(audit(self.fixture),72)
    def test_missing(self):
        with self.assertRaises(ValueError):audit(self.fixture[:-1])
    def test_duplicate(self):
        with self.assertRaises(ValueError):audit(self.fixture+[self.fixture[0]])
    def test_incomplete_insertion(self):
        x=[r.copy() for r in self.fixture]
        x[0]['overflow']='0'
        with self.assertRaises(ValueError):audit(x)
    def test_invalid_timing(self):
        x=[r.copy() for r in self.fixture]
        x[0]['hit_ns_op']='nan'
        with self.assertRaises(ValueError):audit(x)
if __name__=='__main__':unittest.main()
