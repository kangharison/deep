# Delay and sleep mechanisms

> 출처(원문): https://docs.kernel.org/timers/delay_sleep_functions.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Delay and sleep mechanisms

This document seeks to answer the common question: “What is the
RightWay (TM) to insert a delay?”

This question is most often faced by driver writers who have to
deal with hardware delays and who may not be the most intimately
familiar with the inner workings of the Linux Kernel.

The following table gives a rough overview about the existing function
‘families’ and their limitations. This overview table does not replace the
reading of the function description before usage!

|  | \*`delay()` | usleep\_range\*() | \*`sleep()` | [`fsleep()`](#c.fsleep "fsleep") |
| --- | --- | --- | --- | --- |
|  | busy-wait loop | hrtimers based | timer list timers based | combines the others |
| Usage in atomic Context | yes | no | no | no |
| precise on “short intervals” | yes | yes | depends | yes |
| precise on “long intervals” | Do not use! | yes | max 12.5% slack | yes |
| interruptible variant | no | yes | yes | no |

A generic advice for non atomic contexts could be:

1. Use [`fsleep()`](#c.fsleep "fsleep") whenever unsure (as it combines all the advantages of the
   others)
2. Use \*`sleep()` whenever possible
3. Use usleep\_range\*() whenever accuracy of \*`sleep()` is not sufficient
4. Use \*`delay()` for very, very short delays

Find some more detailed information about the function ‘families’ in the next
sections.

## \*delay() family of functions

These functions use the jiffy estimation of clock speed and will busy wait for
enough loop cycles to achieve the desired delay. [`udelay()`](#c.udelay "udelay") is the basic
implementation and [`ndelay()`](#c.ndelay "ndelay") as well as [`mdelay()`](#c.mdelay "mdelay") are variants.

These functions are mainly used to add a delay in atomic context. Please make
sure to ask yourself before adding a delay in atomic context: Is this really
required?

void udelay(unsigned long usec)
:   Inserting a delay based on microseconds with busy waiting

**Parameters**

`unsigned long usec`
:   requested delay in microseconds

**Description**

When delaying in an atomic context [`ndelay()`](#c.ndelay "ndelay"), [`udelay()`](#c.udelay "udelay") and [`mdelay()`](#c.mdelay "mdelay") are the
only valid variants of delaying/sleeping to go with.

When inserting delays in non atomic context which are shorter than the time
which is required to queue e.g. an hrtimer and to enter then the scheduler,
it is also valuable to use [`udelay()`](#c.udelay "udelay"). But it is not simple to specify a
generic threshold for this which will fit for all systems. An approximation
is a threshold for all delays up to 10 microseconds.

When having a delay which is larger than the architecture specific
`MAX_UDELAY_MS` value, please make sure [`mdelay()`](#c.mdelay "mdelay") is used. Otherwise a overflow
risk is given.

Please note that [`ndelay()`](#c.ndelay "ndelay"), [`udelay()`](#c.udelay "udelay") and [`mdelay()`](#c.mdelay "mdelay") may return early for several
reasons (<https://lists.openwall.net/linux-kernel/2011/01/09/56>):

1. computed loops\_per\_jiffy too low (due to the time taken to execute the
   timer interrupt.)
2. cache behaviour affecting the time it takes to execute the loop function.
3. CPU clock rate changes.

void ndelay(unsigned long nsec)
:   Inserting a delay based on nanoseconds with busy waiting

**Parameters**

`unsigned long nsec`
:   requested delay in nanoseconds

**Description**

See [`udelay()`](#c.udelay "udelay") for basic information about [`ndelay()`](#c.ndelay "ndelay") and it’s variants.

mdelay

`mdelay (n)`

> Inserting a delay based on milliseconds with busy waiting

**Parameters**

`n`
:   requested delay in milliseconds

**Description**

See [`udelay()`](#c.udelay "udelay") for basic information about [`mdelay()`](#c.mdelay "mdelay") and it’s variants.

Please double check, whether [`mdelay()`](#c.mdelay "mdelay") is the right way to go or whether a
refactoring of the code is the better variant to be able to use [`msleep()`](#c.msleep "msleep")
instead.

## usleep\_range\*() and \*sleep() family of functions

These functions use hrtimers or timer list timers to provide the requested
sleeping duration. In order to decide which function is the right one to use,
take some basic information into account:

1. hrtimers are more expensive as they are using an rb-tree (instead of hashing)
2. hrtimers are more expensive when the requested sleeping duration is the first
   timer which means real hardware has to be programmed
3. timer list timers always provide some sort of slack as they are jiffy based

The generic advice is repeated here:

1. Use [`fsleep()`](#c.fsleep "fsleep") whenever unsure (as it combines all the advantages of the
   others)
2. Use \*`sleep()` whenever possible
3. Use usleep\_range\*() whenever accuracy of \*`sleep()` is not sufficient

First check [`fsleep()`](#c.fsleep "fsleep") function description and to learn more about accuracy,
please check [`msleep()`](#c.msleep "msleep") function description.

### usleep\_range\*()

void usleep\_range(unsigned long min, unsigned long max)
:   Sleep for an approximate time

**Parameters**

`unsigned long min`
:   Minimum time in microseconds to sleep

`unsigned long max`
:   Maximum time in microseconds to sleep

**Description**

For basic information please refer to [`usleep_range_state()`](#c.usleep_range_state "usleep_range_state").

The task will be in the state TASK\_UNINTERRUPTIBLE during the sleep.

void usleep\_range\_idle(unsigned long min, unsigned long max)
:   Sleep for an approximate time with idle time accounting

**Parameters**

`unsigned long min`
:   Minimum time in microseconds to sleep

`unsigned long max`
:   Maximum time in microseconds to sleep

**Description**

For basic information please refer to [`usleep_range_state()`](#c.usleep_range_state "usleep_range_state").

The sleeping task has the state TASK\_IDLE during the sleep to prevent
contribution to the load average.

void usleep\_range\_state(unsigned long min, unsigned long max, unsigned int state)
:   Sleep for an approximate time in a given state

**Parameters**

`unsigned long min`
:   Minimum time in usecs to sleep

`unsigned long max`
:   Maximum time in usecs to sleep

`unsigned int state`
:   State of the current task that will be while sleeping

**Description**

[`usleep_range_state()`](#c.usleep_range_state "usleep_range_state") sleeps at least for the minimum specified time but not
longer than the maximum specified amount of time. The range might reduce
power usage by allowing hrtimers to coalesce an already scheduled interrupt
with this hrtimer. In the worst case, an interrupt is scheduled for the upper
bound.

The sleeping task is set to the specified state before starting the sleep.

In non-atomic context where the exact wakeup time is flexible, use
[`usleep_range()`](#c.usleep_range "usleep_range") or its variants instead of [`udelay()`](#c.udelay "udelay"). The sleep improves
responsiveness by avoiding the CPU-hogging busy-wait of [`udelay()`](#c.udelay "udelay").

### \*sleep()

void msleep(unsigned int msecs)
:   sleep safely even with waitqueue interruptions

**Parameters**

`unsigned int msecs`
:   Requested sleep duration in milliseconds

**Description**

[`msleep()`](#c.msleep "msleep") uses jiffy based timeouts for the sleep duration. Because of the
design of the timer wheel, the maximum additional percentage delay (slack) is
12.5%. This is only valid for timers which will end up in level 1 or a higher
level of the timer wheel. For explanation of those 12.5% please check the
detailed description about the basics of the timer wheel.

The slack of timers which will end up in level 0 depends on sleep duration
(msecs) and HZ configuration and can be calculated in the following way (with
the timer wheel design restriction that the slack is not less than 12.5%):

> `slack = MSECS_PER_TICK / msecs`

When the allowed slack of the callsite is known, the calculation could be
turned around to find the minimal allowed sleep duration to meet the
constraints. For example:

* `HZ=1000` with `slack=25%`: `MSECS_PER_TICK / slack = 1 / (1/4) = 4`:
  all sleep durations greater or equal 4ms will meet the constraints.
* `HZ=1000` with `slack=12.5%`: `MSECS_PER_TICK / slack = 1 / (1/8) = 8`:
  all sleep durations greater or equal 8ms will meet the constraints.
* `HZ=250` with `slack=25%`: `MSECS_PER_TICK / slack = 4 / (1/4) = 16`:
  all sleep durations greater or equal 16ms will meet the constraints.
* `HZ=250` with `slack=12.5%`: `MSECS_PER_TICK / slack = 4 / (1/8) = 32`:
  all sleep durations greater or equal 32ms will meet the constraints.

See also the signal aware variant [`msleep_interruptible()`](#c.msleep_interruptible "msleep_interruptible").

unsigned long msleep\_interruptible(unsigned int msecs)
:   sleep waiting for signals

**Parameters**

`unsigned int msecs`
:   Requested sleep duration in milliseconds

**Description**

See [`msleep()`](#c.msleep "msleep") for some basic information.

The difference between [`msleep()`](#c.msleep "msleep") and [`msleep_interruptible()`](#c.msleep_interruptible "msleep_interruptible") is that the sleep
could be interrupted by a signal delivery and then returns early.

**Return**

The remaining time of the sleep duration transformed to msecs (see
`schedule_timeout()` for details).

void ssleep(unsigned int seconds)
:   wrapper for seconds around msleep

**Parameters**

`unsigned int seconds`
:   Requested sleep duration in seconds

**Description**

Please refer to [`msleep()`](#c.msleep "msleep") for detailed information.

void fsleep(unsigned long usecs)
:   flexible sleep which autoselects the best mechanism

**Parameters**

`unsigned long usecs`
:   requested sleep duration in microseconds

**Description**

`flseep()` selects the best mechanism that will provide maximum 25% slack
to the requested sleep duration. Therefore it uses:

* [`udelay()`](#c.udelay "udelay") loop for sleep durations <= 10 microseconds to avoid hrtimer
  overhead for really short sleep durations.
* [`usleep_range()`](#c.usleep_range "usleep_range") for sleep durations which would lead with the usage of
  [`msleep()`](#c.msleep "msleep") to a slack larger than 25%. This depends on the granularity of
  jiffies.
* [`msleep()`](#c.msleep "msleep") for all other sleep durations.

**Note**

When `CONFIG_HIGH_RES_TIMERS` is not set, all sleeps are processed with
the granularity of jiffies and the slack might exceed 25% especially for
short sleep durations.
