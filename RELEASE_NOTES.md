# HashFrontier v0.12.0 — SIGMOD 2025 ZombieHT 原作者实现的真实云端测试

- 官方原版 ZombieHT fixed SHA `5064a381fd3b7a540594fadd6f7595fdcc748a6c`，四种方法 RHM / TRHM / GZHM / GZHM_ADAPTIVE 均为论文作者源码，原核心 `src/gqf.c`、`src/hm.c` SHA-256 与运行时检查一致。
- 仅给 **Benchmark 工作负载输入**加入固定随机种子、同周期操作指纹，不宣称 Benchmark 源文件未经改动。
- GitHub Actions 原生构建、Ubuntu 24.04 / GCC 13.3 / AMD EPYC 7763：70%、85%、95% × 3 seeds × 4 implementations，8192 slots / 96 churn cycles，**36/36 组完整、0 中断**。
- **873 条**分阶段操作指纹匹配，云端计算得到含失败审计的 `completion.csv`、完整运行计时、summary、环境与来源 SHA、SVG 图表。
- 95% 负载下的插入 clock ticks/op：RHM 595.61；TRHM 215.91；GZHM 629.12；GZHM_ADAPTIVE 522.55（3 seeds，最后 32 周期；真实测量、非 ns）。
- 注意：历史 95% × 320 周期 TRHM 失败并未由此 96 周期实验排除；此版是**真实云端探索复现**，未计算 HF-Score，也未完成统一 64-bit key/value 对比。
- Release 上传作者原生完整日志打包，保留源输入指纹校验和可独立复核的实验程序。实验记录 [GitHub Action #40175879142](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40175879142)。
