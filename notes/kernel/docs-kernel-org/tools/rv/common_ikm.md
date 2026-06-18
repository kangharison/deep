> 출처(원문): https://docs.kernel.org/tools/rv/common_ikm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

**-h**, **--help**

> Print the monitor’s options and the available reactors list.

**-r**, **--reactor** *reactor*

> Enables the *reactor*. See **-h** for a list of available reactors.

**-s**, **--self**

> When tracing (**-t**), also print the events that happened during the **rv**
> command itself. If the **rv** command itself generates too many events,
> the tool might get busy processing its own events only.

**-t**, **--trace**

> Trace monitor’s events and error.

**-v**, **--verbose**

> Print debug messages.
