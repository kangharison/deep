# DAMON: Data Access MONitoring and Access-aware System Operations

> 출처(원문): https://docs.kernel.org/admin-guide/mm/damon/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DAMON: Data Access MONitoring and Access-aware System Operations

[DAMON](../../../mm/damon/index.html) is a Linux kernel subsystem for efficient data
access monitoring and access-aware system operations.

* [Getting Started](start.html)
  + [Prerequisites](start.html#prerequisites)
  + [Snapshot Data Access Patterns](start.html#snapshot-data-access-patterns)
  + [Recording Data Access Patterns](start.html#recording-data-access-patterns)
  + [Visualizing Recorded Patterns](start.html#visualizing-recorded-patterns)
  + [Data Access Pattern Aware Memory Management](start.html#data-access-pattern-aware-memory-management)
* [Detailed Usages](usage.html)
  + [sysfs Interface](usage.html#sysfs-interface)
  + [Tracepoints for Monitoring Results](usage.html#tracepoints-for-monitoring-results)
* [DAMON-based Reclamation](reclaim.html)
  + [Where Proactive Reclamation is Required?](reclaim.html#where-proactive-reclamation-is-required)
  + [How It Works?](reclaim.html#how-it-works)
  + [Interface: Module Parameters](reclaim.html#interface-module-parameters)
  + [Example](reclaim.html#example)
* [DAMON-based LRU-lists Sorting](lru_sort.html)
  + [Where Proactive LRU-lists Sorting is Required?](lru_sort.html#where-proactive-lru-lists-sorting-is-required)
  + [How It Works?](lru_sort.html#how-it-works)
  + [Interface: Module Parameters](lru_sort.html#interface-module-parameters)
  + [Example](lru_sort.html#example)
* [Data Access Monitoring Results Stat](stat.html)
  + [Monitoring Accuracy and Overhead](stat.html#monitoring-accuracy-and-overhead)
  + [Interface: Module Parameters](stat.html#interface-module-parameters)
