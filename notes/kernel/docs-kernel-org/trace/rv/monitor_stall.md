# Monitor stall

> 출처(원문): https://docs.kernel.org/trace/rv/monitor_stall.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Monitor stall

* Name: stall - stalled task monitor
* Type: per-task hybrid automaton
* Author: Gabriele Monaco <[gmonaco@redhat.com](mailto:gmonaco%40redhat.com)>

## Description

The stalled task (stall) monitor is a sample per-task timed monitor that checks
if tasks are scheduled within a defined threshold after they are ready:

```
                       |
                       |
                       v
                     #==========================#
 +-----------------> H         dequeued         H
 |                   #==========================#
 |                     |
sched_switch_wait      | sched_wakeup;reset(clk)
 |                     v
 |                   +--------------------------+ <+
 |                   |         enqueued         |  | sched_wakeup
 |                   | clk < threshold_jiffies  | -+
 |                   +--------------------------+
 |                     |                 ^
 |              sched_switch_in    sched_switch_preempt;reset(clk)
 |                     v                 |
 |                   +--------------------------+
 +------------------ |         running          |
                     +--------------------------+
                       ^ sched_switch_in      |
                       | sched_wakeup         |
                       +----------------------+
```

The threshold can be configured as a parameter by either booting with the
`stall.threshold_jiffies=<new value>` argument or writing a new value to
`/sys/module/stall/parameters/threshold_jiffies`.

## Specification

Graphviz Dot file in tools/verification/models/stall.dot
