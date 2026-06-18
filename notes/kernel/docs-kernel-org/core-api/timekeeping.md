# ktime accessors

> 출처(원문): https://docs.kernel.org/core-api/timekeeping.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ktime accessors

Device drivers can read the current time using [`ktime_get()`](#c.ktime_get "ktime_get") and the many
related functions declared in linux/timekeeping.h. As a rule of thumb,
using an accessor with a shorter name is preferred over one with a longer
name if both are equally fit for a particular use case.

## Basic ktime\_t based interfaces

The recommended simplest form returns an opaque ktime\_t, with variants
that return time for different clock references:

ktime\_t ktime\_get(void)
:   CLOCK\_MONOTONIC

    Useful for reliable timestamps and measuring short time intervals
    accurately. Starts at system boot time but stops during suspend.

ktime\_t ktime\_get\_boottime(void)
:   CLOCK\_BOOTTIME

    Like [`ktime_get()`](#c.ktime_get "ktime_get"), but does not stop when suspended. This can be
    used e.g. for key expiration times that need to be synchronized
    with other machines across a suspend operation.

ktime\_t ktime\_get\_real(void)
:   CLOCK\_REALTIME

    Returns the time in relative to the UNIX epoch starting in 1970
    using the Coordinated Universal Time (UTC), same as `gettimeofday()`
    user space. This is used for all timestamps that need to
    persist across a reboot, like inode times, but should be avoided
    for internal uses, since it can jump backwards due to a leap
    second update, NTP adjustment `settimeofday()` operation from user
    space.

ktime\_t ktime\_get\_clocktai(void)
:   > CLOCK\_TAI

    Like [`ktime_get_real()`](#c.ktime_get_real "ktime_get_real"), but uses the International Atomic Time (TAI)
    reference instead of UTC to avoid jumping on leap second updates.
    This is rarely useful in the kernel.

ktime\_t ktime\_get\_raw(void)
:   CLOCK\_MONOTONIC\_RAW

    Like [`ktime_get()`](#c.ktime_get "ktime_get"), but runs at the same rate as the hardware
    clocksource without (NTP) adjustments for clock drift. This is
    also rarely needed in the kernel.

## nanosecond, timespec64, and second output

For all of the above, there are variants that return the time in a
different format depending on what is required by the user:

u64 ktime\_get\_ns(void)

u64 ktime\_get\_boottime\_ns(void)

u64 ktime\_get\_real\_ns(void)

u64 ktime\_get\_clocktai\_ns(void)

u64 ktime\_get\_raw\_ns(void)
:   Same as the plain ktime\_get functions, but returning a u64 number
    of nanoseconds in the respective time reference, which may be
    more convenient for some callers.

void ktime\_get\_ts64(struct timespec64\*)

void ktime\_get\_boottime\_ts64(struct timespec64\*)

void ktime\_get\_real\_ts64(struct timespec64\*)

void ktime\_get\_clocktai\_ts64(struct timespec64\*)

void ktime\_get\_raw\_ts64(struct timespec64\*)
:   Same above, but returns the time in a ‘`struct timespec64`’, split
    into seconds and nanoseconds. This can avoid an extra division
    when printing the time, or when passing it into an external
    interface that expects a ‘timespec’ or ‘timeval’ structure.

time64\_t ktime\_get\_seconds(void)

time64\_t ktime\_get\_boottime\_seconds(void)

time64\_t ktime\_get\_real\_seconds(void)

time64\_t ktime\_get\_clocktai\_seconds(void)

time64\_t ktime\_get\_raw\_seconds(void)
:   Return a coarse-grained version of the time as a scalar
    time64\_t. This avoids accessing the clock hardware and rounds
    down the seconds to the full seconds of the last timer tick
    using the respective reference.

## Coarse and fast\_ns access

Some additional variants exist for more specialized cases:

ktime\_t ktime\_get\_coarse(void)

ktime\_t ktime\_get\_coarse\_boottime(void)

ktime\_t ktime\_get\_coarse\_real(void)

ktime\_t ktime\_get\_coarse\_clocktai(void)

u64 ktime\_get\_coarse\_ns(void)

u64 ktime\_get\_coarse\_boottime\_ns(void)

u64 ktime\_get\_coarse\_real\_ns(void)

u64 ktime\_get\_coarse\_clocktai\_ns(void)

void ktime\_get\_coarse\_ts64(struct timespec64\*)

void ktime\_get\_coarse\_boottime\_ts64(struct timespec64\*)

void ktime\_get\_coarse\_real\_ts64(struct timespec64\*)

void ktime\_get\_coarse\_clocktai\_ts64(struct timespec64\*)
:   These are quicker than the non-coarse versions, but less accurate,
    corresponding to CLOCK\_MONOTONIC\_COARSE and CLOCK\_REALTIME\_COARSE
    in user space, along with the equivalent boottime/tai/raw
    timebase not available in user space.

    The time returned here corresponds to the last timer tick, which
    may be as much as 10ms in the past (for CONFIG\_HZ=100), same as
    reading the ‘jiffies’ variable. These are only useful when called
    in a fast path and one still expects better than second accuracy,
    but can’t easily use ‘jiffies’, e.g. for inode timestamps.
    Skipping the hardware clock access saves around 100 CPU cycles
    on most modern machines with a reliable cycle counter, but
    up to several microseconds on older hardware with an external
    clocksource.

u64 ktime\_get\_mono\_fast\_ns(void)

u64 ktime\_get\_raw\_fast\_ns(void)

u64 ktime\_get\_boot\_fast\_ns(void)

u64 ktime\_get\_tai\_fast\_ns(void)

u64 ktime\_get\_real\_fast\_ns(void)
:   These variants are safe to call from any context, including from
    a non-maskable interrupt (NMI) during a timekeeper update, and
    while we are entering suspend with the clocksource powered down.
    This is useful in some tracing or debugging code as well as
    machine check reporting, but most drivers should never call them,
    since the time is allowed to jump under certain conditions.

## Deprecated time interfaces

Older kernels used some other interfaces that are now being phased out
but may appear in third-party drivers being ported here. In particular,
all interfaces returning a ‘`struct timeval`’ or ‘`struct timespec`’ have
been replaced because the tv\_sec member overflows in year 2038 on 32-bit
architectures. These are the recommended replacements:

void ktime\_get\_ts(struct timespec\*)
:   Use [`ktime_get()`](#c.ktime_get "ktime_get") or [`ktime_get_ts64()`](#c.ktime_get_ts64 "ktime_get_ts64") instead.

void do\_gettimeofday(struct timeval\*)

void getnstimeofday(struct timespec\*)

void getnstimeofday64(struct timespec64\*)

void ktime\_get\_real\_ts(struct timespec\*)
:   [`ktime_get_real_ts64()`](#c.ktime_get_real_ts64 "ktime_get_real_ts64") is a direct replacement, but consider using
    monotonic time ([`ktime_get_ts64()`](#c.ktime_get_ts64 "ktime_get_ts64")) and/or a ktime\_t based interface
    ([`ktime_get()`](#c.ktime_get "ktime_get")/[`ktime_get_real()`](#c.ktime_get_real "ktime_get_real")).

struct timespec current\_kernel\_time(void)

struct timespec64 current\_kernel\_time64(void)

struct timespec get\_monotonic\_coarse(void)

struct timespec64 get\_monotonic\_coarse64(void)
:   These are replaced by [`ktime_get_coarse_real_ts64()`](#c.ktime_get_coarse_real_ts64 "ktime_get_coarse_real_ts64") and
    [`ktime_get_coarse_ts64()`](#c.ktime_get_coarse_ts64 "ktime_get_coarse_ts64"). However, A lot of code that wants
    coarse-grained times can use the simple ‘jiffies’ instead, while
    some drivers may actually want the higher resolution accessors
    these days.

struct timespec getrawmonotonic(void)

struct timespec64 getrawmonotonic64(void)

struct timespec timekeeping\_clocktai(void)

struct timespec64 timekeeping\_clocktai64(void)

struct timespec get\_monotonic\_boottime(void)

struct timespec64 get\_monotonic\_boottime64(void)
:   These are replaced by [`ktime_get_raw()`](#c.ktime_get_raw "ktime_get_raw")/[`ktime_get_raw_ts64()`](#c.ktime_get_raw_ts64 "ktime_get_raw_ts64"),
    [`ktime_get_clocktai()`](#c.ktime_get_clocktai "ktime_get_clocktai")/[`ktime_get_clocktai_ts64()`](#c.ktime_get_clocktai_ts64 "ktime_get_clocktai_ts64") as well
    as [`ktime_get_boottime()`](#c.ktime_get_boottime "ktime_get_boottime")/[`ktime_get_boottime_ts64()`](#c.ktime_get_boottime_ts64 "ktime_get_boottime_ts64").
    However, if the particular choice of clock source is not
    important for the user, consider converting to
    [`ktime_get()`](#c.ktime_get "ktime_get")/[`ktime_get_ts64()`](#c.ktime_get_ts64 "ktime_get_ts64") instead for consistency.
