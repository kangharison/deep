# Net DIM - Generic Network Dynamic Interrupt Moderation

> 출처(원문): https://docs.kernel.org/networking/net_dim.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Net DIM - Generic Network Dynamic Interrupt Moderation](#id1)

Author:
:   Tal Gilboa <[talgi@mellanox.com](mailto:talgi%40mellanox.com)>

## [Assumptions](#id2)

This document assumes the reader has basic knowledge in network drivers
and in general interrupt moderation.

## [Introduction](#id3)

Dynamic Interrupt Moderation (DIM) (in networking) refers to changing the
interrupt moderation configuration of a channel in order to optimize packet
processing. The mechanism includes an algorithm which decides if and how to
change moderation parameters for a channel, usually by performing an analysis on
runtime data sampled from the system. Net DIM is such a mechanism. In each
iteration of the algorithm, it analyses a given sample of the data, compares it
to the previous sample and if required, it can decide to change some of the
interrupt moderation configuration fields. The data sample is composed of data
bandwidth, the number of packets and the number of events. The time between
samples is also measured. Net DIM compares the current and the previous data and
returns an adjusted interrupt moderation configuration object. In some cases,
the algorithm might decide not to change anything. The configuration fields are
the minimum duration (microseconds) allowed between events and the maximum
number of wanted packets per event. The Net DIM algorithm ascribes importance to
increase bandwidth over reducing interrupt rate.

## [Net DIM Algorithm](#id4)

Each iteration of the Net DIM algorithm follows these steps:

1. Calculates new data sample.
2. Compares it to previous sample.
3. Makes a decision - suggests interrupt moderation configuration fields.
4. Applies a schedule work function, which applies suggested configuration.

The first two steps are straightforward, both the new and the previous data are
supplied by the driver registered to Net DIM. The previous data is the new data
supplied to the previous iteration. The comparison step checks the difference
between the new and previous data and decides on the result of the last step.
A step would result as “better” if bandwidth increases and as “worse” if
bandwidth reduces. If there is no change in bandwidth, the packet rate is
compared in a similar fashion - increase == “better” and decrease == “worse”.
In case there is no change in the packet rate as well, the interrupt rate is
compared. Here the algorithm tries to optimize for lower interrupt rate so an
increase in the interrupt rate is considered “worse” and a decrease is
considered “better”. Step #2 has an optimization for avoiding false results: it
only considers a difference between samples as valid if it is greater than a
certain percentage. Also, since Net DIM does not measure anything by itself, it
assumes the data provided by the driver is valid.

Step #3 decides on the suggested configuration based on the result from step #2
and the internal state of the algorithm. The states reflect the “direction” of
the algorithm: is it going left (reducing moderation), right (increasing
moderation) or standing still. Another optimization is that if a decision
to stay still is made multiple times, the interval between iterations of the
algorithm would increase in order to reduce calculation overhead. Also, after
“parking” on one of the most left or most right decisions, the algorithm may
decide to verify this decision by taking a step in the other direction. This is
done in order to avoid getting stuck in a “deep sleep” scenario. Once a
decision is made, an interrupt moderation configuration is selected from
the predefined profiles.

The last step is to notify the registered driver that it should apply the
suggested configuration. This is done by scheduling a work function, defined by
the Net DIM API and provided by the registered driver.

As you can see, Net DIM itself does not actively interact with the system. It
would have trouble making the correct decisions if the wrong data is supplied to
it and it would be useless if the work function would not apply the suggested
configuration. This does, however, allow the registered driver some room for
manoeuvre as it may provide partial data or ignore the algorithm suggestion
under some conditions.

## [Registering a Network Device to DIM](#id5)

Net DIM API exposes the main function [`net_dim()`](#c.net_dim "net_dim").
This function is the entry point to the Net
DIM algorithm and has to be called every time the driver would like to check if
it should change interrupt moderation parameters. The driver should provide two
data structures: [`struct dim`](#c.dim "dim") and
[`struct dim_sample`](#c.dim_sample "dim_sample"). [`struct dim`](#c.dim "dim")
describes the state of DIM for a specific object (RX queue, TX queue,
other queues, etc.). This includes the current selected profile, previous data
samples, the callback function provided by the driver and more.
[`struct dim_sample`](#c.dim_sample "dim_sample") describes a data sample,
which will be compared to the data sample stored in [`struct dim`](#c.dim "dim")
in order to decide on the algorithm’s next
step. The sample should include bytes, packets and interrupts, measured by
the driver.

In order to use Net DIM from a networking driver, the driver needs to call the
main [`net_dim()`](#c.net_dim "net_dim") function. The recommended method is to call [`net_dim()`](#c.net_dim "net_dim") on each
interrupt. Since Net DIM has a built-in moderation and it might decide to skip
iterations under certain conditions, there is no need to moderate the [`net_dim()`](#c.net_dim "net_dim")
calls as well. As mentioned above, the driver needs to provide an object of type
[`struct dim`](#c.dim "dim") to the [`net_dim()`](#c.net_dim "net_dim") function call. It is advised for
each entity using Net DIM to hold a [`struct dim`](#c.dim "dim") as part of its
data structure and use it as the main Net DIM API object.
The [`struct dim_sample`](#c.dim_sample "dim_sample") should hold the latest
bytes, packets and interrupts count. No need to perform any calculations, just
include the raw data.

The [`net_dim()`](#c.net_dim "net_dim") call itself does not return anything. Instead Net DIM relies on
the driver to provide a callback function, which is called when the algorithm
decides to make a change in the interrupt moderation parameters. This callback
will be scheduled and run in a separate thread in order not to add overhead to
the data flow. After the work is done, Net DIM algorithm needs to be set to
the proper state in order to move to the next iteration.

## [Example](#id6)

The following code demonstrates how to register a driver to Net DIM. The actual
usage is not complete but it should make the outline of the usage clear.

```
#include <linux/dim.h>

/* Callback for net DIM to schedule on a decision to change moderation */
void my_driver_do_dim_work(struct work_struct *work)
{
      /* Get struct dim from struct work_struct */
      struct dim *dim = container_of(work, struct dim,
                                     work);
      /* Do interrupt moderation related stuff */
      ...

      /* Signal net DIM work is done and it should move to next iteration */
      dim->state = DIM_START_MEASURE;
}

/* My driver's interrupt handler */
int my_driver_handle_interrupt(struct my_driver_entity *my_entity, ...)
{
      ...
      /* A struct to hold current measured data */
      struct dim_sample dim_sample;
      ...
      /* Initiate data sample struct with current data */
      dim_update_sample(my_entity->events,
                        my_entity->packets,
                        my_entity->bytes,
                        &dim_sample);
      /* Call net DIM */
      net_dim(&my_entity->dim, &dim_sample);
      ...
}

/* My entity's initialization function (my_entity was already allocated) */
int my_driver_init_my_entity(struct my_driver_entity *my_entity, ...)
{
      ...
      /* Initiate struct work_struct with my driver's callback function */
      INIT_WORK(&my_entity->dim.work, my_driver_do_dim_work);
      ...
}
```

## [Tuning DIM](#id7)

Net DIM serves a range of network devices and delivers excellent acceleration
benefits. Yet, it has been observed that some preset configurations of DIM may
not align seamlessly with the varying specifications of network devices, and
this discrepancy has been identified as a factor to the suboptimal performance
outcomes of DIM-enabled network devices, related to a mismatch in profiles.

To address this issue, Net DIM introduces a per-device control to modify and
access a device’s `rx-profile` and `tx-profile` parameters:
Assume that the target network device is named ethx, and ethx only declares
support for RX profile setting and supports modification of `usec` field
and `pkts` field (See the data structure:
[`struct dim_cq_moder`](#c.dim_cq_moder "dim_cq_moder")).

You can use ethtool to modify the current RX DIM profile where all
values are 64:

```
$ ethtool -C ethx rx-profile 1,1,n_2,2,n_3,n,n_n,4,n_n,n,n
```

`n` means do not modify this field, and `_` separates structure
elements of the profile array.

Querying the current profiles using:

```
$ ethtool -c ethx
...
rx-profile:
{.usec =   1, .pkts =   1, .comps = n/a,},
{.usec =   2, .pkts =   2, .comps = n/a,},
{.usec =   3, .pkts =  64, .comps = n/a,},
{.usec =  64, .pkts =   4, .comps = n/a,},
{.usec =  64, .pkts =  64, .comps = n/a,}
tx-profile:   n/a
```

If the network device does not support specific fields of DIM profiles,
the corresponding `n/a` will display. If the `n/a` field is being
modified, error messages will be reported.

## [Dynamic Interrupt Moderation (DIM) library API](#id8)

struct dim\_cq\_moder
:   Structure for CQ moderation values. Used for communications between DIM and its consumer.

**Definition**:

```
struct dim_cq_moder {
    u16 usec;
    u16 pkts;
    u16 comps;
    u8 cq_period_mode;
    struct rcu_head rcu;
};
```

**Members**

`usec`
:   CQ timer suggestion (by DIM)

`pkts`
:   CQ packet counter suggestion (by DIM)

`comps`
:   Completion counter

`cq_period_mode`
:   CQ period count mode (from CQE/EQE)

`rcu`
:   for asynchronous kfree\_rcu

struct dim\_irq\_moder
:   Structure for irq moderation information. Used to collect irq moderation related information.

**Definition**:

```
struct dim_irq_moder {
    u8 profile_flags;
    u8 coal_flags;
    u8 dim_rx_mode;
    u8 dim_tx_mode;
    struct dim_cq_moder *rx_profile;
    struct dim_cq_moder *tx_profile;
    void (*rx_dim_work)(struct work_struct *work);
    void (*tx_dim_work)(struct work_struct *work);
};
```

**Members**

`profile_flags`
:   DIM\_PROFILE\_\*

`coal_flags`
:   DIM\_COALESCE\_\* for Rx and Tx

`dim_rx_mode`
:   Rx DIM period count mode: CQE or EQE

`dim_tx_mode`
:   Tx DIM period count mode: CQE or EQE

`rx_profile`
:   DIM profile list for Rx

`tx_profile`
:   DIM profile list for Tx

`rx_dim_work`
:   Rx DIM worker scheduled by [`net_dim()`](#c.net_dim "net_dim")

`tx_dim_work`
:   Tx DIM worker scheduled by [`net_dim()`](#c.net_dim "net_dim")

struct dim\_sample
:   Structure for DIM sample data. Used for communications between DIM and its consumer.

**Definition**:

```
struct dim_sample {
    ktime_t time;
    u32 pkt_ctr;
    u32 byte_ctr;
    u16 event_ctr;
    u32 comp_ctr;
};
```

**Members**

`time`
:   Sample timestamp

`pkt_ctr`
:   Number of packets

`byte_ctr`
:   Number of bytes

`event_ctr`
:   Number of events

`comp_ctr`
:   Current completion counter

struct dim\_stats
:   Structure for DIM stats. Used for holding current measured rates.

**Definition**:

```
struct dim_stats {
    int ppms;
    int bpms;
    int epms;
    int cpms;
    int cpe_ratio;
};
```

**Members**

`ppms`
:   Packets per msec

`bpms`
:   Bytes per msec

`epms`
:   Events per msec

`cpms`
:   Completions per msec

`cpe_ratio`
:   Ratio of completions to events

struct dim
:   Main structure for dynamic interrupt moderation (DIM). Used for holding all information about a specific DIM instance.

**Definition**:

```
struct dim {
    u8 state;
    struct dim_stats prev_stats;
    struct dim_sample start_sample;
    struct dim_sample measuring_sample;
    struct work_struct work;
    void *priv;
    u8 profile_ix;
    u8 mode;
    u8 tune_state;
    u8 steps_right;
    u8 steps_left;
    u8 tired;
};
```

**Members**

`state`
:   Algorithm state (see below)

`prev_stats`
:   Measured rates from previous iteration (for comparison)

`start_sample`
:   Sampled data at start of current iteration

`measuring_sample`
:   A [`dim_sample`](#c.dim_sample "dim_sample") that is used to update the current events

`work`
:   Work to perform on action required

`priv`
:   A pointer to the struct that points to dim

`profile_ix`
:   Current moderation profile

`mode`
:   CQ period count mode

`tune_state`
:   Algorithm tuning state (see below)

`steps_right`
:   Number of steps taken towards higher moderation

`steps_left`
:   Number of steps taken towards lower moderation

`tired`
:   Parking depth counter

enum dim\_cq\_period\_mode
:   Modes for CQ period count

**Constants**

`DIM_CQ_PERIOD_MODE_START_FROM_EQE`
:   Start counting from EQE

`DIM_CQ_PERIOD_MODE_START_FROM_CQE`
:   Start counting from CQE (implies timer reset)

`DIM_CQ_PERIOD_NUM_MODES`
:   Number of modes

enum dim\_state
:   DIM algorithm states

**Constants**

`DIM_START_MEASURE`
:   This is the first iteration (also after applying a new profile)

`DIM_MEASURE_IN_PROGRESS`
:   Algorithm is already in progress - check if
    need to perform an action

`DIM_APPLY_NEW_PROFILE`
:   DIM consumer is currently applying a profile - no need to measure

**Description**

These will determine if the algorithm is in a valid state to start an iteration.

enum dim\_tune\_state
:   DIM algorithm tune states

**Constants**

`DIM_PARKING_ON_TOP`
:   Algorithm found a local top point - exit on significant difference

`DIM_PARKING_TIRED`
:   Algorithm found a deep top point - don’t exit if tired > 0

`DIM_GOING_RIGHT`
:   Algorithm is currently trying higher moderation levels

`DIM_GOING_LEFT`
:   Algorithm is currently trying lower moderation levels

**Description**

These will determine which action the algorithm should perform.

enum dim\_stats\_state
:   DIM algorithm statistics states

**Constants**

`DIM_STATS_WORSE`
:   Current iteration shows worse performance than before

`DIM_STATS_SAME`
:   Current iteration shows same performance than before

`DIM_STATS_BETTER`
:   Current iteration shows better performance than before

**Description**

These will determine the verdict of current iteration.

enum dim\_step\_result
:   DIM algorithm step results

**Constants**

`DIM_STEPPED`
:   Performed a regular step

`DIM_TOO_TIRED`
:   Same kind of step was done multiple times - should go to
    tired parking

`DIM_ON_EDGE`
:   Stepped to the most left/right profile

**Description**

These describe the result of a step.

int net\_dim\_init\_irq\_moder(struct [net\_device](kapi.html#c.net_device "net_device") \*dev, u8 profile\_flags, u8 coal\_flags, u8 rx\_mode, u8 tx\_mode, void (\*rx\_dim\_work)(struct work\_struct \*work), void (\*tx\_dim\_work)(struct work\_struct \*work))
:   collect information to initialize irq moderation

**Parameters**

`struct net_device *dev`
:   target network device

`u8 profile_flags`
:   Rx or Tx profile modification capability

`u8 coal_flags`
:   irq moderation params flags

`u8 rx_mode`
:   CQ period mode for Rx

`u8 tx_mode`
:   CQ period mode for Tx

`void (*rx_dim_work)(struct work_struct *work)`
:   Rx worker called after dim decision

`void (*tx_dim_work)(struct work_struct *work)`
:   Tx worker called after dim decision

**Return**

0 on success or a negative error code.

void net\_dim\_free\_irq\_moder(struct [net\_device](kapi.html#c.net_device "net_device") \*dev)
:   free fields for irq moderation

**Parameters**

`struct net_device *dev`
:   target network device

void net\_dim\_setting(struct [net\_device](kapi.html#c.net_device "net_device") \*dev, struct [dim](#c.net_dim_setting "dim") \*dim, bool is\_tx)
:   initialize DIM’s cq mode and schedule worker

**Parameters**

`struct net_device *dev`
:   target network device

`struct dim *dim`
:   DIM context

`bool is_tx`
:   true indicates the tx direction, false indicates the rx direction

void net\_dim\_work\_cancel(struct [dim](#c.net_dim_work_cancel "dim") \*dim)
:   synchronously cancel dim’s worker

**Parameters**

`struct dim *dim`
:   DIM context

struct [dim\_cq\_moder](#c.dim_cq_moder "dim_cq_moder") net\_dim\_get\_rx\_irq\_moder(struct [net\_device](kapi.html#c.net_device "net_device") \*dev, struct [dim](#c.net_dim_get_rx_irq_moder "dim") \*dim)
:   get DIM rx results based on profile\_ix

**Parameters**

`struct net_device *dev`
:   target network device

`struct dim *dim`
:   DIM context

**Return**

DIM irq moderation

struct [dim\_cq\_moder](#c.dim_cq_moder "dim_cq_moder") net\_dim\_get\_tx\_irq\_moder(struct [net\_device](kapi.html#c.net_device "net_device") \*dev, struct [dim](#c.net_dim_get_tx_irq_moder "dim") \*dim)
:   get DIM tx results based on profile\_ix

**Parameters**

`struct net_device *dev`
:   target network device

`struct dim *dim`
:   DIM context

**Return**

DIM irq moderation

void net\_dim\_set\_rx\_mode(struct [net\_device](kapi.html#c.net_device "net_device") \*dev, u8 rx\_mode)
:   set DIM rx cq mode

**Parameters**

`struct net_device *dev`
:   target network device

`u8 rx_mode`
:   target rx cq mode

void net\_dim\_set\_tx\_mode(struct [net\_device](kapi.html#c.net_device "net_device") \*dev, u8 tx\_mode)
:   set DIM tx cq mode

**Parameters**

`struct net_device *dev`
:   target network device

`u8 tx_mode`
:   target tx cq mode

bool dim\_on\_top(struct [dim](#c.dim_on_top "dim") \*dim)
:   check if current state is a good place to stop (top location)

**Parameters**

`struct dim *dim`
:   DIM context

**Description**

Check if current profile is a good place to park at.
This will result in reducing the DIM checks frequency as we assume we
shouldn’t probably change profiles, unless traffic pattern wasn’t changed.

void dim\_turn(struct [dim](#c.dim_turn "dim") \*dim)
:   change profile altering direction

**Parameters**

`struct dim *dim`
:   DIM context

**Description**

Go left if we were going right and vice-versa.
Do nothing if currently parking.

void dim\_park\_on\_top(struct [dim](#c.dim_park_on_top "dim") \*dim)
:   enter a parking state on a top location

**Parameters**

`struct dim *dim`
:   DIM context

**Description**

Enter parking state.
Clear all movement history.

void dim\_park\_tired(struct [dim](#c.dim_park_tired "dim") \*dim)
:   enter a tired parking state

**Parameters**

`struct dim *dim`
:   DIM context

**Description**

Enter parking state.
Clear all movement history and cause DIM checks frequency to reduce.

bool dim\_calc\_stats(const struct [dim\_sample](#c.dim_sample "dim_sample") \*start, const struct [dim\_sample](#c.dim_sample "dim_sample") \*end, struct [dim\_stats](#c.dim_stats "dim_stats") \*curr\_stats)
:   calculate the difference between two samples

**Parameters**

`const struct dim_sample *start`
:   start sample

`const struct dim_sample *end`
:   end sample

`struct dim_stats *curr_stats`
:   delta between samples

**Description**

Calculate the delta between two samples (in data rates).
Takes into consideration counter wrap-around.
Returned boolean indicates whether curr\_stats are reliable.

void dim\_update\_sample(u16 event\_ctr, u64 packets, u64 bytes, struct [dim\_sample](#c.dim_sample "dim_sample") \*s)
:   set a sample’s fields with given values

**Parameters**

`u16 event_ctr`
:   number of events to set

`u64 packets`
:   number of packets to set

`u64 bytes`
:   number of bytes to set

`struct dim_sample *s`
:   DIM sample

void dim\_update\_sample\_with\_comps(u16 event\_ctr, u64 packets, u64 bytes, u64 comps, struct [dim\_sample](#c.dim_sample "dim_sample") \*s)
:   set a sample’s fields with given values including the completion parameter

**Parameters**

`u16 event_ctr`
:   number of events to set

`u64 packets`
:   number of packets to set

`u64 bytes`
:   number of bytes to set

`u64 comps`
:   number of completions to set

`struct dim_sample *s`
:   DIM sample

struct [dim\_cq\_moder](#c.dim_cq_moder "dim_cq_moder") net\_dim\_get\_rx\_moderation(u8 cq\_period\_mode, int ix)
:   provide a CQ moderation object for the given RX profile

**Parameters**

`u8 cq_period_mode`
:   CQ period mode

`int ix`
:   Profile index

struct [dim\_cq\_moder](#c.dim_cq_moder "dim_cq_moder") net\_dim\_get\_def\_rx\_moderation(u8 cq\_period\_mode)
:   provide the default RX moderation

**Parameters**

`u8 cq_period_mode`
:   CQ period mode

struct [dim\_cq\_moder](#c.dim_cq_moder "dim_cq_moder") net\_dim\_get\_tx\_moderation(u8 cq\_period\_mode, int ix)
:   provide a CQ moderation object for the given TX profile

**Parameters**

`u8 cq_period_mode`
:   CQ period mode

`int ix`
:   Profile index

struct [dim\_cq\_moder](#c.dim_cq_moder "dim_cq_moder") net\_dim\_get\_def\_tx\_moderation(u8 cq\_period\_mode)
:   provide the default TX moderation

**Parameters**

`u8 cq_period_mode`
:   CQ period mode

void net\_dim(struct [dim](#c.net_dim "dim") \*dim, const struct [dim\_sample](#c.dim_sample "dim_sample") \*end\_sample)
:   main DIM algorithm entry point

**Parameters**

`struct dim *dim`
:   DIM instance information

`const struct dim_sample *end_sample`
:   Current data measurement

**Description**

Called by the consumer.
This is the main logic of the algorithm, where data is processed in order
to decide on next required action.

void rdma\_dim(struct [dim](#c.rdma_dim "dim") \*dim, u64 completions)
:   Runs the adaptive moderation.

**Parameters**

`struct dim *dim`
:   The moderation struct.

`u64 completions`
:   The number of completions collected in this round.

**Description**

Each call to rdma\_dim takes the latest amount of completions that
have been collected and counts them as a new event.
Once enough events have been collected the algorithm decides a new
moderation level.
