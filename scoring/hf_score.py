#!/usr/bin/env python3
"""HashFrontier unified CPU-dynamic hash table scoring (protocol v1.0).

Strict input completeness, correctness, hardware and trace matching precede any
score computation. Fixture-only inputs never become publication-grade evidence.
This tool never substitutes paper reproduction for actual measurements.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import random
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any

METRICS = (
    "build_ns_per_key", "hit_ns_per_op", "miss_ns_per_op",
    "insert_ns_per_op", "delete_ns_per_op", "update_p99_ns", "bytes_per_key",
)
FIELDS = (
    "algorithm", "scenario", "seed", "track", "hardware_id", "trace_sha256",
    "status", "correct", *METRICS,
)
SHA256 = re.compile(r"[a-f0-9]{64}\Z")
MIN_FORMAL_SEEDS = 30


class ScoreValidationError(ValueError):
    """Raised for an invalid or incompatible scoring dataset."""


@dataclass(frozen=True)
class Observation:
    algorithm: str
    scenario: str
    seed: int
    trace_sha256: str
    status: str
    correct: bool
    metrics: tuple[float, ...] | None


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ScoreValidationError(message)


def read_protocol(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    _require(data.get("score_version") == "HF-Score/1.0", "Unsupported score_version")
    _require(data.get("track") == "cpu_dynamic", "HF-Score v1.0 only supports cpu_dynamic; static/GPU/concurrent require separate protocols")
    _require(data.get("dataset_kind") in {"measurement", "synthetic_fixture"}, "dataset_kind must be measurement or synthetic_fixture")
    _require(isinstance(data.get("hardware_id"), str) and data["hardware_id"].strip(), "hardware_id required")
    algorithms = data.get("algorithms")
    _require(isinstance(algorithms, list) and len(algorithms) >= 2 and
             all(isinstance(a, str) and a for a in algorithms) and len(set(algorithms)) == len(algorithms),
             "algorithms must be distinct nonempty names")
    _require(data.get("reference") in algorithms, "reference absent from algorithms")
    seeds = data.get("seeds")
    _require(isinstance(seeds, list) and seeds and
             all(type(i) is int and i >= 0 for i in seeds) and len(seeds) == len(set(seeds)),
             "seeds must be unique nonnegative integers")
    scenes = data.get("scenarios")
    _require(isinstance(scenes, list) and scenes and
             all(isinstance(s, dict) and isinstance(s.get("id"), str) and s["id"] and
                 isinstance(s.get("weight"), (int, float)) and not isinstance(s.get("weight"), bool) and
                 math.isfinite(s["weight"]) and s["weight"] > 0 for s in scenes) and
             len({s["id"] for s in scenes}) == len(scenes), "scenarios must have unique IDs and finite positive weights")
    _require(math.isclose(sum(s["weight"] for s in scenes), 1.0, rel_tol=0, abs_tol=1e-9), "scenario weights must sum to 1")
    weights = data.get("metric_weights")
    _require(isinstance(weights, dict) and set(weights) == set(METRICS) and
             all(isinstance(v, (int, float)) and not isinstance(v, bool) and
                 math.isfinite(v) and v >= 0 for v in weights.values()) and
             math.isclose(sum(weights.values()), 1.0, rel_tol=0, abs_tol=1e-9),
             "metric_weights must contain all seven metrics, be nonnegative and sum to 1")
    lam = data.get("stability_lambda")
    _require(isinstance(lam, (int, float)) and not isinstance(lam, bool) and
             math.isfinite(lam) and lam >= 0, "stability_lambda must be finite and nonnegative")
    _require(data.get("metric_unit_policy") == "positive_lower_is_better", "Only positive lower-is-better metrics are supported")
    return data


def read_observations(path: Path, protocol: dict[str, Any]) -> dict[tuple[str, str, int], Observation]:
    with path.open(encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        _require(reader.fieldnames is not None and set(reader.fieldnames) == set(FIELDS),
                 f"CSV columns do not match protocol: expected {','.join(FIELDS)}")
        observations: dict[tuple[str, str, int], Observation] = {}
        allowed_algorithms = set(protocol["algorithms"])
        allowed_scenes = {s["id"] for s in protocol["scenarios"]}
        allowed_seeds = set(protocol["seeds"])
        for row_no, row in enumerate(reader, start=2):
            alg, scene = row["algorithm"], row["scenario"]
            _require(alg in allowed_algorithms and scene in allowed_scenes, f"line {row_no}: algorithm/scenario not in preregistration")
            try:
                seed = int(row["seed"])
            except (TypeError, ValueError) as exc:
                raise ScoreValidationError(f"line {row_no}: invalid seed") from exc
            _require(seed in allowed_seeds and str(seed) == row["seed"], f"line {row_no}: unregistered/noncanonical seed")
            key = (alg, scene, seed)
            _require(key not in observations, f"line {row_no}: duplicate observation {key}")
            _require(row["track"] == protocol["track"] and
                     row["hardware_id"] == protocol["hardware_id"], f"line {row_no}: different track or hardware")
            _require(bool(SHA256.fullmatch(row["trace_sha256"])), f"line {row_no}: missing/noncanonical trace SHA-256")
            _require(row["status"] in ("complete", "failed", "timeout", "unsupported", "not_run"), f"line {row_no}: invalid status")
            _require(row["correct"] in ("true", "false"), f"line {row_no}: invalid correctness flag")
            completed = row["status"] == "complete"
            correct = row["correct"] == "true"
            _require(not (completed and not correct), f"line {row_no}: complete with failed correctness")
            if completed:
                try:
                    values = tuple(float(row[k]) for k in METRICS)
                except (ValueError, TypeError) as exc:
                    raise ScoreValidationError(f"line {row_no}: invalid metric") from exc
                _require(all(math.isfinite(v) and v > 0 for v in values), f"line {row_no}: metrics must be finite and > 0")
            else:
                _require(not correct, f"line {row_no}: failed/incomplete row must have correct=false")
                _require(all(not row[k] for k in METRICS), f"line {row_no}: do not provide performance metrics for incomplete runs")
                values = None
            observations[key] = Observation(alg, scene, seed, row["trace_sha256"], row["status"], correct, values)
    _require(observations, "Empty CSV")
    # All reported completed algorithms within the same cohort must have precisely identical trace bytes.
    for scene in allowed_scenes:
        for seed in allowed_seeds:
            sigs = {obs.trace_sha256 for (a, s, k), obs in observations.items()
                    if s == scene and k == seed and obs.status == "complete"}
            _require(len(sigs) <= 1, f"trace mismatch for scenario={scene}, seed={seed}; abort scoring")
    return observations


def _weighted_scenario_log_gain(protocol: dict[str, Any], observations: dict,
                                alg: str, sampled_seeds: list[int] | None = None) -> tuple[float, float, dict[str, float]]:
    reference = protocol["reference"]
    original_seeds = protocol["seeds"]
    seeds = original_seeds if sampled_seeds is None else sampled_seeds
    vals: dict[str, float] = {}
    for scenario in protocol["scenarios"]:
        sid = scenario["id"]
        per_seed = []
        for seed in seeds:
            ours = observations[(alg, sid, seed)].metrics
            baseline = observations[(reference, sid, seed)].metrics
            assert ours is not None and baseline is not None
            per_seed.append(sum(protocol["metric_weights"][k] * math.log(b / a)
                                for k, b, a in zip(METRICS, baseline, ours)))
        vals[sid] = sum(per_seed) / len(per_seed)
    mean = sum(s["weight"] * vals[s["id"]] for s in protocol["scenarios"])
    sigma = math.sqrt(max(0., sum(s["weight"] * (vals[s["id"]] - mean) ** 2
                                  for s in protocol["scenarios"])))
    return mean, sigma, vals


def evaluate(protocol: dict[str, Any], observations: dict, *, bootstrap_repetitions: int = 1000,
             bootstrap_seed: int = 20261022) -> dict[str, Any]:
    _require(type(bootstrap_repetitions) is int and bootstrap_repetitions >= 0, "invalid bootstrap count")
    ref = protocol["reference"]
    expected = [(sid["id"], seed) for sid in protocol["scenarios"] for seed in protocol["seeds"]]
    incomplete: dict[str, list[str]] = {}
    for alg in protocol["algorithms"]:
        errs = []
        for sid, seed in expected:
            obs = observations.get((alg, sid, seed))
            if obs is None:
                errs.append(f"{sid}/{seed}:missing")
            elif obs.status != "complete" or not obs.correct:
                errs.append(f"{sid}/{seed}:{obs.status}/correct={obs.correct}")
        incomplete[alg] = errs
    _require(not incomplete[ref], "Reference is missing/failed: cannot compute any HF-Score: " + "; ".join(incomplete[ref][:5]))
    rng = random.Random(bootstrap_seed)
    n = len(protocol["seeds"])
    formal = n >= MIN_FORMAL_SEEDS and protocol["dataset_kind"] == "measurement"
    entries = []
    for alg in protocol["algorithms"]:
        item: dict[str, Any] = {"algorithm": alg, "eligible": not bool(incomplete[alg]),
                                "missing_or_failed_count": len(incomplete[alg]),
                                "missing_or_failed_examples": incomplete[alg][:10],
                                "score": None, "ci95": None}
        if item["eligible"]:
            mean, sigma, scenes = _weighted_scenario_log_gain(protocol, observations, alg)
            score = 100. * math.exp(mean - protocol["stability_lambda"] * sigma)
            _require(math.isfinite(score), f"score overflow for {alg}")
            item.update({"score": round(score, 6), "mean_log_gain": round(mean, 10),
                         "scenario_log_gain_sd": round(sigma, 10),
                         "scene_log_gains": {s: round(v, 10) for s, v in scenes.items()}})
            if formal and bootstrap_repetitions:
                draws = []
                for _ in range(bootstrap_repetitions):
                    sampled = rng.choices(protocol["seeds"], k=n)
                    mu, sd, _ = _weighted_scenario_log_gain(protocol, observations, alg, sampled)
                    draws.append(100 * math.exp(mu - protocol["stability_lambda"] * sd))
                draws.sort()
                item["ci95"] = [round(draws[int(.025 * (len(draws) - 1))], 6),
                                round(draws[int(.975 * (len(draws) - 1))], 6)]
        entries.append(item)
    label = ("formal_candidate" if formal else "synthetic_fixture_not_evidence" if
             protocol["dataset_kind"] == "synthetic_fixture" else "exploratory_insufficient_seeds")
    return {"score_version": protocol["score_version"], "track": protocol["track"],
            "dataset_kind": protocol["dataset_kind"], "evidence_level": label,
            "reference": ref, "hardware_id": protocol["hardware_id"],
            "num_scenarios": len(protocol["scenarios"]), "num_seeds": n,
            "expected_rows": len(protocol["algorithms"]) * len(expected),
            "observed_rows": len(observations), "metric_weights": protocol["metric_weights"],
            "stability_lambda": protocol["stability_lambda"], "results": entries}


def main() -> int:
    parser = argparse.ArgumentParser(description="Strict seven-metric dynamic hash-table comparison")
    parser.add_argument("--protocol", type=Path, required=True, help="Frozen preregistered scoring JSON")
    parser.add_argument("--observations", type=Path, required=True, help="One row per algorithm/scenario/seed")
    parser.add_argument("--output", type=Path, required=True, help="JSON output path")
    parser.add_argument("--bootstrap-repetitions", type=int, default=1000)
    parser.add_argument("--csv-output", type=Path, help="Optional machine-readable comparison CSV")
    args = parser.parse_args()
    try:
        cfg = read_protocol(args.protocol)
        rows = read_observations(args.observations, cfg)
        results = evaluate(cfg, rows, bootstrap_repetitions=args.bootstrap_repetitions)
    except (ScoreValidationError, json.JSONDecodeError, OSError) as exc:
        parser.exit(2, f"HF-Score refused invalid comparison: {exc}\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    if args.csv_output:
        args.csv_output.parent.mkdir(parents=True, exist_ok=True)
        with args.csv_output.open("w", newline="", encoding="utf-8") as f:
            fields = ("algorithm", "eligible", "evidence_level", "score", "ci95_low", "ci95_high", "missing_or_failed_count")
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writeheader()
            for item in results["results"]:
                lo, hi = item["ci95"] or ("", "")
                writer.writerow(dict(algorithm=item["algorithm"], eligible=str(item["eligible"]).lower(),
                                     evidence_level=results["evidence_level"],
                                     score="" if item["score"] is None else item["score"],
                                     ci95_low=lo, ci95_high=hi,
                                     missing_or_failed_count=item["missing_or_failed_count"]))
    print(f"HF-Score: {results['evidence_level']}; {len(results['results'])} algorithms; "
          f"{results['num_scenarios']} scenarios × {results['num_seeds']} seeds → {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
