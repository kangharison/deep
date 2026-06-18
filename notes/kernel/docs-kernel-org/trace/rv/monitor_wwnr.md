# Monitor wwnr

> 출처(원문): https://docs.kernel.org/trace/rv/monitor_wwnr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Monitor wwnr

* Name: wwrn - wakeup while not running
* Type: per-task deterministic automaton
* Author: Daniel Bristot de Oliveira <[bristot@kernel.org](mailto:bristot%40kernel.org)>

## Description

This is a per-task sample monitor, with the following
definition:

```
             |
             |
             v
  wakeup   +-------------+
+--------- |             |
|          | not_running |
+--------> |             | <+
           +-------------+  |
             |              |
             | switch_in    | switch_out
             v              |
           +-------------+  |
           |   running   | -+
           +-------------+
```

This model is broken, the reason is that a task can be running
in the processor without being set as RUNNABLE. Think about a
task about to sleep:

```
1:      set_current_state(TASK_UNINTERRUPTIBLE);
2:      schedule();
```

And then imagine an IRQ happening in between the lines one and two,
waking the task up. BOOM, the wakeup will happen while the task is
running.

* Why do we need this model, so?
* To test the reactors.

## Specification

Grapviz Dot file in tools/verification/models/wwnr.dot
