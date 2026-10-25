"""Regression guards for the STOC25 §3.2 pilot's measured-data reporting."""
import csv
import importlib.util
import pathlib
import tempfile
import unittest
from contextlib import redirect_stdout
import io

ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('report_partner',ROOT/'scripts/report_partner_dictionary_section3.py')
report_module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(report_module)

class Audit(unittest.TestCase):
    def example(self):
        rows=[]
        for b in report_module.BINS:
            for B in report_module.BS:
                for seed in range(1,6):
                    rows.append(dict(component='partner_section3_2_oracle',bins=b,bin_size=B,
                       capacity=b*B,initial_keys=b*B*3//4,seed=seed,
                       hit_ns_op=21.3,miss_ns_op=22.3,ram_write_ns_attempt=48.3,
                       delete_ns_attempt=12400,insert_ns_attempt=43.0,write_attempts=400,
                       write_successes=394,delete_attempts=240,delete_successes=240,
                       insert_successes=240,physical_bytes=8*b*B,
                       external_metadata_min_bytes=16*b*B,checksum=2000,valid=1))
        return rows
    def save(self,filename,rows):
        with filename.open('w',newline='') as f:
            writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    def test_complete_and_svg(self):
        with tempfile.TemporaryDirectory() as t:
            d=pathlib.Path(t);self.save(d/'raw.csv',self.example())
            with redirect_stdout(io.StringIO()):s,info=report_module.report(d/'raw.csv',d/'out',d/'fig',5)
            self.assertEqual(len(s),12)
            self.assertEqual(info['operation_counts']['rows'],60)
            self.assertTrue((d/'fig'/'hit_ns_op.svg').read_text().startswith('<svg'))
            self.assertIn('external retrieval unordered_map',str(info))
    def test_duplicate_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            d=pathlib.Path(t);rows=self.example();rows[1]=rows[0].copy();self.save(d/'raw.csv',rows)
            with self.assertRaisesRegex(ValueError,'duplicate'):report_module.load_rows(d/'raw.csv',5)
    def test_failed_correctness_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            d=pathlib.Path(t);rows=self.example();rows[9]['checksum']=1;self.save(d/'raw.csv',rows)
            with self.assertRaisesRegex(ValueError,'incorrect'):report_module.load_rows(d/'raw.csv',5)
    def test_missing_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            d=pathlib.Path(t);rows=self.example()[:-1];self.save(d/'raw.csv',rows)
            with self.assertRaisesRegex(ValueError,'row count'):report_module.load_rows(d/'raw.csv',5)
if __name__=='__main__': unittest.main()
