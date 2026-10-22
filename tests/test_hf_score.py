"""Deterministic scoring protocol regressions; synthetic inputs only."""
import csv
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / 'scoring' / 'hf_score.py'
spec = importlib.util.spec_from_file_location('hf_score', SCRIPT)
hf = importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name] = hf
spec.loader.exec_module(hf)


class HFScoreTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = {
            'score_version': 'HF-Score/1.0', 'track': 'cpu_dynamic',
            'dataset_kind': 'synthetic_fixture', 'hardware_id': 'synthetic-cpu',
            'reference': 'robin_hood', 'algorithms': ['robin_hood', 'new_method'],
            'metric_unit_policy': 'positive_lower_is_better',
            'metric_weights': {m: w for m, w in zip(hf.METRICS, [.15,.2,.15,.15,.1,.1,.15])},
            'stability_lambda': .25,
            'scenarios': [{'id': 'scenario_a', 'weight': .5}, {'id': 'scenario_b', 'weight': .5}],
            'seeds': [1, 2, 3],
        }
        self.rows = []
        for scenario in ('scenario_a','scenario_b'):
            for seed in (1,2,3):
                sha = ('a' if scenario == 'scenario_a' else 'b') * 64
                for alg in self.config['algorithms']:
                    m = {k: '100.0' for k in hf.METRICS}
                    if alg != 'robin_hood':
                        m = {k: '80.0' for k in hf.METRICS}
                    self.rows.append(dict(algorithm=alg, scenario=scenario, seed=str(seed),
                                          track='cpu_dynamic', hardware_id='synthetic-cpu',
                                          trace_sha256=sha, status='complete', correct='true', **m))

    def compute(self, *, draws=0):
        cfg_path = self.root / 'config.json'
        cfg_path.write_text(json.dumps(self.config), encoding='utf8')
        csv_path = self.root / 'runs.csv'
        with csv_path.open('w', newline='', encoding='utf8') as f:
            writer = csv.DictWriter(f, fieldnames=hf.FIELDS)
            writer.writeheader()
            writer.writerows(self.rows)
        cfg = hf.read_protocol(cfg_path)
        runs = hf.read_observations(csv_path, cfg)
        return hf.evaluate(cfg, runs, bootstrap_repetitions=draws)

    def test_reference_100_improvement_125_and_fixture_not_evidence(self):
        result = self.compute()
        by = {r['algorithm']:r for r in result['results']}
        self.assertEqual(by['robin_hood']['score'], 100.0)
        self.assertEqual(by['new_method']['score'], 125.0)
        self.assertEqual(result['evidence_level'], 'synthetic_fixture_not_evidence')
        self.assertIsNone(by['new_method']['ci95'])

    def test_incomplete_algorithm_is_ineligible(self):
        self.rows = [r for r in self.rows if not (r['algorithm'] == 'new_method' and r['seed'] == '3')]
        result = self.compute()
        new = result['results'][1]
        self.assertFalse(new['eligible'])
        self.assertIsNone(new['score'])
        self.assertEqual(new['missing_or_failed_count'], 2)

    def test_incomplete_reference_rejected(self):
        self.rows = [r for r in self.rows if not (r['algorithm'] == 'robin_hood' and r['seed'] == '1')]
        with self.assertRaisesRegex(hf.ScoreValidationError, 'Reference is missing'):
            self.compute()

    def test_failed_run_excluded(self):
        r = self.rows[-1]
        r['status'],r['correct']='failed','false'
        for m in hf.METRICS:r[m]=''
        output=self.compute()
        self.assertFalse(output['results'][1]['eligible'])

    def test_duplicate_rejected(self):
        self.rows.append(dict(self.rows[0]))
        with self.assertRaisesRegex(hf.ScoreValidationError, 'duplicate'):
            self.compute()

    def test_mismatched_trace_rejected(self):
        self.rows[1]['trace_sha256']='c'*64
        with self.assertRaisesRegex(hf.ScoreValidationError, 'trace mismatch'):
            self.compute()

    def test_mismatched_hardware_rejected(self):
        self.rows[1]['hardware_id']='another-system'
        with self.assertRaisesRegex(hf.ScoreValidationError, 'different track or hardware'):
            self.compute()

    def test_zero_inf_negative_metric_rejected(self):
        for bad in ('0','inf','-1','nan'):
            with self.subTest(bad=bad):
                original=self.rows[1]['update_p99_ns'];self.rows[1]['update_p99_ns']=bad
                with self.assertRaisesRegex(hf.ScoreValidationError, 'finite and > 0'):
                    self.compute()
                self.rows[1]['update_p99_ns']=original

    def test_full_vector_score_has_scenario_stability_penalty(self):
        for r in self.rows:
            if r['algorithm'] == 'new_method' and r['scenario'] == 'scenario_b':
                for key in hf.METRICS:r[key]='100.0'
        output = self.compute()
        score = output['results'][1]['score']
        self.assertGreater(score,100)
        self.assertLess(score,125)
        self.assertGreater(output['results'][1]['scenario_log_gain_sd'],0)

    def test_thirty_real_seeds_allow_bootstrap(self):
        self.config['seeds']=list(range(1,31));self.config['dataset_kind']='measurement'
        self.rows=[]
        for scenario in ('scenario_a','scenario_b'):
            for seed in range(1,31):
                for alg in ('robin_hood','new_method'):
                    self.rows.append(dict(algorithm=alg,scenario=scenario,seed=str(seed),
                        track='cpu_dynamic',hardware_id='synthetic-cpu',
                        trace_sha256=('a' if scenario=='scenario_a' else 'b')*64,
                        status='complete',correct='true',
                        **{k:('80' if alg=='new_method' else '100') for k in hf.METRICS}))
        output=self.compute(draws=80)
        self.assertEqual(output['evidence_level'],'formal_candidate')
        self.assertEqual(output['results'][1]['ci95'],[125.0,125.0])

    def test_protocol_weights_cannot_be_changed_implicitly(self):
        self.config['metric_weights']['hit_ns_per_op']=.21
        with self.assertRaisesRegex(hf.ScoreValidationError,'sum to 1'):
            self.compute()

    def test_wrong_lane_rejected(self):
        self.config['track']='gpu_static'
        with self.assertRaisesRegex(hf.ScoreValidationError,'only supports cpu_dynamic'):
            self.compute()


if __name__=='__main__':unittest.main()
