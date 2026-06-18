# CPU Scheduler implementation hints for architecture specific code

> 출처(원문): https://docs.kernel.org/scheduler/sched-arch.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# CPU Scheduler implementation hints for architecture specific code

> Nick Piggin, 2005

## Context switch

1. Runqueue locking
By default, the switch\_to arch function is called with the runqueue
locked. This is usually not a problem unless switch\_to may need to
take the runqueue lock. This is usually due to a wake up operation in
the context switch.

To request the scheduler call switch\_to with the runqueue unlocked,
you must #define \_\_ARCH\_WANT\_UNLOCKED\_CTXSW in a header file
(typically the one where switch\_to is defined).

Unlocked context switches introduce only a very minor performance
penalty to the core scheduler implementation in the CONFIG\_SMP case.

## CPU idle

Your cpu\_idle routines need to obey the following rules:

1. Preempt should now disabled over idle routines. Should only
   be enabled to call `schedule()` then disabled again.
2. need\_resched/TIF\_NEED\_RESCHED is only ever set, and will never
   be cleared until the running task has called `schedule()`. Idle
   threads need only ever query need\_resched, and may never set or
   clear it.
3. When cpu\_idle finds (`need_resched()` == ‘true’), it should call
   `schedule()`. It should not call `schedule()` otherwise.
4. The only time interrupts need to be disabled when checking
   need\_resched is if we are about to sleep the processor until
   the next interrupt (this doesn’t provide any protection of
   need\_resched, it prevents losing an interrupt):

   > 4a. Common problem with this type of sleep appears to be:
   >
   > ```
   > local_irq_disable();
   > if (!need_resched()) {
   >         local_irq_enable();
   >         *** resched interrupt arrives here ***
   >         __asm__("sleep until next interrupt");
   > }
   > ```
5. TIF\_POLLING\_NRFLAG can be set by idle routines that do not
   need an interrupt to wake them up when need\_resched goes high.
   In other words, they must be periodically polling need\_resched,
   although it may be reasonable to do some background work or enter
   a low CPU priority.

   > * 5a. If TIF\_POLLING\_NRFLAG is set, and we do decide to enter
   >   an interrupt sleep, it needs to be cleared then a memory
   >   barrier issued (followed by a test of need\_resched with
   >   interrupts disabled, as explained in 3).

arch/x86/kernel/process.c has examples of both polling and
sleeping idle functions.

## Possible arch/ problems

Possible arch problems I found (and either tried to fix or didn’t):

sparc - IRQs on at this point(?), change local\_irq\_save to \_disable.
:   * TODO: needs secondary CPUs to disable preempt (See #1)
