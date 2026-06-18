# Generic networking statistics for netlink users

> 출처(원문): https://docs.kernel.org/networking/gen_stats.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Generic networking statistics for netlink users

Statistic counters are grouped into structs:

| Struct | TLV type | Description |
| --- | --- | --- |
| gnet\_stats\_basic | TCA\_STATS\_BASIC | Basic statistics |
| gnet\_stats\_rate\_est | TCA\_STATS\_RATE\_EST | Rate estimator |
| gnet\_stats\_queue | TCA\_STATS\_QUEUE | Queue statistics |
| none | TCA\_STATS\_APP | Application specific |

## Collecting:

Declare the statistic structs you need:

```
struct mystruct {
        struct gnet_stats_basic bstats;
        struct gnet_stats_queue qstats;
        ...
};
```

Update statistics, in `dequeue()` methods only, (while owning qdisc->running):

```
mystruct->tstats.packet++;
mystruct->qstats.backlog += skb->pkt_len;
```

## Export to userspace (Dump):

```
my_dumping_routine(struct sk_buff *skb, ...)
{
        struct gnet_dump dump;

        if (gnet_stats_start_copy(skb, TCA_STATS2, &mystruct->lock, &dump,
                                TCA_PAD) < 0)
                goto rtattr_failure;

        if (gnet_stats_copy_basic(&dump, &mystruct->bstats) < 0 ||
            gnet_stats_copy_queue(&dump, &mystruct->qstats) < 0 ||
                gnet_stats_copy_app(&dump, &xstats, sizeof(xstats)) < 0)
                goto rtattr_failure;

        if (gnet_stats_finish_copy(&dump) < 0)
                goto rtattr_failure;
        ...
}
```

## TCA\_STATS/TCA\_XSTATS backward compatibility:

Prior users of `struct tc_stats` and xstats can maintain backward
compatibility by calling the compat wrappers to keep providing the
existing TLV types:

```
my_dumping_routine(struct sk_buff *skb, ...)
{
    if (gnet_stats_start_copy_compat(skb, TCA_STATS2, TCA_STATS,
                                    TCA_XSTATS, &mystruct->lock, &dump,
                                    TCA_PAD) < 0)
                goto rtattr_failure;
        ...
}
```

A `struct tc_stats` will be filled out during gnet\_stats\_copy\_\* calls
and appended to the skb. TCA\_XSTATS is provided if gnet\_stats\_copy\_app
was called.

## Locking:

Locks are taken before writing and released once all statistics have
been written. Locks are always released in case of an error. You
are responsible for making sure that the lock is initialized.

## Rate Estimator:

0. Prepare an estimator attribute. Most likely this would be in user
   space. The value of this TLV should contain a tc\_estimator structure.
   As usual, such a TLV needs to be 32 bit aligned and therefore the
   length needs to be appropriately set, etc. The estimator interval
   and ewma log need to be converted to the appropriate values.
   tc\_estimator.c::`tc_setup_estimator()` is advisable to be used as the
   conversion routine. It does a few clever things. It takes a time
   interval in microsecs, a time constant also in microsecs and a `struct
   tc_estimator` to be populated. The returned tc\_estimator can be
   transported to the kernel. Transfer such a structure in a TLV of type
   TCA\_RATE to your code in the kernel.

In the kernel when setting up:

1. make sure you have basic stats and rate stats setup first.
2. make sure you have initialized stats lock that is used to setup such
   stats.
3. Now initialize a new estimator:

   ```
   int ret = gen_new_estimator(my_basicstats,my_rate_est_stats,
       mystats_lock, attr_with_tcestimator_struct);

   if ret == 0
       success
   else
       failed
   ```

From now on, every time you dump my\_rate\_est\_stats it will contain
up-to-date info.

Once you are done, call gen\_kill\_estimator(my\_basicstats,
my\_rate\_est\_stats) Make sure that my\_basicstats and my\_rate\_est\_stats
are still valid (i.e still exist) at the time of making this call.

## Authors:

* Thomas Graf <[tgraf@suug.ch](mailto:tgraf%40suug.ch)>
* Jamal Hadi Salim <[hadi@cyberus.ca](mailto:hadi%40cyberus.ca)>
