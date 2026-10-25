"""Small deterministic synthetic fixtures to test the validator, not research data."""
import csv
from pathlib import Path
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from report_zombie_long_v0121 import summarize

class NativeAuditTests(unittest.TestCase):
    def make_logs(self,root,loads=(85,),seeds=(101,),cycles=96,fail_variant=None):
        for load in loads:
            for seed in seeds:
                for variant in ('RHM','TRHM','GZHM','GZHM_ADAPTIVE'):
                    d=root/f'L{load}'/f'S{seed}'/variant
                    d.mkdir(parents=True)
                    (d/'protocol.txt').write_text(f'q=13;load={load};cycles={cycles};deletions=2;insertions=2;lookups=4;unpaired=false;seed={seed}\n')
                    failed=(variant==fail_variant)
                    (d/'exitcode.txt').write_text('139\n' if failed else '0\n')
                    with (d/'churn_thrput.txt').open('w') as f:
                        f.write('churn_cycle  ts  duration   num_ops  false_lookups total_lookups false_delete total_delete total_inserts op\n')
                        for i in range(cycles//2 if failed else cycles):
                            for phase,n in [('DELETE',2),('INSERT',2),('LOOKUP',4)]:
                                f.write(f'{i} {i} 100 {n} 0 0 0 0 0 {phase}\n')
                    with (d/'paired-traces.csv').open('w',newline='') as f:
                        w=csv.writer(f);w.writerow(['phase','cycle','operations','fnv64']);w.writerow(['BUILD',-1,4,'10'])
                        for i in range(cycles//2 if failed else cycles):w.writerow(['CHURN',i,8,str(i+20)])
    def test_complete_and_crash_not_hidden(self):
        with tempfile.TemporaryDirectory() as t:
            r=Path(t);self.make_logs(r/'input',fail_variant='TRHM')
            status=summarize(r/'input',r/'output',loads=(85,),seeds=(101,),cycles=96)
            self.assertEqual(status['expected_runs'],4)
            self.assertEqual(status['complete_runs'],3)
            self.assertEqual(status['failed_or_incomplete_runs'],1)
            self.assertEqual(len(status['integrity_errors']),0)
            with (r/'output'/'summary.csv').open() as f:
                self.assertNotIn('TRHM', {x['variant'] for x in csv.DictReader(f)})
    def test_trace_mismatch_detected(self):
        with tempfile.TemporaryDirectory() as t:
            r=Path(t);self.make_logs(r/'input')
            trace=r/'input'/'L85'/'S101'/'GZHM'/'paired-traces.csv'
            trace.write_text(trace.read_text().replace(',20\n',',999\n'))
            status=summarize(r/'input',r/'output',loads=(85,),seeds=(101,),cycles=96)
            self.assertTrue(any('TRACE MISMATCH' in x for x in status['integrity_errors']))
if __name__=='__main__':unittest.main()
