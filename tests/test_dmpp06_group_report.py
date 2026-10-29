import csv
import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from report_dmpp06_group_v024 import FIELDS, load

class StrictAudit(unittest.TestCase):
    def make(self, path, mutate=None):
        with path.open('w', newline='') as f:
            writer = csv.DictWriter(f, FIELDS);writer.writeheader()
            for method in ('transformation3','transformation4'):
                for n in (4096,8192,16384):
                    for bits in (9,10,11,12,13):
                        for dist in ('contiguous','uniform'):
                            for seed in (1,2,3,4,5):
                                row = dict(method=method, keys=n, bucket_bits=bits, distribution=dist, seed=seed,
                                           collision_keys=100, over_threshold_keys=2, max_bucket_load=n, metadata_bytes=1024 if method=='transformation3' else 1536, hash_ns_per_key=19.5, valid=1)
                                if mutate:row=mutate(row)
                                writer.writerow(row)

    def test_complete(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'raw.csv';self.make(p)
            self.assertEqual(len(load(p)),300)

    def test_duplicate(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'raw.csv';self.make(p)
            with p.open('r') as f:content=f.readlines()
            p.write_text(''.join(content[:-1]+[content[-2]]))
            with self.assertRaises(ValueError):load(p)

    def test_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'raw.csv';self.make(p)
            with p.open('r') as f:content=f.readlines()
            p.write_text(''.join(content[:-1]))
            with self.assertRaises(ValueError):load(p)

    def test_bad_metric(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'raw.csv';self.make(p,lambda row:{**row,'hash_ns_per_key':'nan'})
            with self.assertRaises(ValueError):load(p)

    def test_wrong_metadata(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'raw.csv';self.make(p,lambda row:{**row,'metadata_bytes':0})
            with self.assertRaises(ValueError):load(p)

if __name__=='__main__':unittest.main()
