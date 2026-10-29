import csv,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"scripts"))
from report_dmpp06_three_level import audit
class ReportTest(unittest.TestCase):
 def setUp(self):
  self.rows=[]
  for b1 in (4,6):
   for b2 in (4,6):
    for fc in (2,4):
     for tile in (2,4):
      for cap in (64,512):
       for seed in (1,2,3):
        self.rows.append(dict(bucket_bits_first=b1,bucket_bits_second=b2,first_codes=fc,first_tiles=tile,third_capacity=cap,
        seed=seed,attempted=1600,inserted_first=10,inserted_second=20,inserted_third=30,overflow=1540,
        hit_queries=240,miss_queries=4096,updates=15,erases=60,build_ns_per_attempt=1,hit_ns_per_op=2,
        miss_ns_per_op=3,update_ns_per_op=4,erase_ns_per_op=5,valid=1))
 def check(self,rows,valid):
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp)/"data.csv"
   with p.open("w",newline="") as f:
    w=csv.DictWriter(f,fieldnames=list(self.rows[0]));w.writeheader();w.writerows(rows)
   if valid:self.assertEqual(len(audit(p)[1]),32)
   else:self.assertRaises(ValueError,audit,p)
 def test_valid(self):self.check(self.rows,True)
 def test_missing(self):self.check(self.rows[:-1],False)
 def test_duplicate(self):self.check(self.rows[:-1]+[self.rows[0]],False)
 def test_invalid_counts(self):
  self.rows[0]["inserted_third"]=31;self.check(self.rows,False)
 def test_invalid_status(self):
  self.rows[0]["valid"]=0;self.check(self.rows,False)
if __name__=="__main__":unittest.main()
