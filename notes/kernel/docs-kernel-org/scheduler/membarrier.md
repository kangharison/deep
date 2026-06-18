# membarrier() System Call

> 출처(원문): https://docs.kernel.org/scheduler/membarrier.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# membarrier() System Call

## MEMBARRIER\_CMD\_{PRIVATE,GLOBAL}\_EXPEDITED - Architecture requirements

### Memory barriers before updating rq->curr

The commands MEMBARRIER\_CMD\_PRIVATE\_EXPEDITED and MEMBARRIER\_CMD\_GLOBAL\_EXPEDITED
require each architecture to have a full memory barrier after coming from
user-space, before updating rq->curr. This barrier is implied by the sequence
`rq_lock()`; `smp_mb__after_spinlock()` in `__schedule()`. The barrier matches a full
barrier in the proximity of the membarrier system call exit, cf.
membarrier\_{private,global}`_expedited()`.

### Memory barriers after updating rq->curr

The commands MEMBARRIER\_CMD\_PRIVATE\_EXPEDITED and MEMBARRIER\_CMD\_GLOBAL\_EXPEDITED
require each architecture to have a full memory barrier after updating rq->curr,
before returning to user-space. The schemes providing this barrier on the various
architectures are as follows.

> * alpha, arc, arm, hexagon, mips rely on the full barrier implied by
>   `spin_unlock()` in `finish_lock_switch()`.
> * arm64 relies on the full barrier implied by `switch_to()`.
> * powerpc, riscv, s390, sparc, x86 rely on the full barrier implied by
>   `switch_mm()`, if mm is not NULL; they rely on the full barrier implied
>   by `mmdrop()`, otherwise. On powerpc and riscv, `switch_mm()` relies on
>   `membarrier_arch_switch_mm()`.

The barrier matches a full barrier in the proximity of the membarrier system call
entry, cf. membarrier\_{private,global}`_expedited()`.
