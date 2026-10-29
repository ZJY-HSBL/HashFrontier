import csv,sys,unittest,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from report_siegel_finite_v025 import audit
class AuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows=list(csv.DictReader((Path(__file__).resolve().parents[1]/'results/local-seed-for-tests.csv').open())) if (Path(__file__).resolve().parents[1]/'results/local-seed-for-tests.csv').exists() else None
    def run_case(self,modifier,expect_error):
        from itertools import product
        import math
        header=['component','domain','right_cells','degree','k','seed','search_attempts','certified_subsets','evaluations','hash_ns_per_eval','graph_min_bytes','random_table_bytes','checksum','valid']
        rows=[dict(zip(header,['siegel_lemma2.6_finite',str(n),str(n*2),str(d),'4',str(seed),'1',str(sum(math.comb(n,k) for k in range(1,5))),'120000','1.5',str(n*d*2),str(n*16),'42','1'])) for n,d,seed in product([16,24,32,64],[3,4],range(1,6))]
        modifier(rows)
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'raw.csv'
            with p.open('w',newline='') as f:
                writer=csv.DictWriter(f,fieldnames=header);writer.writeheader();writer.writerows(rows)
            if expect_error:
                with self.assertRaises(ValueError):audit(p)
            else:self.assertEqual(len(audit(p)[0]),40)
    def test_valid(self):self.run_case(lambda r:None,False)
    def test_missing(self):self.run_case(lambda r:r.pop(),True)
    def test_duplicate(self):self.run_case(lambda r:r.__setitem__(1,r[0].copy()),True)
    def test_false_certification(self):self.run_case(lambda r:r[0].__setitem__('certified_subsets','3'),True)
    def test_invalid_time(self):self.run_case(lambda r:r[0].__setitem__('hash_ns_per_eval','nan'),True)
if __name__=='__main__':unittest.main()
