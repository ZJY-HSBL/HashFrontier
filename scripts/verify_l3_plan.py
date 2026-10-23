#!/usr/bin/env python3
"""Refuse accidental drift from preregistered HF-Score/1.0."""
import json
from pathlib import Path
def validate(plan,score,host):
    errors=[]
    if plan.get("score_protocol")!="scoring/protocol-v1.0.json":errors.append("wrong score protocol path")
    if plan["reference_algorithm"]!=score["reference"]:errors.append("reference drift")
    if plan["seed_count"]!=len(score["seeds"]) or list(range(plan["seed_start"],plan["seed_start"]+plan["seed_count"]))!=score["seeds"]:errors.append("seed drift")
    if [s["id"] for s in plan["scenarios"]]!=[s["id"] for s in score["scenarios"]]:errors.append("scenario drift")
    for s in plan["scenarios"]:
        if abs(s["read_fraction"]+s["insert_fraction"]+s["delete_fraction"]-1)>1e-12:errors.append("operation fraction drift "+s["id"])
    if plan["key_bits"]!=64 or plan["value_bits"]!=64:errors.append("key/value width drift")
    if set(plan["required_metrics"])!=set(score["metric_weights"]):errors.append("metric drift")
    if plan.get("track")!="cpu_dynamic_exact_dictionary":errors.append("track drift")
    if host["hardware_profile"]!="hf-cpu-epyc9654-256g-v1":errors.append("hardware profile drift")
    if plan["pilot_slots"] in plan["formal_capacity_slots"]:errors.append("pilot/formal capacity mixed")
    if len({s["id"] for s in plan["scenarios"]})!=len(plan["scenarios"]):errors.append("duplicate scenario")
    return errors
def main():
    p=json.loads(Path("benchmarks/experiment-plan-v1.0.json").read_text())
    s=json.loads(Path("scoring/protocol-v1.0.json").read_text())
    h=json.loads(Path("benchmarks/frozen-platform-v1.0.json").read_text())
    errors=validate(p,s,h);print("L3 plan consistent with HF-Score/1.0" if not errors else "\n".join(errors))
    if errors:raise SystemExit(1)
if __name__=="__main__":main()
