import csv,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from report_dmpp06_column_quotient import report
FIELDS='family,bucket_bits,buckets,keys,distribution,seed,colliding_keys,max_bucket,forward_ns_per_op,inverse_ns_per_op,metadata_bytes,correct'.split(',')
class ReportTests(unittest.TestCase):
 def build(self,p):
  with p.open('w',newline='') as f:
   w=csv.DictWriter(f,fieldnames=FIELDS);w.writeheader()
   for b in (10,11,12):
    for n in (512,1024,2048):
     if n>2**b:continue
     for d in ('contiguous','uniform'):
      w.writerow(dict(family='row_shift_column_affine',bucket_bits=b,buckets=2**b,keys=n,distribution=d,seed=1,colliding_keys=10,max_bucket=2,forward_ns_per_op=3.5,inverse_ns_per_op=4.5,metadata_bytes=1024,correct=1))
 def test_good_and_fail_closed(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td)/'raw.csv';self.build(p)
   self.assertEqual(len(report(p,Path(td)/'good',1)),16)
   content=p.read_text()
   for altered in [content.replace('row_shift_column_affine','bad_family',1),
                   content.replace(',1024,1\n',',1024,0\n',1),
                   content+content.splitlines()[-1]+'\n',
                   '\n'.join(content.splitlines()[:-1])+'\n']:
    p.write_text(altered)
    with self.assertRaises(ValueError):report(p,Path(td)/'bad',1)
   p.write_text(content)
if __name__=='__main__':unittest.main()
