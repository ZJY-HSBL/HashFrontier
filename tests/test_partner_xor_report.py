import csv
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from report_partner_xor_v018 import analyze,REQUIRED
class Audit(unittest.TestCase):
    def fixture(self,path):
        with path.open('w',newline='') as f:
            w=csv.DictWriter(f,fieldnames=sorted(REQUIRED));w.writeheader()
            for b in (64,128,256):
                for B in (32,64):
                    for s in (1,2,3):
                        w.writerow(dict(bins=b,bin_size=B,ram_words=b*B//4,
                          seed=s,keys_built=min(120,b*B//40),build_ns_per_key=100,
                          hit_ns_per_query=10,update_ns_per_op=100,delete_ns_per_op=120,
                          encoded_array_bytes=b*B*8,external_metadata_lower_bound_bytes=500,
                          successful_rebuilds=150,peel_attempts=150,placement_attempts=150,valid=1))
    def test_full(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'raw.csv';self.fixture(p)
            out=Path(td)/'out';r=analyze(p,out)
            self.assertEqual(len(r),6)
            self.assertIn('NON-DMPP06',(out/'audit.json').read_text())
            self.assertIn('<svg',(out/'update.svg').read_text())
    def test_missing(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'raw.csv';self.fixture(p)
            s=p.read_text().splitlines();p.write_text('\n'.join(s[:-1])+'\n')
            with self.assertRaises(ValueError):analyze(p,Path(td)/'out')
    def test_duplicate(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'raw.csv';self.fixture(p)
            s=p.read_text().splitlines();s[-1]=s[-2];p.write_text('\n'.join(s)+'\n')
            with self.assertRaises(ValueError):analyze(p,Path(td)/'out')
    def test_invalid(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'raw.csv';self.fixture(p)
            s=p.read_text().replace(',1,',',0,',1) # can still be valid if replaces seed; ensure explicit corrupt field instead
            with p.open(newline='') as f:rows=list(csv.DictReader(f));fields=list(rows[0])
            rows[0]['valid']='0'
            with p.open('w',newline='') as f:
                w=csv.DictWriter(f,fieldnames=fields);w.writeheader();w.writerows(rows)
            with self.assertRaises(ValueError):analyze(p,Path(td)/'out')
if __name__=='__main__':unittest.main()
