# API Reference

> 출처(원문): https://docs.kernel.org/mm/damon/api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# API Reference

Kernel space programs can use every feature of DAMON using below APIs. All you
need to do is including `damon.h`, which is located in `include/linux/` of
the source tree.

## Structures

struct damon\_addr\_range
:   Represents an address region of [**start**, **end**).

**Definition**:

```
struct damon_addr_range {
    unsigned long start;
    unsigned long end;
};
```

**Members**

`start`
:   Start address of the region (inclusive).

`end`
:   End address of the region (exclusive).

struct damon\_size\_range
:   Represents size for filter to operate on [**min**, **max**].

**Definition**:

```
struct damon_size_range {
    unsigned long min;
    unsigned long max;
};
```

**Members**

`min`
:   Min size (inclusive).

`max`
:   Max size (inclusive).

struct damon\_region
:   Represents a monitoring target region.

**Definition**:

```
struct damon_region {
    struct damon_addr_range ar;
    unsigned long sampling_addr;
    unsigned int nr_accesses;
    unsigned int nr_accesses_bp;
    struct list_head list;
    unsigned int age;
};
```

**Members**

`ar`
:   The address range of the region.

`sampling_addr`
:   Address of the sample for the next access check.

`nr_accesses`
:   Access frequency of this region.

`nr_accesses_bp`
:   **nr\_accesses** in basis point (0.01%) that updated for
    each sampling interval.

`list`
:   List head for siblings.

`age`
:   Age of this region.

**Description**

For any use case, **ar** should be non-zero positive size.

**nr\_accesses** is reset to zero for every [`damon_attrs->aggr_interval`](#c.damon_attrs "damon_attrs") and be
increased for every [`damon_attrs->sample_interval`](#c.damon_attrs "damon_attrs") if an access to the region
during the last sampling interval is found. The update of this field should
not be done with direct access but with the helper function,
[`damon_update_region_access_rate()`](#c.damon_update_region_access_rate "damon_update_region_access_rate").

**nr\_accesses\_bp** is another representation of **nr\_accesses** in basis point
(1 in 10,000) that updated for every [`damon_attrs->sample_interval`](#c.damon_attrs "damon_attrs") in a
manner similar to moving sum. By the algorithm, this value becomes
**nr\_accesses** \* 10000 for every [`struct damon_attrs`](#c.damon_attrs "damon_attrs")->aggr\_interval. This can
be used when the aggregation interval is too huge and therefore cannot wait
for it before getting the access monitoring results.

**age** is initially zero, increased for each aggregation interval, and reset
to zero again if the access frequency is significantly changed. If two
regions are merged into a new region, both **nr\_accesses** and **age** of the new
region are set as region size-weighted average of those of the two regions.

struct damon\_target
:   Represents a monitoring target.

**Definition**:

```
struct damon_target {
    struct pid *pid;
    unsigned int nr_regions;
    struct list_head regions_list;
    struct list_head list;
    bool obsolete;
};
```

**Members**

`pid`
:   The PID of the virtual address space to monitor.

`nr_regions`
:   Number of monitoring target regions of this target.

`regions_list`
:   Head of the monitoring target regions of this target.

`list`
:   List head for siblings.

`obsolete`
:   Whether the commit destination target is obsolete.

**Description**

Each monitoring context could have multiple targets. For example, a context
for virtual memory address spaces could have multiple target processes. The
**pid** should be set for appropriate [`struct damon_operations`](#c.damon_operations "damon_operations") including the
virtual address spaces monitoring operations.

**obsolete** is used only for `damon_commit_targets()` source targets, to specify
the matching destination targets are obsolete. Read `damon_commit_targets()`
to see how it is handled.

enum damos\_action
:   Represents an action of a Data Access Monitoring-based Operation Scheme.

**Constants**

`DAMOS_WILLNEED`
:   Call `madvise()` for the region with MADV\_WILLNEED.

`DAMOS_COLD`
:   Call `madvise()` for the region with MADV\_COLD.

`DAMOS_PAGEOUT`
:   Reclaim the region.

`DAMOS_HUGEPAGE`
:   Call `madvise()` for the region with MADV\_HUGEPAGE.

`DAMOS_NOHUGEPAGE`
:   Call `madvise()` for the region with MADV\_NOHUGEPAGE.

`DAMOS_LRU_PRIO`
:   Prioritize the region on its LRU lists.

`DAMOS_LRU_DEPRIO`
:   Deprioritize the region on its LRU lists.

`DAMOS_MIGRATE_HOT`
:   Migrate the regions prioritizing warmer regions.

`DAMOS_MIGRATE_COLD`
:   Migrate the regions prioritizing colder regions.

`DAMOS_STAT`
:   Do nothing but count the stat.

`NR_DAMOS_ACTIONS`
:   Total number of DAMOS actions

**Description**

The support of each action is up to running [`struct damon_operations`](#c.damon_operations "damon_operations").
Refer to ‘Operation Action’ section of [Design](design.html) for
status of the supports.

Note that DAMOS\_PAGEOUT doesn’t trigger demotions.

enum damos\_quota\_goal\_metric
:   Represents the metric to be used as the goal

**Constants**

`DAMOS_QUOTA_USER_INPUT`
:   User-input value.

`DAMOS_QUOTA_SOME_MEM_PSI_US`
:   System level some memory PSI in us.

`DAMOS_QUOTA_NODE_MEM_USED_BP`
:   MemUsed ratio of a node.

`DAMOS_QUOTA_NODE_MEM_FREE_BP`
:   MemFree ratio of a node.

`DAMOS_QUOTA_NODE_MEMCG_USED_BP`
:   MemUsed ratio of a node for a cgroup.

`DAMOS_QUOTA_NODE_MEMCG_FREE_BP`
:   MemFree ratio of a node for a cgroup.

`DAMOS_QUOTA_ACTIVE_MEM_BP`
:   Active to total LRU memory ratio.

`DAMOS_QUOTA_INACTIVE_MEM_BP`
:   Inactive to total LRU memory ratio.

`NR_DAMOS_QUOTA_GOAL_METRICS`
:   Number of DAMOS quota goal metrics.

**Description**

Metrics equal to larger than **NR\_DAMOS\_QUOTA\_GOAL\_METRICS** are unsupported.

struct damos\_quota\_goal
:   DAMOS scheme quota auto-tuning goal.

**Definition**:

```
struct damos_quota_goal {
    enum damos_quota_goal_metric metric;
    unsigned long target_value;
    unsigned long current_value;
    union {
        u64 last_psi_total;
        struct {
            int nid;
            u64 memcg_id;
        };
    };
    struct list_head list;
};
```

**Members**

`metric`
:   Metric to be used for representing the goal.

`target_value`
:   Target value of **metric** to achieve with the tuning.

`current_value`
:   Current value of **metric**.

`{unnamed_union}`
:   anonymous

`last_psi_total`
:   Last measured total PSI

`{unnamed_struct}`
:   anonymous

`nid`
:   Node id.

`memcg_id`
:   Memcg id.

`list`
:   List head for siblings.

**Description**

Data structure for getting the current score of the quota tuning goal. The
score is calculated by how close **current\_value** and **target\_value** are. Then
the score is entered to DAMON’s internal feedback loop mechanism to get the
auto-tuned quota.

If **metric** is DAMOS\_QUOTA\_USER\_INPUT, **current\_value** should be manually
entered by the user, probably inside the kdamond callbacks. Otherwise,
DAMON sets **current\_value** with self-measured value of **metric**.

If **metric** is DAMOS\_QUOTA\_NODE\_MEM\_{USED,FREE}\_BP, **nid** represents the node
id of the target node to account the used/free memory.

If **metric** is DAMOS\_QUOTA\_NODE\_MEMCG\_{USED,FREE}\_BP, **nid** and **memcg\_id**
represents the node id and the cgroup to account the used memory for.

enum damos\_quota\_goal\_tuner
:   Goal-based quota tuning logic.

**Constants**

`DAMOS_QUOTA_GOAL_TUNER_CONSIST`
:   Aim long term consistent quota.

`DAMOS_QUOTA_GOAL_TUNER_TEMPORAL`
:   Aim zero quota asap.

struct damos\_quota
:   Controls the aggressiveness of the given scheme.

**Definition**:

```
struct damos_quota {
    unsigned long reset_interval;
    unsigned long ms;
    unsigned long sz;
    struct list_head goals;
    enum damos_quota_goal_tuner goal_tuner;
    unsigned long esz;
    unsigned int weight_sz;
    unsigned int weight_nr_accesses;
    unsigned int weight_age;
};
```

**Members**

`reset_interval`
:   Charge reset interval in milliseconds.

`ms`
:   Maximum milliseconds that the scheme can use.

`sz`
:   Maximum bytes of memory that the action can be applied.

`goals`
:   Head of quota tuning goals ([`damos_quota_goal`](#c.damos_quota_goal "damos_quota_goal")) list.

`goal_tuner`
:   Goal-based **esz** tuning algorithm to use.

`esz`
:   Effective size quota in bytes.

`weight_sz`
:   Weight of the region’s size for prioritization.

`weight_nr_accesses`
:   Weight of the region’s nr\_accesses for prioritization.

`weight_age`
:   Weight of the region’s age for prioritization.

**Description**

To avoid consuming too much CPU time or IO resources for applying the
[`struct damos`](#c.damos "damos")->action to large memory, DAMON allows users to set time and/or
size quotas. The quotas can be set by writing non-zero values to `ms` and
`sz`, respectively. If the time quota is set, DAMON tries to use only up to
`ms` milliseconds within `reset_interval` for applying the action. If the
size quota is set, DAMON tries to apply the action only up to `sz` bytes
within `reset_interval`.

To convince the different types of quotas and goals, DAMON internally
converts those into one single size quota called “effective quota”. DAMON
internally uses it as the only one real quota. The conversion is made as
follows.

The time quota is transformed to a size quota using estimated throughput of
the scheme’s action. DAMON then compares it against `sz` and uses smaller
one as the effective quota.

If **goals** is not empty, DAMON calculates yet another size quota based on the
goals using its internal feedback loop algorithm, for every **reset\_interval**.
Then, if the new size quota is smaller than the effective quota, it uses the
new size quota as the effective quota.

The resulting effective size quota in bytes is set to **esz**.

For selecting regions within the quota, DAMON prioritizes current scheme’s
target memory regions using the [`struct damon_operations`](#c.damon_operations "damon_operations")->get\_scheme\_score.
You could customize the prioritization logic by setting `weight_sz`,
`weight_nr_accesses`, and `weight_age`, because monitoring operations are
encouraged to respect those.

enum damos\_wmark\_metric
:   Represents the watermark metric.

**Constants**

`DAMOS_WMARK_NONE`
:   Ignore the watermarks of the given scheme.

`DAMOS_WMARK_FREE_MEM_RATE`
:   Free memory rate of the system in [0,1000].

`NR_DAMOS_WMARK_METRICS`
:   Total number of DAMOS watermark metrics

struct damos\_watermarks
:   Controls when a given scheme should be activated.

**Definition**:

```
struct damos_watermarks {
    enum damos_wmark_metric metric;
    unsigned long interval;
    unsigned long high;
    unsigned long mid;
    unsigned long low;
};
```

**Members**

`metric`
:   Metric for the watermarks.

`interval`
:   Watermarks check time interval in microseconds.

`high`
:   High watermark.

`mid`
:   Middle watermark.

`low`
:   Low watermark.

**Description**

If `metric` is `DAMOS_WMARK_NONE`, the scheme is always active. Being active
means DAMON does monitoring and applying the action of the scheme to
appropriate memory regions. Else, DAMON checks `metric` of the system for at
least every `interval` microseconds and works as below.

If `metric` is higher than `high`, the scheme is inactivated. If `metric` is
between `mid` and `low`, the scheme is activated. If `metric` is lower than
`low`, the scheme is inactivated.

struct damos\_stat
:   Statistics on a given scheme.

**Definition**:

```
struct damos_stat {
    unsigned long nr_tried;
    unsigned long sz_tried;
    unsigned long nr_applied;
    unsigned long sz_applied;
    unsigned long sz_ops_filter_passed;
    unsigned long qt_exceeds;
    unsigned long nr_snapshots;
};
```

**Members**

`nr_tried`
:   Total number of regions that the scheme is tried to be applied.

`sz_tried`
:   Total size of regions that the scheme is tried to be applied.

`nr_applied`
:   Total number of regions that the scheme is applied.

`sz_applied`
:   Total size of regions that the scheme is applied.

`sz_ops_filter_passed`
:   Total bytes that passed ops layer-handled DAMOS filters.

`qt_exceeds`
:   Total number of times the quota of the scheme has exceeded.

`nr_snapshots`
:   Total number of DAMON snapshots that the scheme has tried.

**Description**

“Tried an action to a region” in this context means the DAMOS core logic
determined the region as eligible to apply the action. The access pattern
([`struct damos_access_pattern`](#c.damos_access_pattern "damos_access_pattern")), quotas ([`struct damos_quota`](#c.damos_quota "damos_quota")), watermarks
([`struct damos_watermarks`](#c.damos_watermarks "damos_watermarks")) and filters ([`struct damos_filter`](#c.damos_filter "damos_filter")) that handled
on core logic can affect this. The core logic asks the operation set
([`struct damon_operations`](#c.damon_operations "damon_operations")) to apply the action to the region.

“Applied an action to a region” in this context means the operation set
([`struct damon_operations`](#c.damon_operations "damon_operations")) successfully applied the action to the region, at
least to a part of the region. The filters ([`struct damos_filter`](#c.damos_filter "damos_filter")) that
handled on operation set layer and type of the action and pages of the
region can affect this. For example, if a filter is set to exclude
anonymous pages and the region has only anonymous pages, the region will be
failed at applying the action. If the action is `DAMOS_PAGEOUT` and all
pages of the region are already paged out, the region will be failed at
applying the action.

enum damos\_filter\_type
:   Type of memory for [`struct damos_filter`](#c.damos_filter "damos_filter")

**Constants**

`DAMOS_FILTER_TYPE_ANON`
:   Anonymous pages.

`DAMOS_FILTER_TYPE_ACTIVE`
:   Active pages.

`DAMOS_FILTER_TYPE_MEMCG`
:   Specific memcg’s pages.

`DAMOS_FILTER_TYPE_YOUNG`
:   Recently accessed pages.

`DAMOS_FILTER_TYPE_HUGEPAGE_SIZE`
:   Page is part of a hugepage.

`DAMOS_FILTER_TYPE_UNMAPPED`
:   Unmapped pages.

`DAMOS_FILTER_TYPE_ADDR`
:   Address range.

`DAMOS_FILTER_TYPE_TARGET`
:   Data Access Monitoring target.

`NR_DAMOS_FILTER_TYPES`
:   Number of filter types.

**Description**

The anon pages type and memcg type filters are handled by underlying
[`struct damon_operations`](#c.damon_operations "damon_operations") as a part of scheme action trying, and therefore
accounted as ‘tried’. In contrast, other types are handled by core layer
before trying of the action and therefore not accounted as ‘tried’.

The support of the filters that handled by [`struct damon_operations`](#c.damon_operations "damon_operations") depend
on the running [`struct damon_operations`](#c.damon_operations "damon_operations").
`enum DAMON_OPS_PADDR` supports both anon pages type and memcg type filters,
while `enum DAMON_OPS_VADDR` and `enum DAMON_OPS_FVADDR` don’t support any of
the two types.

struct damos\_filter
:   DAMOS action target memory filter.

**Definition**:

```
struct damos_filter {
    enum damos_filter_type type;
    bool matching;
    bool allow;
    union {
        u64 memcg_id;
        struct damon_addr_range addr_range;
        int target_idx;
        struct damon_size_range sz_range;
    };
    struct list_head list;
};
```

**Members**

`type`
:   Type of the target memory.

`matching`
:   Whether this is for **type**-matching memory.

`allow`
:   Whether to include or exclude the **matching** memory.

`{unnamed_union}`
:   anonymous

`memcg_id`
:   Memcg id of the question if **type** is DAMOS\_FILTER\_MEMCG.

`addr_range`
:   Address range if **type** is DAMOS\_FILTER\_TYPE\_ADDR.

`target_idx`
:   Index of the [`struct damon_target`](#c.damon_target "damon_target") of
    [`damon_ctx->adaptive_targets`](#c.damon_ctx "damon_ctx") if **type** is
    DAMOS\_FILTER\_TYPE\_TARGET.

`sz_range`
:   Size range if **type** is DAMOS\_FILTER\_TYPE\_HUGEPAGE\_SIZE.

`list`
:   List head for siblings.

**Description**

Before applying the [`damos->action`](#c.damos "damos") to a memory region, DAMOS checks if each
byte of the region matches to this given condition and avoid applying the
action if so. Support of each filter type depends on the running [`struct
damon_operations`](#c.damon_operations "damon_operations") and the type. Refer to [`enum damos_filter_type`](#c.damos_filter_type "damos_filter_type") for more
details.

struct damos\_walk\_control
:   Control [`damos_walk()`](#c.damos_walk "damos_walk").

**Definition**:

```
struct damos_walk_control {
    void (*walk_fn)(void *data, struct damon_ctx *ctx, struct damon_target *t, struct damon_region *r, struct damos *s, unsigned long sz_filter_passed);
    void *data;
};
```

**Members**

`walk_fn`
:   Function to be called back for each region.

`data`
:   Data that will be passed to walk functions.

**Description**

Control [`damos_walk()`](#c.damos_walk "damos_walk"), which requests specific kdamond to invoke the given
function to each region that eligible to apply actions of the kdamond’s
schemes. Refer to [`damos_walk()`](#c.damos_walk "damos_walk") for more details.

struct damos\_access\_pattern
:   Target access pattern of the given scheme.

**Definition**:

```
struct damos_access_pattern {
    unsigned long min_sz_region;
    unsigned long max_sz_region;
    unsigned int min_nr_accesses;
    unsigned int max_nr_accesses;
    unsigned int min_age_region;
    unsigned int max_age_region;
};
```

**Members**

`min_sz_region`
:   Minimum size of target regions.

`max_sz_region`
:   Maximum size of target regions.

`min_nr_accesses`
:   Minimum `->nr_accesses` of target regions.

`max_nr_accesses`
:   Maximum `->nr_accesses` of target regions.

`min_age_region`
:   Minimum age of target regions.

`max_age_region`
:   Maximum age of target regions.

struct damos\_migrate\_dests
:   Migration destination nodes and their weights.

**Definition**:

```
struct damos_migrate_dests {
    unsigned int *node_id_arr;
    unsigned int *weight_arr;
    size_t nr_dests;
};
```

**Members**

`node_id_arr`
:   Array of migration destination node ids.

`weight_arr`
:   Array of migration weights for **node\_id\_arr**.

`nr_dests`
:   Length of the **node\_id\_arr** and **weight\_arr** arrays.

**Description**

**node\_id\_arr** is an array of the ids of migration destination nodes.
**weight\_arr** is an array of the weights for those. The weights in
**weight\_arr** are for nodes in **node\_id\_arr** of same array index.

struct damos
:   Represents a Data Access Monitoring-based Operation Scheme.

**Definition**:

```
struct damos {
    struct damos_access_pattern pattern;
    enum damos_action action;
    unsigned long apply_interval_us;
    struct damos_quota quota;
    struct damos_watermarks wmarks;
    union {
        struct {
            int target_nid;
            struct damos_migrate_dests migrate_dests;
        };
    };
    struct list_head core_filters;
    struct list_head ops_filters;
    void *last_applied;
    struct damos_stat stat;
    unsigned long max_nr_snapshots;
    struct list_head list;
};
```

**Members**

`pattern`
:   Access pattern of target regions.

`action`
:   [`damos_action`](#c.damos_action "damos_action") to be applied to the target regions.

`apply_interval_us`
:   The time between applying the **action**.

`quota`
:   Control the aggressiveness of this scheme.

`wmarks`
:   Watermarks for automated (in)activation of this scheme.

`{unnamed_union}`
:   anonymous

`{unnamed_struct}`
:   anonymous

`target_nid`
:   Destination node if **action** is “migrate\_{hot,cold}”.

`migrate_dests`
:   Destination nodes if **action** is “migrate\_{hot,cold}”.

`core_filters`
:   Additional set of [`struct damos_filter`](#c.damos_filter "damos_filter") for `action`.

`ops_filters`
:   ops layer handling [`struct damos_filter`](#c.damos_filter "damos_filter") objects list.

`last_applied`
:   Last **action** applied ops-managing entity.

`stat`
:   Statistics of this scheme.

`max_nr_snapshots`
:   Upper limit of nr\_snapshots stat.

`list`
:   List head for siblings.

**Description**

For each **apply\_interval\_us**, DAMON finds regions which fit in the
`pattern` and applies `action` to those. To avoid consuming too much
CPU time or IO resources for the `action`, `quota` is used.

If **apply\_interval\_us** is zero, [`damon_attrs->aggr_interval`](#c.damon_attrs "damon_attrs") is used instead.

To do the work only when needed, schemes can be activated for specific
system situations using `wmarks`. If all schemes that registered to the
monitoring context are inactive, DAMON stops monitoring either, and just
repeatedly checks the watermarks.

**migrate\_dests** specifies multiple migration target nodes with different
weights for migrate\_hot or migrate\_cold actions. **target\_nid** is ignored if
this is set.

**target\_nid** is used to set the migration target node for migrate\_hot or
migrate\_cold actions, and **migrate\_dests** is unset.

Before applying the `action` to a memory region, [`struct damon_operations`](#c.damon_operations "damon_operations")
implementation could check pages of the region and skip `action` to respect
`core_filters`

The minimum entity that **action** can be applied depends on the underlying
[`struct damon_operations`](#c.damon_operations "damon_operations"). Since it may not be aligned with the core layer
abstract, namely [`struct damon_region`](#c.damon_region "damon_region"), [`struct damon_operations`](#c.damon_operations "damon_operations") could apply
**action** to same entity multiple times. Large folios that underlying on
multiple `struct damon` region objects could be such examples. The [`struct
damon_operations`](#c.damon_operations "damon_operations") can use **last\_applied** to avoid that. DAMOS core logic
unsets **last\_applied** when each regions walking for applying the scheme is
finished.

After applying the `action` to each region, `stat` is updated.

If `max_nr_snapshots` is set as non-zero and `stat.nr_snapshots` be same to or
greater than it, the scheme is deactivated.

enum damon\_ops\_id
:   Identifier for each monitoring operations implementation

**Constants**

`DAMON_OPS_VADDR`
:   Monitoring operations for virtual address spaces

`DAMON_OPS_FVADDR`
:   Monitoring operations for only fixed ranges of virtual
    address spaces

`DAMON_OPS_PADDR`
:   Monitoring operations for the physical address space

`NR_DAMON_OPS`
:   Number of monitoring operations implementations

struct damon\_operations
:   Monitoring operations for given use cases.

**Definition**:

```
struct damon_operations {
    enum damon_ops_id id;
    void (*init)(struct damon_ctx *context);
    void (*update)(struct damon_ctx *context);
    void (*prepare_access_checks)(struct damon_ctx *context);
    unsigned int (*check_accesses)(struct damon_ctx *context);
    int (*get_scheme_score)(struct damon_ctx *context, struct damon_region *r, struct damos *scheme);
    unsigned long (*apply_scheme)(struct damon_ctx *context, struct damon_target *t, struct damon_region *r, struct damos *scheme, unsigned long *sz_filter_passed);
    bool (*target_valid)(struct damon_target *t);
    void (*cleanup_target)(struct damon_target *t);
};
```

**Members**

`id`
:   Identifier of this operations set.

`init`
:   Initialize operations-related data structures.

`update`
:   Update operations-related data structures.

`prepare_access_checks`
:   Prepare next access check of target regions.

`check_accesses`
:   Check the accesses to target regions.

`get_scheme_score`
:   Get the score of a region for a scheme.

`apply_scheme`
:   Apply a DAMON-based operation scheme.

`target_valid`
:   Determine if the target is valid.

`cleanup_target`
:   Clean up each target before deallocation.

**Description**

DAMON can be extended for various address spaces and usages. For this,
users should register the low level operations for their target address
space and usecase via the [`damon_ctx.ops`](#c.damon_ctx "damon_ctx"). Then, the monitoring thread
([`damon_ctx.kdamond`](#c.damon_ctx "damon_ctx")) calls **init** and **prepare\_access\_checks** before starting
the monitoring, **update** after each [`damon_attrs.ops_update_interval`](#c.damon_attrs "damon_attrs"), and
**check\_accesses**, **target\_valid** and **prepare\_access\_checks** after each
[`damon_attrs.sample_interval`](#c.damon_attrs "damon_attrs").

Each [`struct damon_operations`](#c.damon_operations "damon_operations") instance having valid **id** can be registered
via [`damon_register_ops()`](#c.damon_register_ops "damon_register_ops") and selected by [`damon_select_ops()`](#c.damon_select_ops "damon_select_ops") later.
**init** should initialize operations-related data structures. For example,
this could be used to construct proper monitoring target regions and link
those to **damon\_ctx.adaptive\_targets**.
**update** should update the operations-related data structures. For example,
this could be used to update monitoring target regions for current status.
**prepare\_access\_checks** should manipulate the monitoring regions to be
prepared for the next access check.
**check\_accesses** should check the accesses to each region that made after the
last preparation and update the number of observed accesses of each region.
It should also return max number of observed accesses that made as a result
of its update. The value will be used for regions adjustment threshold.
**get\_scheme\_score** should return the priority score of a region for a scheme
as an integer in [0, `DAMOS_MAX_SCORE`].
**apply\_scheme** is called from **kdamond** when a region for user provided
DAMON-based operation scheme is found. It should apply the scheme’s action
to the region and return bytes of the region that the action is successfully
applied. It should also report how many bytes of the region has passed
filters ([`struct damos_filter`](#c.damos_filter "damos_filter")) that handled by itself.
**target\_valid** should check whether the target is still valid for the
monitoring.
**cleanup\_target** is called before the target will be deallocated.

struct damon\_intervals\_goal
:   Monitoring intervals auto-tuning goal.

**Definition**:

```
struct damon_intervals_goal {
    unsigned long access_bp;
    unsigned long aggrs;
    unsigned long min_sample_us;
    unsigned long max_sample_us;
};
```

**Members**

`access_bp`
:   Access events observation ratio to achieve in bp.

`aggrs`
:   Number of aggregations to achieve **access\_bp** within.

`min_sample_us`
:   Minimum resulting sampling interval in microseconds.

`max_sample_us`
:   Maximum resulting sampling interval in microseconds.

**Description**

DAMON automatically tunes [`damon_attrs->sample_interval`](#c.damon_attrs "damon_attrs") and
[`damon_attrs->aggr_interval`](#c.damon_attrs "damon_attrs") aiming the ratio in bp (1/10,000) of
DAMON-observed access events to theoretical maximum amount within **aggrs**
aggregations be same to **access\_bp**. The logic increases
[`damon_attrs->aggr_interval`](#c.damon_attrs "damon_attrs") and [`damon_attrs->sampling_interval`](#c.damon_attrs "damon_attrs") in same
ratio if the current access events observation ratio is lower than the
target for each **aggrs** aggregations, and vice versa.

If **aggrs** is zero, the tuning is disabled and hence this `struct is` ignored.

struct damon\_attrs
:   Monitoring attributes for accuracy/overhead control.

**Definition**:

```
struct damon_attrs {
    unsigned long sample_interval;
    unsigned long aggr_interval;
    unsigned long ops_update_interval;
    struct damon_intervals_goal intervals_goal;
    unsigned long min_nr_regions;
    unsigned long max_nr_regions;
};
```

**Members**

`sample_interval`
:   The time between access samplings.

`aggr_interval`
:   The time between monitor results aggregations.

`ops_update_interval`
:   The time between monitoring operations updates.

`intervals_goal`
:   Intervals auto-tuning goal.

`min_nr_regions`
:   The minimum number of adaptive monitoring
    regions.

`max_nr_regions`
:   The maximum number of adaptive monitoring
    regions.

**Description**

For each **sample\_interval**, DAMON checks whether each region is accessed or
not during the last **sample\_interval**. If such access is found, DAMON
aggregates the information by increasing [`damon_region->nr_accesses`](#c.damon_region "damon_region") for
**aggr\_interval** time. For each **aggr\_interval**, the count is reset. DAMON
also checks whether the target memory regions need update (e.g., by
`mmap()` calls from the application, in case of virtual memory monitoring)
and applies the changes for each **ops\_update\_interval**. All time intervals
are in micro-seconds. Please refer to [`struct damon_operations`](#c.damon_operations "damon_operations") and `struct
damon_call_control` for more detail.

struct damon\_ctx
:   Represents a context for each monitoring. This is the main interface that allows users to set the attributes and get the results of the monitoring.

**Definition**:

```
struct damon_ctx {
    struct damon_attrs attrs;
    struct damon_operations ops;
    unsigned long addr_unit;
    unsigned long min_region_sz;
    struct list_head adaptive_targets;
    struct list_head schemes;
};
```

**Members**

`attrs`
:   Monitoring attributes for accuracy/overhead control.

`ops`
:   Set of monitoring operations for given use cases.

`addr_unit`
:   Scale factor for core to ops address conversion.

`min_region_sz`
:   Minimum region size.

`adaptive_targets`
:   Head of monitoring targets ([`damon_target`](#c.damon_target "damon_target")) list.

`schemes`
:   Head of schemes ([`damos`](#c.damos "damos")) list.

**Description**

For each monitoring context, one kernel thread for the monitoring, namely
kdamond, is created. The pid of kdamond can be retrieved using
[`damon_kdamond_pid()`](#c.damon_kdamond_pid "damon_kdamond_pid").

Once started, kdamond runs until explicitly required to be terminated or
every monitoring target is invalid. The validity of the targets is checked
via the [`damon_operations.target_valid`](#c.damon_operations "damon_operations") of **ops**. The termination can also be
explicitly requested by calling [`damon_stop()`](#c.damon_stop "damon_stop"). To know if a kdamond is
running, [`damon_is_running()`](#c.damon_is_running "damon_is_running") can be used.

While the kdamond is running, all accesses to [`struct damon_ctx`](#c.damon_ctx "damon_ctx") from a
thread other than the kdamond should be made using safe DAMON APIs,
including [`damon_call()`](#c.damon_call "damon_call") and [`damos_walk()`](#c.damos_walk "damos_walk").

## Functions

bool damon\_is\_registered\_ops(enum [damon\_ops\_id](#c.damon_ops_id "damon_ops_id") id)
:   Check if a given damon\_operations is registered.

**Parameters**

`enum damon_ops_id id`
:   Id of the damon\_operations to check if registered.

**Return**

true if the ops is set, false otherwise.

int damon\_register\_ops(struct [damon\_operations](#c.damon_operations "damon_operations") \*ops)
:   Register a monitoring operations set to DAMON.

**Parameters**

`struct damon_operations *ops`
:   monitoring operations set to register.

**Description**

This function registers a monitoring operations set of valid [`struct
damon_operations`](#c.damon_operations "damon_operations")->id so that others can find and use them later.

**Return**

0 on success, negative error code otherwise.

int damon\_select\_ops(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx, enum [damon\_ops\_id](#c.damon_ops_id "damon_ops_id") id)
:   Select a monitoring operations to use with the context.

**Parameters**

`struct damon_ctx *ctx`
:   monitoring context to use the operations.

`enum damon_ops_id id`
:   id of the registered monitoring operations to select.

**Description**

This function finds registered monitoring operations set of **id** and make
**ctx** to use it.

**Return**

0 on success, negative error code otherwise.

bool damos\_filter\_for\_ops(enum [damos\_filter\_type](#c.damos_filter_type "damos_filter_type") type)
:   Return if the filter is ops-handled one.

**Parameters**

`enum damos_filter_type type`
:   type of the filter.

**Return**

true if the filter of **type** needs to be handled by ops layer, false
otherwise.

int damon\_set\_attrs(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx, struct [damon\_attrs](#c.damon_attrs "damon_attrs") \*attrs)
:   Set attributes for the monitoring.

**Parameters**

`struct damon_ctx *ctx`
:   monitoring context

`struct damon_attrs *attrs`
:   monitoring attributes

**Description**

This function updates monitoring results and next monitoring/damos operation
schedules. Because those are periodically updated by kdamond, this should
be called from a safe contexts. Such contexts include damon\_ctx setup time
while the kdamond is not yet started, and inside of `kdamond_fn()`.

In detail, all DAMON API callers directly call this function for initial
setup of damon\_ctx before calling [`damon_start()`](#c.damon_start "damon_start"). Some of the API callers
also indirectly call this function via [`damon_call()`](#c.damon_call "damon_call") -> `damon_commit()` for
online parameters updates. Finally, `kdamond_fn()` itself use this for
applying auto-tuned monitoring intervals.

Every time interval is in micro-seconds.

**Return**

0 on success, negative error code otherwise.

void damon\_set\_schemes(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx, struct [damos](#c.damos "damos") \*\*schemes, ssize\_t nr\_schemes)
:   Set data access monitoring based operation schemes.

**Parameters**

`struct damon_ctx *ctx`
:   monitoring context

`struct damos **schemes`
:   array of the schemes

`ssize_t nr_schemes`
:   number of entries in **schemes**

**Description**

This function should not be called while the kdamond of the context is
running.

int damos\_commit\_quota\_goals(struct [damos\_quota](#c.damos_quota "damos_quota") \*dst, struct [damos\_quota](#c.damos_quota "damos_quota") \*src)
:   Commit DAMOS quota goals to another quota.

**Parameters**

`struct damos_quota *dst`
:   The commit destination DAMOS quota.

`struct damos_quota *src`
:   The commit source DAMOS quota.

**Description**

Copies user-specified parameters for quota goals from **src** to **dst**. Users
should use this function for quota goals-level parameters update of running
DAMON contexts, instead of manual in-place updates.

This function should be called from parameters-update safe context, like
[`damon_call()`](#c.damon_call "damon_call").

bool damos\_filters\_default\_reject(struct list\_head \*filters)
:   decide whether to reject memory that didn’t match with any given filter.

**Parameters**

`struct list_head *filters`
:   Given DAMOS filters of a group.

int damon\_commit\_ctx(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*dst, struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*src)
:   Commit parameters of a DAMON context to another.

**Parameters**

`struct damon_ctx *dst`
:   The commit destination DAMON context.

`struct damon_ctx *src`
:   The commit source DAMON context.

**Description**

This function copies user-specified parameters from **src** to **dst** and update
the internal status and results accordingly. Users should use this function
for context-level parameters update of running context, instead of manual
in-place updates.

This function should be called from parameters-update safe context, like
[`damon_call()`](#c.damon_call "damon_call").

int damon\_nr\_running\_ctxs(void)
:   Return number of currently running contexts.

**Parameters**

`void`
:   no arguments

int damon\_start(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*\*ctxs, int nr\_ctxs, bool exclusive)
:   Starts the monitorings for a given group of contexts.

**Parameters**

`struct damon_ctx **ctxs`
:   an array of the pointers for contexts to start monitoring

`int nr_ctxs`
:   size of **ctxs**

`bool exclusive`
:   exclusiveness of this contexts group

**Description**

This function starts a group of monitoring threads for a group of monitoring
contexts. One thread per each context is created and run in parallel. The
caller should handle synchronization between the threads by itself. If
**exclusive** is true and a group of threads that created by other
‘[`damon_start()`](#c.damon_start "damon_start")’ call is currently running, this function does nothing but
returns -EBUSY.

**Return**

0 on success, negative error code otherwise.

int damon\_stop(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*\*ctxs, int nr\_ctxs)
:   Stops the monitorings for a given group of contexts.

**Parameters**

`struct damon_ctx **ctxs`
:   an array of the pointers for contexts to stop monitoring

`int nr_ctxs`
:   size of **ctxs**

**Return**

0 on success, negative error code otherwise.

bool damon\_is\_running(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx)
:   Returns if a given DAMON context is running.

**Parameters**

`struct damon_ctx *ctx`
:   The DAMON context to see if running.

**Return**

true if **ctx** is running, false otherwise.

int damon\_kdamond\_pid(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx)
:   Return pid of a given DAMON context’s worker thread.

**Parameters**

`struct damon_ctx *ctx`
:   The DAMON context of the question.

**Return**

pid if **ctx** is running, negative error code otherwise.

int damon\_call(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx, struct damon\_call\_control \*control)
:   Invoke a given function on DAMON worker thread (kdamond).

**Parameters**

`struct damon_ctx *ctx`
:   DAMON context to call the function for.

`struct damon_call_control *control`
:   Control variable of the call request.

**Description**

Ask DAMON worker thread (kdamond) of **ctx** to call a function with an
argument data that respectively passed via `damon_call_control->fn` and
`damon_call_control->data` of **control**. If `damon_call_control->repeat` of
**control** is unset, further wait until the kdamond finishes handling of the
request. Otherwise, return as soon as the request is made.

The kdamond executes the function with the argument in the main loop, just
after a sampling of the iteration is finished. The function can hence
safely access the internal data of the [`struct damon_ctx`](#c.damon_ctx "damon_ctx") without additional
synchronization. The return value of the function will be saved in
`damon_call_control->return_code`.

Note that this function should be called only after [`damon_start()`](#c.damon_start "damon_start") with the
**ctx** has succeeded. Otherwise, this function could fall into an indefinite
wait.

**Return**

0 on success, negative error code otherwise.

int damos\_walk(struct [damon\_ctx](#c.damon_ctx "damon_ctx") \*ctx, struct [damos\_walk\_control](#c.damos_walk_control "damos_walk_control") \*control)
:   Invoke a given functions while DAMOS walk regions.

**Parameters**

`struct damon_ctx *ctx`
:   DAMON context to call the functions for.

`struct damos_walk_control *control`
:   Control variable of the walk request.

**Description**

Ask DAMON worker thread (kdamond) of **ctx** to call a function for each region
that the kdamond will apply DAMOS action to, and wait until the kdamond
finishes handling of the request.

The kdamond executes the given function in the main loop, for each region
just after it applied any DAMOS actions of **ctx** to it. The invocation is
made only within one [`damos->apply_interval_us`](#c.damos "damos") since [`damos_walk()`](#c.damos_walk "damos_walk")
invocation, for each scheme. The given callback function can hence safely
access the internal data of [`struct damon_ctx`](#c.damon_ctx "damon_ctx") and [`struct damon_region`](#c.damon_region "damon_region") that
each of the scheme will apply the action for next interval, without
additional synchronizations against the kdamond. If every scheme of **ctx**
passed at least one [`damos->apply_interval_us`](#c.damos "damos"), kdamond marks the request as
completed so that [`damos_walk()`](#c.damos_walk "damos_walk") can wakeup and return.

Note that this function should be called only after [`damon_start()`](#c.damon_start "damon_start") with the
**ctx** has succeeded. Otherwise, this function could fall into an indefinite
wait.

**Return**

0 on success, negative error code otherwise.

int damon\_set\_region\_biggest\_system\_ram\_default(struct [damon\_target](#c.damon_target "damon_target") \*t, unsigned long \*start, unsigned long \*end, unsigned long addr\_unit, unsigned long min\_region\_sz)
:   Set the region of the given monitoring target as requested, or biggest ‘System RAM’.

**Parameters**

`struct damon_target *t`
:   The monitoring target to set the region.

`unsigned long *start`
:   The pointer to the start address of the region.

`unsigned long *end`
:   The pointer to the end address of the region.

`unsigned long addr_unit`
:   The address unit for the damon\_ctx of **t**.

`unsigned long min_region_sz`
:   Minimum region size.

**Description**

This function sets the region of **t** as requested by **start** and **end**. If the
values of **start** and **end** are zero, however, this function finds the biggest
‘System RAM’ resource and sets the region to cover the resource. In the
latter case, this function saves the start and end addresses of the resource
in **start** and **end**, respectively.

**Return**

0 on success, negative error code otherwise.

void damon\_update\_region\_access\_rate(struct [damon\_region](#c.damon_region "damon_region") \*r, bool accessed, struct [damon\_attrs](#c.damon_attrs "damon_attrs") \*attrs)
:   Update the access rate of a region.

**Parameters**

`struct damon_region *r`
:   The DAMON region to update for its access check result.

`bool accessed`
:   Whether the region has accessed during last sampling interval.

`struct damon_attrs *attrs`
:   The damon\_attrs of the DAMON context.

**Description**

Update the access rate of a region with the region’s last sampling interval
access check result.

Usually this will be called by [`damon_operations->check_accesses`](#c.damon_operations "damon_operations") callback.

bool damon\_initialized(void)
:   Return if DAMON is ready to be used.

**Parameters**

`void`
:   no arguments

**Return**

true if DAMON is ready to be used, false otherwise.
