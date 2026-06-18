# DAMON: Data Access MONitoring and Access-aware System Operations

> 출처(원문): https://docs.kernel.org/mm/damon/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DAMON: Data Access MONitoring and Access-aware System Operations

DAMON is a Linux kernel subsystem for efficient [data access monitoring](design.html#damon-design-monitoring) and [access-aware system operations](design.html#damon-design-damos). It is designed for being

> * *accurate* (for DRAM level memory management),
> * *light-weight* (for production online usages),
> * *scalable* (in terms of memory size),
> * *tunable* (for flexible usages), and
> * *automated* (for production operation without manual tunings).

* [Frequently Asked Questions](faq.html)
  + [Does DAMON support virtual memory only?](faq.html#does-damon-support-virtual-memory-only)
  + [Can I simply monitor page granularity?](faq.html#can-i-simply-monitor-page-granularity)
* [Design](design.html)
  + [Execution Model and Data Structures](design.html#execution-model-and-data-structures)
  + [Overall Architecture](design.html#overall-architecture)
  + [Operations Set Layer](design.html#operations-set-layer)
  + [Core Logics](design.html#core-logics)
  + [Modules](design.html#modules)
* [API Reference](api.html)
  + [Structures](api.html#structures)
  + [Functions](api.html#functions)
* [DAMON Maintainer Entry Profile](maintainer-profile.html)
  + [SCM Trees](maintainer-profile.html#scm-trees)
  + [Submit checklist addendum](maintainer-profile.html#submit-checklist-addendum)
  + [Key cycle dates](maintainer-profile.html#key-cycle-dates)
  + [Review cadence](maintainer-profile.html#review-cadence)
  + [Mailing tool](maintainer-profile.html#mailing-tool)
  + [Community meetup](maintainer-profile.html#community-meetup)

To utilize and control DAMON from the user-space, please refer to the
administration [guide](../../admin-guide/mm/damon/index.html).

If you prefer academic papers for reading and citations, please use the papers
from [HPDC’22](https://dl.acm.org/doi/abs/10.1145/3502181.3531466) and
[Middleware19 Industry](https://dl.acm.org/doi/abs/10.1145/3366626.3368125) .
Note that those cover DAMON implementations in Linux v5.16 and v5.15,
respectively.
