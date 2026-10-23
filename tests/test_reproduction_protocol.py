"""12 fail-closed tests for frozen L3 spec and paper theorem preconditions."""
import json,sys,unittest
from copy import deepcopy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"scripts"))
from verify_l3_plan import validate
from verify_benchmark_host import evaluate
from audit_bubble_theorem import audit
class ProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.p=json.loads((ROOT/"benchmarks/experiment-plan-v1.0.json").read_text())
        cls.h=json.loads((ROOT/"benchmarks/frozen-platform-v1.0.json").read_text())
        cls.s=json.loads((ROOT/"scoring/protocol-v1.0.json").read_text())
    def test_preregistered_plan(self):self.assertEqual([],validate(self.p,self.s,self.h))
    def test_wrong_reference(self):
        p=deepcopy(self.p);p["reference_algorithm"]="other"
        self.assertIn("reference drift",validate(p,self.s,self.h))
    def test_seed_drift(self):
        p=deepcopy(self.p);p["seed_count"]=29
        self.assertIn("seed drift",validate(p,self.s,self.h))
    def test_scenario_drift(self):
        p=deepcopy(self.p);p["scenarios"][0]["id"]="changed"
        self.assertIn("scenario drift",validate(p,self.s,self.h))
    def test_operation_fractions(self):
        p=deepcopy(self.p);p["scenarios"][0]["read_fraction"]=.8
        self.assertTrue(any("operation fraction" in x for x in validate(p,self.s,self.h)))
    def test_metric_removed(self):
        p=deepcopy(self.p);p["required_metrics"].remove("bytes_per_key")
        self.assertIn("metric drift",validate(p,self.s,self.h))
    def test_key_value_width(self):
        p=deepcopy(self.p);p["value_bits"]=1
        self.assertIn("key/value width drift",validate(p,self.s,self.h))
    def test_pilot_parameters_not_theorem(self):
        for lf in [.70,.85,.95]:
            x=audit(4096,1-lf,.5,5,10)
            self.assertFalse(x["all_numeric_necessary"])
            self.assertFalse(x["eligible_to_claim_theorem"])
    def test_theorem_numeric_are_not_a_proof(self):
        x=audit(2**38,.005,.5,5,6)
        self.assertTrue(x["all_numeric_necessary"])
        self.assertFalse(x["eligible_to_claim_theorem"])
    def test_invalid_theorem_parameters(self):
        for epsilon,alpha in [(0,.5),(.05,1)]:
            with self.assertRaises(ValueError):audit(4096,epsilon,alpha,5,10)
    def test_unbound_host_always_fails(self):
        host={"cpu_model":"AMD EPYC 9654","memory_bytes":512*1024**3,"os_release":{"ID":"ubuntu","VERSION_ID":"24.04"},"compiler_version":"13.3.0","cpu_affinity_allowed":[4],"observed_governor":"performance","observed_numa_node":0}
        r=evaluate(self.h,host)
        self.assertFalse(r["formal_host_eligible"])
        self.assertFalse(r["checks"]["host_bound"]["passed"])
    def test_cpu_governor_memory_required(self):
        h=deepcopy(self.h)
        h.update(stage="physically_bound_and_frozen",machine_binding_id="approved-physical-machine",cpu_affinity_logical_id=4,numa_node_id=0,allocator_id="pinned-system-allocator")
        host={"cpu_model":"AMD EPYC 9654","memory_bytes":512*1024**3,"os_release":{"ID":"ubuntu","VERSION_ID":"24.04"},"compiler_version":"13.3.0","cpu_affinity_allowed":[4],"observed_governor":"performance","observed_numa_node":0}
        self.assertTrue(evaluate(h,host)["formal_host_eligible"])
        host["observed_governor"]="powersave"
        self.assertFalse(evaluate(h,host)["formal_host_eligible"])
        host["observed_governor"]="performance";host["memory_bytes"]=64*1024**3
        self.assertFalse(evaluate(h,host)["formal_host_eligible"])
if __name__=="__main__":unittest.main()
