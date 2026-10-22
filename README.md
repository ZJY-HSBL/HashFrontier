# HashFrontier

**Hash table construction, collision resolution and reproducible benchmarks**  
**散列表构造、冲突处理与可复现基准研究**

[![Validate and benchmark](https://github.com/ZJY-HSBL/HashFrontier/actions/workflows/ci.yml/badge.svg)](https://github.com/ZJY-HSBL/HashFrontier/actions/workflows/ci.yml) · [Releases](https://github.com/ZJY-HSBL/HashFrontier/releases) · [Paper registry](papers/registry.csv)

## 中文 | 研究目标

通过 **2025-01-01—2026-10-22 高水平论文筛选 → 逐篇正确复现 → 统一负载评测 → 数据驱动机制设计 → 独立确认**，研究在构造耗时、成功/失败查询、动态插删、尾部延迟、空间开销之间具有稳健 Pareto 表现的散列表。核心方法优先不依赖机器学习。

收录范围限与散列表结构或冲突处理直接相关的 Nature / Science / Cell 体系、经年份及学科核实的 SCI Q1/Q2、以及 STOC / FOCS / SODA / SIGMOD / VLDB 等顶会。当前只有 **5 篇已核实的 2025 年论文候选**，并非穷尽检索；全部为 **queued（尚未复现）**。详见 [准入与验证协议](papers/PROTOCOL.md)。

**v0.1.0 = 基准基础设施与经典对照，不是 2025–2026 论文复现，也未实现原创候选算法。**

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
