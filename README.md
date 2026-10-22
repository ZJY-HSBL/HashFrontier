# HashFrontier

**Hash table construction, collision resolution and reproducible benchmarks**  
**散列表构造、冲突处理与可复现基准研究**

[![Validate and benchmark](https://github.com/ZJY-HSBL/HashFrontier/actions/workflows/ci.yml/badge.svg)](https://github.com/ZJY-HSBL/HashFrontier/actions/workflows/ci.yml) · [Releases](https://github.com/ZJY-HSBL/HashFrontier/releases) · [Paper registry](papers/registry.csv)

## 中文 | 研究目标

通过 **2025-01-01—2026-10-22 高水平论文筛选 → 逐篇正确复现 → 统一负载评测 → 数据驱动机制设计 → 独立确认**，研究在构造耗时、成功/失败查询、动态插删、尾部延迟、空间开销之间具有稳健 Pareto 表现的散列表。核心方法优先不依赖机器学习。

收录范围限与散列表结构或冲突处理直接相关的 Nature / Science / Cell 体系、经年份及学科核实的 SCI Q1/Q2、以及 STOC / FOCS / SODA / SIGMOD / VLDB 等顶会。当前只有 **5 篇已核实的 2025 年论文候选**，并非穷尽检索；全部为 **queued（尚未复现）**。详见 [准入与验证协议](papers/PROTOCOL.md)。

**v0.1.0 = 基础设施与经典对照；v0.2.0 = 长周期 churn、维护尾部开销和 ZombieHT 原版复现准备。两者都不声明已完成 2025–2026 论文复现或原创算法验证。**

### 已实现的经典基线

| Implementation | 构造/插入 | 删除 | 标记 |
|---|---|---|---|
| `linear_tombstone` | 线性探测 | 墓碑标记 | classic baseline |
| `linear_backshift` | 线性探测 | 后移补洞 | classic baseline |
| `robin_hood` | Robin Hood 交换 | 后移补洞 | classic baseline |

三者均使用相同的 SplitMix64 混合函数、固定的 2 的幂容量、同一批构建键及相同操作轨迹。它们是**独立工程实现，不代表任何新论文的精确复现**。

### v0.1.0 本地探索实验

固定容量 **4,096 个槽位**，目标负载 50%、80%、93%；每档使用 5 个配对随机种子，Uniform / Zipf(s=1.1) / hotspot-shift 三类访问分布，每种执行：建表、成功查询、失败查询、1,500 轮插删、插删后成功查询、插删后失败查询。每个查询阶段 8,000 次操作；每档设置都从新表开始。合计 **810 行原始观测**。

![Successful lookup probes](figures/successful_probes.svg)

![Failure probes after churn](figures/post_churn_misses.svg)

![Churn latency](figures/churn_latency.svg)

**代表性数据（Uniform，93% 负载，5 种子均值；探测次数越低越好）：**

| 基线 | 建表后成功查询探测/次 | 插删后失败查询探测/次 |
|---|---:|---:|
| Linear + tombstones | 7.93 | 691.45 |
| Linear + backshift | 7.93 | 80.63 |
| Robin Hood | 7.71 | 7.55 |

墓碑累积可能使长期未命中查询恶化；Robin Hood 的“按位移提前终止”有利于控制失败查询探测。然而 **Robin Hood 并非所有指标最好**：它的维护成本、额外 `home()` 计算和实际时间也必须考核。图表呈现的是这台本地容器的结果，不构成跨硬件的性能结论。

[完整原始 CSV](results/v0.1.0-local.csv) · [分组均值与标准差](results/v0.1.0-summary.csv) · [运行环境](results/v0.1.0-environment.json)

### 一键复现实验

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/hf_benchmark 8000 5 > results/my-run.csv
python -m pip install matplotlib
python scripts/analyze.py results/my-run.csv --out figures
```

**测试要求**：正确性差分测试与 `std::unordered_set` 对照，4 档容量 × 6 种子 × 12,500 次操作 × 3 类实现 = 900,000 次随机操作；Release 编译下依然启用显式断言检查。所有记录由实际可运行代码生成。

**重要限制**：这是单进程 CPU 微基准；包含 8,000 次查询的较短时间片，时钟噪声较大。分组标准差仅描述跨 5 个种子的分散程度，不是置信区间。尚未控制 CPU 频率/NUMA、重复批次、内存分配器、分位延迟与多线程公平性；图表的时间数据不得用于正式论文的性能优越性声明。固定容量实验不包含动态扩容成本。 `allocated_bytes` 仅指槽位向量的底层容量，不包括容器头部/分配器/进程 RSS。多赛道（并发、GPU、静态完美散列）**不会直接放在一个不加限定的榜单**。

### v0.2.0 | 长周期插删与重建代价研究（已真实运行）

新增同容量 `linear_rebuild_2pct`、`linear_rebuild_8pct`、`linear_rebuild_half_slack` 三种 **全表墓碑清理对照机制**（**不是** SIGMOD ZombieHT），加上 v0.1.0 三个经典实现，共六个对照。12 个 churn 周期（每周期随机替换 5% 现存键）、75/85/95% 负载、5 个配对随机种子、每周期 256 次成功查询及 256 次失败查询、全部算法合计 1,080 条实验记录（6×3×5×12）。原始数据含更新对的 P99 与最大探测次数、重建次数、槽位搬移、计时指标。

![95% 负载长期失败查询变化](figures/v0.2.0-miss-drift.svg)

![P99 更新代价](figures/v0.2.0-update-p99.svg)

![探测与更新代价 Pareto 权衡](figures/v0.2.0-tradeoff.svg)

**95% 负载、末四周期（9–12）、每种算法先对每个随机种子求均值，再对五个种子平均：**

| 对照算法 | 失败查询均值（探测/次） | 周期内 P99 更新对探测均值 | 单周期最大更新对探测均值 | 平均每周期全表重建次数 |
|---|---:|---:|---:|---:|
| Linear + tombstones | 2,593.27 | 4,052.1 | 4,162.4 | 0 |
| Linear + backshift | 168.56 | 1,334.6 | 1,504.5 | 0 |
| Robin Hood | **10.51** | 1,321.95 | 1,463.1 | 0 |
| Global rebuild at 2% capacity | 225.00 | 6,028.3 | **39,031.3** | 1.15 |
| Global rebuild at 8% capacity | 2,593.27 | 4,052.1 | 4,162.4 | 0 |
| Global rebuild at 50% slack | 263.22 | **1,157.6** | 29,325.35 | 0.75 |

**关键发现：**95% 真实占用意味着最多只有约 5% 的槽位可为空槽或墓碑，因此“墓碑超过总容量 8% 才清理”在该负载下根本不可能触发。以空闲预算而非总容量为基准的阈值可以启动清理，但全表维护形成极大的最大探测开销；P99 则可能漏掉单个稀有的重建尖峰。Robin Hood 在这里的失败查询探测表现更好，但不能直接将其定义为跨场景最优；计时数据受缓存和时钟扰动，不用于论文显著性声明。

[原始 1,080 行 CSV](results/v0.2.0-churn.csv) · [按种子聚合的摘要、探索性区间](results/v0.2.0-churn-summary.csv) · [ZombieHT 作者实现及复现进度](papers/zombie2025/REPRODUCTION.md)

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/hf_churn_benchmark 5 12 256 > results/v0.2.0-churn.csv
python scripts/analyze_churn.py
```

**研究状态：**已定位并审查作者公开仓库固定提交及原始构建/工作负载入口，并加入独立的作者代码编译与小型 smoke 工作流；这不是已经完成的原论文大规模复现。所有全表清理策略均属于经典对照，不冒充 Zombie Hashing。原论文实验需保留与本项目不同的 2²⁷ 槽设置、重复试验和具体硬件条件。

### 研究下一步

1. 对论文扩充检索并审计发表场所、实现/许可及复现实验规格；第一优先级是 **SIGMOD 2025 Zombie Hashing** 的窗口墓碑再分布，和 **SODA 2025 Bubble-up Cuckoo Hashing** 的高负载操作。
2. 在一致操作语义下复现原论文的实验设置，先重现单篇论文内部对比，再加入多算法交叉实验。
3. 将 **有界搬移预算 + 重排收益判断** 作为*待证伪的候选假设*，与已复现的顶会方法及经典基线比较；无显著收益则否定或修改，不预设创新成立。
4. 扩大到多数据规模、热点迁移、长周期 churn、失败查询比例、尾部延迟、内存预算及真实硬件计数器，最终在全新键集合/种子上独立确认。

### 主要已核实文献（2025）

- [Optimal Non-oblivious Open Addressing — STOC 2025](https://doi.org/10.1145/3717823.3718215)
- [Efficient d-ary Cuckoo Hashing at High Load Factors by Bubbling Up — SODA 2025](https://doi.org/10.1137/1.9781611978322.133)
- [History-Independent Concurrent Hash Tables — STOC 2025](https://doi.org/10.1145/3717823.3718283)
- [Zombie Hashing: Reanimating Tombstones in Graveyard — SIGMOD 2025](https://doi.org/10.1145/3725424)
- [GPH: An Efficient and Effective Perfect Hashing Scheme for GPU Architectures — SIGMOD 2025](https://doi.org/10.1145/3725406)

---

## English | Scope and evidence

HashFrontier is a **reproducible benchmark-first research project** for hash table construction and collision handling. It aims to identify robust time/space tradeoffs across diverse workloads, rather than optimize only a single synthetic speed score. Prefer classical algorithmic and mathematical mechanisms over machine-learning-based controllers.

**Admission window:** 1 January 2025 through 22 October 2026. Only relevant high-tier conference papers and date/category-verified Q1/Q2 journals are eligible. The five-paper list is a **verified starter shortlist, not a complete systematic review**. All five entries are **queued**; no paper reproduction is claimed in this release.

**v0.1.0 provides** three independent classical baselines, differential tests, a uniform workload generator, 810 real exploratory local measurements, source CSVs, SVG figures, a manuscript admission protocol, and automated CI / release workflow.

Build and reproduce with the commands above. CPU microbenchmarks are environment-sensitive and are not publication-quality inference. Static perfect hashing, concurrent dictionaries, and GPU tables will be evaluated in separate capability-aware tracks. The performance objective is a Pareto tradeoff involving construction, positive/negative lookup, mutation, tail latency, and space usage; no single opaque score decides the winner.

**Reproduction integrity:** Paper implementations remain `queued` until validated against the relevant paper specification and its experiment protocol. Proposed improvements will have independent ablations, matched traces and seeds, and held-out confirmation.

License: [MIT](LICENSE).

### English | v0.2.0 update

Six independently implemented classical/control variants (three basic tables plus three periodic full-rebuild policies), 1,080 paired 75/85/95%-load churn records and 1.8 million randomized differential operations have now been evaluated locally. **None of the new controls implements Zombie Hashing.** Original SIGMOD 2025 code provenance is pinned in [the reproducibility plan](papers/zombie2025/REPRODUCTION.md), with a separate upstream smoke CI workflow. At 95% occupancy, a tombstone trigger set at 8% of capacity can never activate because total vacancies are only around 5%; a slack-relative trigger works but causes rare O(n) rebuild spikes. Per-cycle P99 alone may hide such extreme maintenance events, so the benchmark records maximum per-update probes as well. CPU timings remain exploratory; no claims of statistically significant speedups or exact author-paper reproduction are made.
