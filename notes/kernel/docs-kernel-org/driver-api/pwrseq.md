# Power Sequencing API

> 출처(원문): https://docs.kernel.org/driver-api/pwrseq.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Power Sequencing API

Author:
:   Bartosz Golaszewski

## Introduction

This framework is designed to abstract complex power-up sequences that are
shared between multiple logical devices in the Linux kernel.

The intention is to allow consumers to obtain a power sequencing handle
exposed by the power sequence provider and delegate the actual requesting and
control of the underlying resources as well as to allow the provider to
mitigate any potential conflicts between multiple users behind the scenes.

### Glossary

The power sequencing API uses a number of terms specific to the subsystem:

Unit

> A unit is a discrete chunk of a power sequence. For instance one unit may
> enable a set of regulators, another may enable a specific GPIO. Units can
> define dependencies in the form of other units that must be enabled before
> it itself can be.

Target

> A target is a set of units (composed of the “final” unit and its
> dependencies) that a consumer selects by its name when requesting a handle
> to the power sequencer. Via the dependency system, multiple targets may
> share the same parts of a power sequence but ignore parts that are
> irrelevant.

Descriptor

> A handle passed by the pwrseq core to every consumer that serves as the
> entry point to the provider layer. It ensures coherence between different
> users and keeps reference counting consistent.

## Consumer interface

The consumer API is aimed to be as simple as possible. The driver interested in
getting a descriptor from the power sequencer should call [`pwrseq_get()`](#c.pwrseq_get "pwrseq_get") and
specify the name of the target it wants to reach in the sequence after calling
`pwrseq_power_up()`. The descriptor can be released by calling [`pwrseq_put()`](#c.pwrseq_put "pwrseq_put") and
the consumer can request the powering down of its target with
[`pwrseq_power_off()`](#c.pwrseq_power_off "pwrseq_power_off"). Note that there is no guarantee that [`pwrseq_power_off()`](#c.pwrseq_power_off "pwrseq_power_off")
will have any effect as there may be multiple users of the underlying resources
who may keep them active.

## Provider interface

The provider API is admittedly not nearly as straightforward as the one for
consumers but it makes up for it in flexibility.

Each provider can logically split the power-up sequence into discrete chunks
(units) and define their dependencies. They can then expose named targets that
consumers may use as the final point in the sequence that they wish to reach.

To that end the providers fill out a set of configuration structures and
register with the pwrseq subsystem by calling [`pwrseq_device_register()`](#c.pwrseq_device_register "pwrseq_device_register").

### Dynamic consumer matching

The main difference between pwrseq and other Linux kernel providers is the
mechanism for dynamic matching of consumers and providers. Every power sequence
provider driver must implement the `match()` callback and pass it to the pwrseq
core when registering with the subsystems.

When a client requests a sequencer handle, the core will call this callback for
every registered provider and let it flexibly figure out whether the proposed
client device is indeed its consumer. For example: if the provider binds to the
device-tree node representing a power management unit of a chipset and the
consumer driver controls one of its modules, the provider driver may parse the
relevant regulator supply properties in device tree and see if they lead from
the PMU to the consumer.

## API reference

struct pwrseq\_unit\_data
:   Configuration of a single power sequencing unit.

**Definition**:

```
struct pwrseq_unit_data {
    const char *name;
    const struct pwrseq_unit_data **deps;
    pwrseq_power_state_func enable;
    pwrseq_power_state_func disable;
};
```

**Members**

`name`
:   Name of the unit.

`deps`
:   Units that must be enabled before this one and disabled after it
    in the order they come in this array. Must be NULL-terminated.

`enable`
:   Callback running the part of the power-on sequence provided by
    this unit.

`disable`
:   Callback running the part of the power-off sequence provided
    by this unit.

struct pwrseq\_target\_data
:   Configuration of a power sequencing target.

**Definition**:

```
struct pwrseq_target_data {
    const char *name;
    const struct pwrseq_unit_data *unit;
    pwrseq_power_state_func post_enable;
};
```

**Members**

`name`
:   Name of the target.

`unit`
:   Final unit that this target must reach in order to be considered
    enabled.

`post_enable`
:   Callback run after the target unit has been enabled, *after*
    the state lock has been released. It’s useful for implementing
    boot-up delays without blocking other users from powering up
    using the same power sequencer.

struct pwrseq\_config
:   Configuration used for registering a new provider.

**Definition**:

```
struct pwrseq_config {
    struct device *parent;
    struct module *owner;
    void *drvdata;
    pwrseq_match_func match;
    const struct pwrseq_target_data **targets;
};
```

**Members**

`parent`
:   Parent device for the sequencer. Must be set.

`owner`
:   Module providing this device.

`drvdata`
:   Private driver data.

`match`
:   Provider callback used to match the consumer device to the sequencer.

`targets`
:   Array of targets for this power sequencer. Must be NULL-terminated.

struct pwrseq\_device \*pwrseq\_device\_register(const struct [pwrseq\_config](#c.pwrseq_config "pwrseq_config") \*config)
:   Register a new power sequencer.

**Parameters**

`const struct pwrseq_config *config`
:   Configuration of the new power sequencing device.

**Description**

The config structure is only used during the call and can be freed after
the function returns. The config structure *must* have the parent device
as well as the `match()` callback and at least one target set.

**Return**

Returns the address of the new pwrseq device or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on failure.

void pwrseq\_device\_unregister(struct pwrseq\_device \*pwrseq)
:   Unregister the power sequencer.

**Parameters**

`struct pwrseq_device *pwrseq`
:   Power sequencer to unregister.

struct pwrseq\_device \*devm\_pwrseq\_device\_register(struct [device](infrastructure.html#c.device "device") \*dev, const struct [pwrseq\_config](#c.pwrseq_config "pwrseq_config") \*config)
:   Managed variant of [`pwrseq_device_register()`](#c.pwrseq_device_register "pwrseq_device_register").

**Parameters**

`struct device *dev`
:   Managing device.

`const struct pwrseq_config *config`
:   Configuration of the new power sequencing device.

**Return**

Returns the address of the new pwrseq device or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on failure.

void \*pwrseq\_device\_get\_drvdata(struct pwrseq\_device \*pwrseq)
:   Get the driver private data associated with this sequencer.

**Parameters**

`struct pwrseq_device *pwrseq`
:   Power sequencer object.

**Return**

Address of the private driver data.

struct pwrseq\_desc \*pwrseq\_get(struct [device](infrastructure.html#c.device "device") \*dev, const char \*target)
:   Get the power sequencer associated with this device.

**Parameters**

`struct device *dev`
:   Device for which to get the sequencer.

`const char *target`
:   Name of the target exposed by the sequencer this device wants to
    reach.

**Return**

New power sequencer descriptor for use by the consumer driver or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")
on failure.

void pwrseq\_put(struct pwrseq\_desc \*desc)
:   Release the power sequencer descriptor.

**Parameters**

`struct pwrseq_desc *desc`
:   Descriptor to release.

struct pwrseq\_desc \*devm\_pwrseq\_get(struct [device](infrastructure.html#c.device "device") \*dev, const char \*target)
:   Managed variant of [`pwrseq_get()`](#c.pwrseq_get "pwrseq_get").

**Parameters**

`struct device *dev`
:   Device for which to get the sequencer and which also manages its
    lifetime.

`const char *target`
:   Name of the target exposed by the sequencer this device wants to
    reach.

**Return**

New power sequencer descriptor for use by the consumer driver or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")
on failure.

int pwrseq\_power\_on(struct pwrseq\_desc \*desc)
:   Issue a power-on request on behalf of the consumer device.

**Parameters**

`struct pwrseq_desc *desc`
:   Descriptor referencing the power sequencer.

**Description**

This function tells the power sequencer that the consumer wants to be
powered-up. The sequencer may already have powered-up the device in which
case the function returns 0. If the power-up sequence is already in
progress, the function will block until it’s done and return 0. If this is
the first request, the device will be powered up.

**Return**

0 on success, negative error number on failure.

int pwrseq\_power\_off(struct pwrseq\_desc \*desc)
:   Issue a power-off request on behalf of the consumer device.

**Parameters**

`struct pwrseq_desc *desc`
:   Descriptor referencing the power sequencer.

**Description**

This undoes the effects of [`pwrseq_power_on()`](#c.pwrseq_power_on "pwrseq_power_on"). It issues a power-off request
on behalf of the consumer and when the last remaining user does so, the
power-down sequence will be started. If one is in progress, the function
will block until it’s complete and then return.

**Return**

0 on success, negative error number on failure.
