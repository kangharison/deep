# Speculation Control

> 출처(원문): https://docs.kernel.org/userspace-api/spec_ctrl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Speculation Control

Quite some CPUs have speculation-related misfeatures which are in
fact vulnerabilities causing data leaks in various forms even across
privilege domains.

The kernel provides mitigation for such vulnerabilities in various
forms. Some of these mitigations are compile-time configurable and some
can be supplied on the kernel command line.

There is also a class of mitigations which are very expensive, but they can
be restricted to a certain set of processes or tasks in controlled
environments. The mechanism to control these mitigations is via
*[prctl(2)](https://man7.org/linux/man-pages/man2/prctl.2.html)*.

There are two prctl options which are related to this:

> * PR\_GET\_SPECULATION\_CTRL
> * PR\_SET\_SPECULATION\_CTRL

## PR\_GET\_SPECULATION\_CTRL

PR\_GET\_SPECULATION\_CTRL returns the state of the speculation misfeature
which is selected with arg2 of prctl(2). The return value uses bits 0-3 with
the following meaning (with the caveat that PR\_SPEC\_L1D\_FLUSH has less obvious
semantics, see documentation for that specific control below):

| Bit | Define | Description |
| --- | --- | --- |
| 0 | PR\_SPEC\_PRCTL | Mitigation can be controlled per task by PR\_SET\_SPECULATION\_CTRL. |
| 1 | PR\_SPEC\_ENABLE | The speculation feature is enabled, mitigation is disabled. |
| 2 | PR\_SPEC\_DISABLE | The speculation feature is disabled, mitigation is enabled. |
| 3 | PR\_SPEC\_FORCE\_DISABLE | Same as PR\_SPEC\_DISABLE, but cannot be undone. A subsequent prctl(..., PR\_SPEC\_ENABLE) will fail. |
| 4 | PR\_SPEC\_DISABLE\_NOEXEC | Same as PR\_SPEC\_DISABLE, but the state will be cleared on *[execve(2)](https://man7.org/linux/man-pages/man2/execve.2.html)*. |

If all bits are 0 the CPU is not affected by the speculation misfeature.

If PR\_SPEC\_PRCTL is set, then the per-task control of the mitigation is
available. If not set, prctl(PR\_SET\_SPECULATION\_CTRL) for the speculation
misfeature will fail.

## PR\_SET\_SPECULATION\_CTRL

PR\_SET\_SPECULATION\_CTRL allows to control the speculation misfeature, which
is selected by arg2 of *[prctl(2)](https://man7.org/linux/man-pages/man2/prctl.2.html)* per task. arg3 is used to hand
in the control value, i.e. either PR\_SPEC\_ENABLE or PR\_SPEC\_DISABLE or
PR\_SPEC\_FORCE\_DISABLE.

## Common error codes

| Value | Meaning |
| --- | --- |
| EINVAL | The prctl is not implemented by the architecture or unused prctl(2) arguments are not 0. |
| ENODEV | arg2 is selecting a not supported speculation misfeature. |

## PR\_SET\_SPECULATION\_CTRL error codes

| Value | Meaning |
| --- | --- |
| 0 | Success |
| ERANGE | arg3 is incorrect, i.e. it’s neither PR\_SPEC\_ENABLE nor PR\_SPEC\_DISABLE nor PR\_SPEC\_FORCE\_DISABLE. |
| ENXIO | For PR\_SPEC\_STORE\_BYPASS: control of the selected speculation misfeature is not possible via prctl, because of the system’s boot configuration. |
| EPERM | Speculation was disabled with PR\_SPEC\_FORCE\_DISABLE and caller tried to enable it again. |
| EPERM | For PR\_SPEC\_L1D\_FLUSH and PR\_SPEC\_INDIRECT\_BRANCH: control of the mitigation is not possible because of the system’s boot configuration. |

## Speculation misfeature controls

* PR\_SPEC\_STORE\_BYPASS: Speculative Store Bypass

  Invocations:
  :   + prctl(PR\_GET\_SPECULATION\_CTRL, PR\_SPEC\_STORE\_BYPASS, 0, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_STORE\_BYPASS, PR\_SPEC\_ENABLE, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_STORE\_BYPASS, PR\_SPEC\_DISABLE, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_STORE\_BYPASS, PR\_SPEC\_FORCE\_DISABLE, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_STORE\_BYPASS, PR\_SPEC\_DISABLE\_NOEXEC, 0, 0);
* PR\_SPEC\_INDIR\_BRANCH: Indirect Branch Speculation in User Processes
  :   (Mitigate Spectre V2 style attacks against user processes)

  Invocations:
  :   + prctl(PR\_GET\_SPECULATION\_CTRL, PR\_SPEC\_INDIRECT\_BRANCH, 0, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_INDIRECT\_BRANCH, PR\_SPEC\_ENABLE, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_INDIRECT\_BRANCH, PR\_SPEC\_DISABLE, 0, 0);
      + prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_INDIRECT\_BRANCH, PR\_SPEC\_FORCE\_DISABLE, 0, 0);
* PR\_SPEC\_L1D\_FLUSH: Flush L1D Cache on context switch out of the task
  :   (works only when tasks run on non SMT cores)

For this control, PR\_SPEC\_ENABLE means that the **mitigation** is enabled (L1D
is flushed), PR\_SPEC\_DISABLE means it is disabled.

> Invocations:
> :   * prctl(PR\_GET\_SPECULATION\_CTRL, PR\_SPEC\_L1D\_FLUSH, 0, 0, 0);
>     * prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_L1D\_FLUSH, PR\_SPEC\_ENABLE, 0, 0);
>     * prctl(PR\_SET\_SPECULATION\_CTRL, PR\_SPEC\_L1D\_FLUSH, PR\_SPEC\_DISABLE, 0, 0);
