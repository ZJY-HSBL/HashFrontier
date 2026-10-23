# HashFrontier Benchmark Protocol v1.0 / 统一实验协议

**State: frozen target specification, NOT a physically provisioned machine. No formal L3 or HF-Score has been measured.**

## 唯一总目标 / Single research goal

忠实复现 2025 年 1 月 1 日以来符合筛选标准的高水平散列表算法，在**同一物理机器、同一原始数据、同一操作轨迹、同一评测指标**下进行公平对照。本阶段禁止原创算法、修改论文核心策略或为了提高排名调参。

**Target CPU platform:** one AMD EPYC 9654, >=256 GiB RAM, Ubuntu 24.04, GCC 13.3.0, C11/C++20, `-O3 -DNDEBUG -march=znver4`, one active benchmark thread, pinned logical core, fixed NUMA, performance governor, frozen allocator. See [target JSON](../benchmarks/frozen-platform-v1.0.json). Machine binding ID, core number, NUMA node and allocator are intentionally **NULL** until a real physical host is obtained. Current GitHub and Debian CI runners are **not** formal measurement hardware.

```bash
python3 scripts/verify_benchmark_host.py --out results/current-host.json --require-formal
python3 scripts/verify_l3_plan.py
python3 -m unittest discover -s tests -p 'test_reproduction_protocol.py' -v
```

The host command intentionally fails with code 2 before actual hardware binding. A local host report is only a configuration check, not remote attestation; collect CPU clocks, throttling, microcode, interference, thermal conditions, allocator and compiler fingerprint on the measurement machine.

## Paper-native L2 and shared-trace L3 are distinct

- **L1:** actual paper-algorithm implementation, tests, source or pseudocode mapping, explicit engineering deviations. Classical substitutes may not pass for paper algorithms.
- **L2:** run authors' provided software and reproduce their published settings/figures when the original contains such experiments. **Theory-only papers do not have empirical figure sets to imitate**; their L2 gate instead requires faithful mechanism + mathematical precondition mapping, transparent hash-family assumptions and independently verifiable finite experiments, not an unsupported theorem proof.
- **L3:** CPU dynamic exact dictionary only, same 64-bit keys and 64-bit values, fixed raw serialised traces and matching SHA-256, 30 preregistered paired seeds, seven directly measured metrics, correct oracle results, complete success/failure audit, and a single bound physical host. Static / GPU / concurrent have separate leagues. Algorithms without required semantics remain in a function-limited track, never quietly changed and relabelled as originals.

The already-frozen HF-Score/1.0 weights and scenarios are not modified: reference `robin_hood`, seeds 101–130, `uniform_load_70_read95`, `uniform_load_95_churn50`, `zipf_load_85_read95`, `hotshift_load_95_churn50`. [Exact profile](../benchmarks/experiment-plan-v1.0.json) is checked against [HF-Score scoring contract](../scoring/protocol-v1.0.json). Pilot 8192-slot runs are **exploration only**; planned formal profile currently uses 1,048,576 slots, five warmups, five repetitions, and 100,000 operations per seed/phase. Its actual timestamp collection and complete allocator instrumentation still have to be implemented and verified.

Never combine historical Github-hosted native timing with results from a future frozen physical host. Full raw outputs, incomplete runs, probe tails and memory accounting belong to a versioned evidence release. Any broader workload suite or changes to weights require a new preregistered protocol version.
