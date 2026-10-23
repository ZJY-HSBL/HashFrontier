"""Validate the strict cross-paper report independent of upstream compilation."""
import csv
import importlib.util
import unittest
from pathlib import Path

MODULE=Path(__file__).resolve().parents[1]/'scripts'/'cross_paper_report.py'
spec=importlib.util.spec_from_file_location('cross_paper_report',MODULE)
report=importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)

class CrossPaperAudit(unittest.TestCase):
    def row(self, alg='tpht_flatten', seed=101, sha='a'*64):
        r={'algorithm':alg,'scenario':'load_85','seed':str(seed),'trace_sha256':sha,
           'status':'complete','correct':'true','load_pct':'85','author_reported_memory_bytes':'2048'}
        for k in report.METRICS:r[k]='12.5'
        return r

    def test_valid(self):
        report.verify([self.row('tpht_flatten'),self.row('zombie_gzhm')],[])

    def test_refuse_mismatched_trace(self):
        with self.assertRaisesRegex(ValueError,'UNPAIRED'):
            report.verify([self.row('tpht_flatten',sha='a'*64),self.row('zombie_gzhm',sha='b'*64)],[])

    def test_refuse_duplicate(self):
        with self.assertRaisesRegex(ValueError,'duplicate'):
            report.verify([self.row(),self.row()],[])

    def test_refuse_bad_metric(self):
        r=self.row();r['hit_ns_per_op']='0'
        with self.assertRaisesRegex(ValueError,'invalid metric'):
            report.verify([r],[])

    def test_refuse_failed_claim(self):
        r=self.row();r['correct']='false'
        with self.assertRaisesRegex(ValueError,'non-complete'):
            report.verify([r],[])

    def test_refuse_incomplete_run_masquerade(self):
        r=self.row()
        cohort={'algorithm':'tpht_flatten','scenario':'load_85','seed':'101','trace_sha256':r['trace_sha256'],'status':'failed'}
        with self.assertRaisesRegex(ValueError,'inconsistent'):
            report.verify([r],[cohort])

    def test_summary_contains_only_completed(self):
        s=report.summary([self.row()],[])
        flat=[r for r in s if r['algorithm']=='tpht_flatten' and r['load_pct']==85][0]
        self.assertEqual(flat['completed_seeds'],1)
        self.assertEqual(flat['mean_hit_ns_per_op'],'12.5000')
        other=[r for r in s if r['algorithm']=='zombie_gzhm' and r['load_pct']==85][0]
        self.assertEqual(other['completed_seeds'],0)
        self.assertEqual(other['mean_hit_ns_per_op'],'')

if __name__=='__main__': unittest.main()
