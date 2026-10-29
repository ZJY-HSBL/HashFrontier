import csv,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from report_siegel_tensor_v026 import audit
COLUMNS=['mode','base_left','base_right','degree','power','k','seed','left_count','right_count','certified_subsets','evaluations','ns_per_op','graph_payload_bytes','expanded_graph_payload_bytes','table_bytes','checksum','valid']

def row(mode,u,d,power,seed):
    from math import comb
    k=4 if power==2 else (3 if u==4 else 2)
    n=u**power;right=(2*u)**power;full=n*d**power*2
    return dict(mode=mode,base_left=u,base_right=2*u,degree=d,power=power,k=k,seed=seed,left_count=n,
      right_count=right,certified_subsets=sum(comb(n,i) for i in range(1,k+1)),evaluations=150000,
      ns_per_op=12,graph_payload_bytes=(u*d*2 if mode=='factorized' else full),
      expanded_graph_payload_bytes=full,table_bytes=right*8,checksum=123456+seed,valid=1)

def dataset():
    return [row(mode,u,d,power,seed)
        for u in (4,6,8) for d in (2,3) for power in (2,3) if not(power==3 and u==8)
        for seed in (1,2,3,4,5) for mode in ('factorized','expanded')]

class AuditTests(unittest.TestCase):
    def checked(self,data,ok):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'raw.csv'
            with p.open('w',newline='') as f:
                w=csv.DictWriter(f,fieldnames=COLUMNS);w.writeheader();w.writerows(data)
            if ok:self.assertEqual(len(audit(p)[1]),50)
            else:
                with self.assertRaises(ValueError):audit(p)
    def test_valid(self):self.checked(dataset(),True)
    def test_missing(self):self.checked(dataset()[:-1],False)
    def test_repeated(self):
        d=dataset();d[2]=d[0].copy();self.checked(d,False)
    def test_mismatch_output(self):
        d=dataset();d[1]['checksum']='121212';self.checked(d,False)
    def test_wrong_certificate(self):
        d=dataset();d[0]['certified_subsets']=1;self.checked(d,False)
    def test_bad_memory(self):
        d=dataset();d[0]['graph_payload_bytes']=0;self.checked(d,False)
    def test_bad_latency(self):
        d=dataset();d[0]['ns_per_op']='nan';self.checked(d,False)
if __name__=='__main__':unittest.main()
