"""Synthetic regression tests for upstream paired-source instrumentation and audit."""
from pathlib import Path
import sys
import tempfile
import unittest
import csv

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from patch_zombie_paired import patch
from audit_zombie_paired import analyze

class PatchTest(unittest.TestCase):
    def test_anchor_guard_and_idempotence(self):
        original=('\n'.join('RAND_bytes(x);' for _ in range(6))
                  +'\n#include "hm_wrapper.h"\n'
                  +'  generate_load_ops(ops, kv);\n'
                  +'    generate_churn_ops(ops, kv);\n'
                  +'int main(int argc, char **argv) {\n')
        new=patch(original)
        self.assertNotIn('RAND_bytes(', new)
        self.assertEqual(new.count('hf_rand_bytes('),7) # helper + six call sites
        self.assertIn('paired-traces.csv',new)
        with self.assertRaises(ValueError):patch(new)
        with self.assertRaises(ValueError):patch(original.replace('RAND_bytes(x);','RAND_bytes(x);',1).replace('    generate_churn_ops(ops, kv);',''))

class AuditTest(unittest.TestCase):
    def setup_run(self,root, variant, fp='42'):
        d=root/'L85'/'S41'/variant
        d.mkdir(parents=True, exist_ok=True)
        (d/'protocol.txt').write_text('q=13;load=85;cycles=320;deletions=5;insertions=5;lookups=10;unpaired=false;seed=41\n')
        (d/'exitcode.txt').write_text('0\n')
        with (d/'paired-traces.csv').open('w',newline='') as f:
            w=csv.writer(f);w.writerow(('phase','cycle','operations','fnv64'))
            w.writerow(('BUILD',-1,6962,'001'))
            for cyc in range(320):w.writerow(('CHURN',cyc,20,fp if cyc==24 else str(cyc+1000)))
        with (d/'churn_thrput.txt').open('w') as f:
            f.write('churn_cycle  ts  duration   num_ops  false_lookups total_lookups false_delete total_delete total_inserts op\n')
            for cyc in range(320):
                for op,n in [('DELETE',5),('INSERT',5),('LOOKUP',10)]:
                    f.write(f'{cyc} {cyc*100+1} 100 {n} 0 {cyc*10} 0 {cyc*5} {cyc*5} {op}\n')

    def test_matched_and_mismatched_hashes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)/'logs'
            for variant in ('RHM','TRHM','GZHM','GZHM_ADAPTIVE'):
                self.setup_run(root,variant)
            self.assertEqual(analyze(root,Path(tmp)/'audit',loads=(85,),seeds=(41,)),0)
            with (Path(tmp)/'audit'/'complete-cohorts.csv').open() as f:
                self.assertEqual(len(list(csv.DictReader(f))),12)
            self.setup_run(root,'GZHM_ADAPTIVE','DIVERGENT')
            self.assertEqual(analyze(root,Path(tmp)/'audit2',loads=(85,),seeds=(41,)),1)
            self.assertIn('TRACE MISMATCH',(Path(tmp)/'audit2'/'errors.txt').read_text())

if __name__=='__main__':unittest.main()
