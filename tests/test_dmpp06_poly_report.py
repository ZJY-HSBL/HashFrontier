import csv,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"scripts"))
from report_dmpp06_poly_v023 import validate
class StrictReport(unittest.TestCase):
  def setUp(self):
    t=tempfile.TemporaryDirectory();self.addCleanup(t.cleanup);self.root=Path(t.name)
    self.data=self.root/"raw.csv"
    with self.data.open("w",newline="") as f:
      w=csv.writer(f)
      w.writerow("method,k,bucket_bits,keys,seed,shape,collision_keys,max_bucket,metadata_bytes,ns_per_hash,checksum,valid".split(","))
      for b in (8,10,12):
        for n in (1024,4096):
          for shape in ("contiguous","uniform"):
            for m,k in (("v022_explicit",0),("v023_poly",2),("v023_poly",8),("v023_poly",16)):
              w.writerow((m,k,b,n,1,shape,20,3,1024 if k==0 else 768+k,42.5,1,1))
  def mutate(self,fn):
    with self.data.open(newline="") as f:r=list(csv.reader(f))
    fn(r)
    with self.data.open("w",newline="") as f:csv.writer(f).writerows(r)
    with self.assertRaises(ValueError):validate(self.data,self.root/"bad",seeds=1)
  def test_valid(self):self.assertEqual(validate(self.data,self.root/"good",seeds=1)["observed"],48)
  def test_missing(self):self.mutate(lambda r:r.pop())
  def test_duplicate(self):self.mutate(lambda r:r.append(r[-1]))
  def test_invalid(self):self.mutate(lambda r:r[1].__setitem__(11,"0"))
  def test_bad_memory(self):self.mutate(lambda r:r[1].__setitem__(8,"999"))
