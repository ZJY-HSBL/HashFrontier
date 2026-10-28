import csv,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from report_dmpp06_first_level import analyze
class ReportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.real=ROOT/'results/v0.19.0-dmpp06/raw.csv'
        if not cls.real.exists():cls.real=ROOT/'results/cloud-local.csv'
    def test_full(self):
        with tempfile.TemporaryDirectory() as t:
            r=analyze(self.real,Path(t));self.assertEqual(len(r),18)
    def test_missing(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'truncated.csv';p.write_text('\n'.join(self.real.read_text().splitlines()[:-1])+'\n')
            with self.assertRaisesRegex(ValueError,'missing'):analyze(p,Path(t)/'out')
    def test_duplicate(self):
        with tempfile.TemporaryDirectory() as t:
            lines=self.real.read_text().splitlines();p=Path(t)/'duplicates.csv';p.write_text('\n'.join(lines+[lines[1]])+'\n')
            with self.assertRaisesRegex(ValueError,'duplicate'):analyze(p,Path(t)/'out')
    def test_invalid(self):
        with tempfile.TemporaryDirectory() as t:
            lines=self.real.read_text().splitlines();s=lines[1].split(',');s[-1]='0';lines[1]=','.join(s)
            p=Path(t)/'wrong.csv';p.write_text('\n'.join(lines)+'\n')
            with self.assertRaisesRegex(ValueError,'invalid'):analyze(p,Path(t)/'out')
if __name__=='__main__':unittest.main()
