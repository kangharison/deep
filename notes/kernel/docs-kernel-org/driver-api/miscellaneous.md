# Parallel Port Devices

> 출처(원문): https://docs.kernel.org/driver-api/miscellaneous.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Parallel Port Devices

parport\_register\_driver

`parport_register_driver (driver)`

> register a parallel port device driver

**Parameters**

`driver`
:   structure describing the driver

**Description**

> This can be called by a parallel port device driver in order
> to receive notifications about ports being found in the
> system, as well as ports no longer available.
>
> The **driver** structure is allocated by the caller and must not be
> deallocated until after calling [`parport_unregister_driver()`](#c.parport_unregister_driver "parport_unregister_driver").
>
> If using the non device model:
> The driver’s `attach()` function may block. The port that
> `attach()` is given will be valid for the duration of the
> callback, but if the driver wants to take a copy of the
> pointer it must call [`parport_get_port()`](#c.parport_get_port "parport_get_port") to do so. Calling
> `parport_register_device()` on that port will do this for you.
>
> The driver’s `detach()` function may block. The port that
> `detach()` is given will be valid for the duration of the
> callback, but if the driver wants to take a copy of the
> pointer it must call [`parport_get_port()`](#c.parport_get_port "parport_get_port") to do so.
>
> Returns 0 on success. The non device model will always succeeds.
> but the new device model can fail and will return the error code.

module\_parport\_driver

`module_parport_driver (__parport_driver)`

> Helper macro for registering a modular parport driver

**Parameters**

`__parport_driver`
:   `struct parport_driver` to be used

**Description**

Helper macro for parport drivers which do not do anything special in module
init and exit. This eliminates a lot of boilerplate. Each module may only
use this macro once, and calling it replaces [`module_init()`](basics.html#c.module_init "module_init") and [`module_exit()`](basics.html#c.module_exit "module_exit").

int parport\_yield(struct pardevice \*dev)
:   relinquish a parallel port temporarily

**Parameters**

`struct pardevice *dev`
:   a device on the parallel port

**Description**

This function relinquishes the port if it would be helpful to other
drivers to do so. Afterwards it tries to reclaim the port using
[`parport_claim()`](#c.parport_claim "parport_claim"), and the return value is the same as for
[`parport_claim()`](#c.parport_claim "parport_claim"). If it fails, the port is left unclaimed and it is
the driver’s responsibility to reclaim the port.

The [`parport_yield()`](#c.parport_yield "parport_yield") and [`parport_yield_blocking()`](#c.parport_yield_blocking "parport_yield_blocking") functions are for
marking points in the driver at which other drivers may claim the
port and use their devices. Yielding the port is similar to
releasing it and reclaiming it, but is more efficient because no
action is taken if there are no other devices needing the port. In
fact, nothing is done even if there are other devices waiting but
the current device is still within its “timeslice”. The default
timeslice is half a second, but it can be adjusted via the /proc
interface.

int parport\_yield\_blocking(struct pardevice \*dev)
:   relinquish a parallel port temporarily

**Parameters**

`struct pardevice *dev`
:   a device on the parallel port

**Description**

This function relinquishes the port if it would be helpful to other
drivers to do so. Afterwards it tries to reclaim the port using
[`parport_claim_or_block()`](#c.parport_claim_or_block "parport_claim_or_block"), and the return value is the same as for
[`parport_claim_or_block()`](#c.parport_claim_or_block "parport_claim_or_block").

int parport\_wait\_event(struct parport \*port, signed long timeout)
:   wait for an event on a parallel port

**Parameters**

`struct parport *port`
:   port to wait on

`signed long timeout`
:   time to wait (in jiffies)

**Description**

> This function waits for up to **timeout** jiffies for an
> interrupt to occur on a parallel port. If the port timeout is
> set to zero, it returns immediately.
>
> If an interrupt occurs before the timeout period elapses, this
> function returns zero immediately. If it times out, it returns
> one. An error code less than zero indicates an error (most
> likely a pending signal), and the calling code should finish
> what it’s doing as soon as it can.

int parport\_wait\_peripheral(struct parport \*port, unsigned char mask, unsigned char result)
:   wait for status lines to change in 35ms

**Parameters**

`struct parport *port`
:   port to watch

`unsigned char mask`
:   status lines to watch

`unsigned char result`
:   desired values of chosen status lines

**Description**

> This function waits until the masked status lines have the
> desired values, or until 35ms have elapsed (see IEEE 1284-1994
> page 24 to 25 for why this value in particular is hardcoded).
> The **mask** and **result** parameters are bitmasks, with the bits
> defined by the constants in parport.h: `PARPORT_STATUS_BUSY`,
> and so on.
>
> The port is polled quickly to start off with, in anticipation
> of a fast response from the peripheral. This fast polling
> time is configurable (using /proc), and defaults to 500usec.
> If the timeout for this port (see [`parport_set_timeout()`](#c.parport_set_timeout "parport_set_timeout")) is
> zero, the fast polling time is 35ms, and this function does
> not call `schedule()`.
>
> If the timeout for this port is non-zero, after the fast
> polling fails it uses [`parport_wait_event()`](#c.parport_wait_event "parport_wait_event") to wait for up to
> 10ms, waking up if an interrupt occurs.

int parport\_negotiate(struct parport \*port, int mode)
:   negotiate an IEEE 1284 mode

**Parameters**

`struct parport *port`
:   port to use

`int mode`
:   mode to negotiate to

**Description**

> Use this to negotiate to a particular IEEE 1284 transfer mode.
> The **mode** parameter should be one of the constants in
> parport.h starting `IEEE1284_MODE_xxx`.
>
> The return value is 0 if the peripheral has accepted the
> negotiation to the mode specified, -1 if the peripheral is not
> IEEE 1284 compliant (or not present), or 1 if the peripheral
> has rejected the negotiation.

ssize\_t parport\_write(struct parport \*port, const void \*buffer, size\_t len)
:   write a block of data to a parallel port

**Parameters**

`struct parport *port`
:   port to write to

`const void *buffer`
:   data buffer (in kernel space)

`size_t len`
:   number of bytes of data to transfer

**Description**

> This will write up to **len** bytes of **buffer** to the port
> specified, using the IEEE 1284 transfer mode most recently
> negotiated to (using [`parport_negotiate()`](#c.parport_negotiate "parport_negotiate")), as long as that
> mode supports forward transfers (host to peripheral).
>
> It is the caller’s responsibility to ensure that the first
> **len** bytes of **buffer** are valid.
>
> This function returns the number of bytes transferred (if zero
> or positive), or else an error code.

ssize\_t parport\_read(struct parport \*port, void \*buffer, size\_t len)
:   read a block of data from a parallel port

**Parameters**

`struct parport *port`
:   port to read from

`void *buffer`
:   data buffer (in kernel space)

`size_t len`
:   number of bytes of data to transfer

**Description**

> This will read up to **len** bytes of **buffer** to the port
> specified, using the IEEE 1284 transfer mode most recently
> negotiated to (using [`parport_negotiate()`](#c.parport_negotiate "parport_negotiate")), as long as that
> mode supports reverse transfers (peripheral to host).
>
> It is the caller’s responsibility to ensure that the first
> **len** bytes of **buffer** are available to write to.
>
> This function returns the number of bytes transferred (if zero
> or positive), or else an error code.

long parport\_set\_timeout(struct pardevice \*dev, long inactivity)
:   set the inactivity timeout for a device

**Parameters**

`struct pardevice *dev`
:   device on a port

`long inactivity`
:   inactivity timeout (in jiffies)

**Description**

> This sets the inactivity timeout for a particular device on a
> port. This affects functions like [`parport_wait_peripheral()`](#c.parport_wait_peripheral "parport_wait_peripheral").
> The special value 0 means not to call `schedule()` while dealing
> with this device.
>
> The return value is the previous inactivity timeout.
>
> Any callers of [`parport_wait_event()`](#c.parport_wait_event "parport_wait_event") for this device are woken
> up.

int \_\_parport\_register\_driver(struct parport\_driver \*drv, struct module \*owner, const char \*mod\_name)
:   register a parallel port device driver

**Parameters**

`struct parport_driver *drv`
:   structure describing the driver

`struct module *owner`
:   owner module of drv

`const char *mod_name`
:   module name string

**Description**

> This can be called by a parallel port device driver in order
> to receive notifications about ports being found in the
> system, as well as ports no longer available.
>
> If devmodel is true then the new device model is used
> for registration.
>
> The **drv** structure is allocated by the caller and must not be
> deallocated until after calling [`parport_unregister_driver()`](#c.parport_unregister_driver "parport_unregister_driver").
>
> If using the non device model:
> The driver’s `attach()` function may block. The port that
> `attach()` is given will be valid for the duration of the
> callback, but if the driver wants to take a copy of the
> pointer it must call [`parport_get_port()`](#c.parport_get_port "parport_get_port") to do so. Calling
> `parport_register_device()` on that port will do this for you.
>
> The driver’s `detach()` function may block. The port that
> `detach()` is given will be valid for the duration of the
> callback, but if the driver wants to take a copy of the
> pointer it must call [`parport_get_port()`](#c.parport_get_port "parport_get_port") to do so.
>
> Returns 0 on success. The non device model will always succeeds.
> but the new device model can fail and will return the error code.

void parport\_unregister\_driver(struct parport\_driver \*drv)
:   deregister a parallel port device driver

**Parameters**

`struct parport_driver *drv`
:   structure describing the driver that was given to
    [`parport_register_driver()`](#c.parport_register_driver "parport_register_driver")

**Description**

> This should be called by a parallel port device driver that
> has registered itself using [`parport_register_driver()`](#c.parport_register_driver "parport_register_driver") when it
> is about to be unloaded.
>
> When it returns, the driver’s `attach()` routine will no longer
> be called, and for each port that `attach()` was called for, the
> `detach()` routine will have been called.
>
> All the driver’s `attach()` and `detach()` calls are guaranteed to have
> finished by the time this function returns.

struct parport \*parport\_get\_port(struct parport \*port)
:   increment a port’s reference count

**Parameters**

`struct parport *port`
:   the port

**Description**

> This ensures that a `struct parport` pointer remains valid
> until the matching [`parport_put_port()`](#c.parport_put_port "parport_put_port") call.

void parport\_put\_port(struct parport \*port)
:   decrement a port’s reference count

**Parameters**

`struct parport *port`
:   the port

**Description**

> This should be called once for each call to [`parport_get_port()`](#c.parport_get_port "parport_get_port"),
> once the port is no longer needed. When the reference count reaches
> zero (port is no longer used), free\_port is called.

struct parport \*parport\_register\_port(unsigned long base, int irq, int dma, struct parport\_operations \*ops)
:   register a parallel port

**Parameters**

`unsigned long base`
:   base I/O address

`int irq`
:   IRQ line

`int dma`
:   DMA channel

`struct parport_operations *ops`
:   pointer to the port driver’s port operations structure

**Description**

> When a parallel port (lowlevel) driver finds a port that
> should be made available to parallel port device drivers, it
> should call [`parport_register_port()`](#c.parport_register_port "parport_register_port"). The **base**, **irq**, and
> **dma** parameters are for the convenience of port drivers, and
> for ports where they aren’t meaningful needn’t be set to
> anything special. They can be altered afterwards by adjusting
> the relevant members of the parport structure that is returned
> and represents the port. They should not be tampered with
> after calling parport\_announce\_port, however.
>
> If there are parallel port device drivers in the system that
> have registered themselves using [`parport_register_driver()`](#c.parport_register_driver "parport_register_driver"),
> they are not told about the port at this time; that is done by
> [`parport_announce_port()`](#c.parport_announce_port "parport_announce_port").
>
> The **ops** structure is allocated by the caller, and must not be
> deallocated before calling [`parport_remove_port()`](#c.parport_remove_port "parport_remove_port").
>
> If there is no memory to allocate a new parport structure,
> this function will return `NULL`.

void parport\_announce\_port(struct parport \*port)
:   tell device drivers about a parallel port

**Parameters**

`struct parport *port`
:   parallel port to announce

**Description**

> After a port driver has registered a parallel port with
> parport\_register\_port, and performed any necessary
> initialisation or adjustments, it should call
> [`parport_announce_port()`](#c.parport_announce_port "parport_announce_port") in order to notify all device drivers
> that have called [`parport_register_driver()`](#c.parport_register_driver "parport_register_driver"). Their `attach()`
> functions will be called, with **port** as the parameter.

void parport\_remove\_port(struct parport \*port)
:   deregister a parallel port

**Parameters**

`struct parport *port`
:   parallel port to deregister

**Description**

> When a parallel port driver is forcibly unloaded, or a
> parallel port becomes inaccessible, the port driver must call
> this function in order to deal with device drivers that still
> want to use it.
>
> The parport structure associated with the port has its
> operations structure replaced with one containing ‘null’
> operations that return errors or just don’t do anything.
>
> Any drivers that have registered themselves using
> [`parport_register_driver()`](#c.parport_register_driver "parport_register_driver") are notified that the port is no
> longer accessible by having their `detach()` routines called
> with **port** as the parameter.

struct pardevice \*parport\_register\_dev\_model(struct parport \*port, const char \*name, const struct pardev\_cb \*par\_dev\_cb, int id)
:   register a device on a parallel port

**Parameters**

`struct parport *port`
:   port to which the device is attached

`const char *name`
:   a name to refer to the device

`const struct pardev_cb *par_dev_cb`
:   `struct containing` callbacks

`int id`
:   device number to be given to the device

**Description**

> This function, called by parallel port device drivers,
> declares that a device is connected to a port, and tells the
> system all it needs to know.
>
> The `struct pardev_cb` contains pointer to callbacks. preemption
> callback function, **preempt**, is called when this device driver
> has claimed access to the port but another device driver wants
> to use it. It is given, **private**, as its parameter, and should
> return zero if it is willing for the system to release the port
> to another driver on its behalf. If it wants to keep control of
> the port it should return non-zero, and no action will be taken.
> It is good manners for the driver to try to release the port at
> the earliest opportunity after its preemption callback rejects a
> preemption attempt. Note that if a preemption callback is happy
> for preemption to go ahead, there is no need to release the
> port; it is done automatically. This function may not block, as
> it may be called from interrupt context. If the device driver
> does not support preemption, **preempt** can be `NULL`.
>
> The wake-up (“kick”) callback function, **wakeup**, is called when
> the port is available to be claimed for exclusive access; that
> is, [`parport_claim()`](#c.parport_claim "parport_claim") is guaranteed to succeed when called from
> inside the wake-up callback function. If the driver wants to
> claim the port it should do so; otherwise, it need not take
> any action. This function may not block, as it may be called
> from interrupt context. If the device driver does not want to
> be explicitly invited to claim the port in this way, **wakeup** can
> be `NULL`.
>
> The interrupt handler, **irq\_func**, is called when an interrupt
> arrives from the parallel port. Note that if a device driver
> wants to use interrupts it should use `parport_enable_irq()`,
> and can also check the irq member of the parport structure
> representing the port.
>
> The parallel port (lowlevel) driver is the one that has called
> [`request_irq()`](../core-api/genericirq.html#c.request_irq "request_irq") and whose interrupt handler is called first.
> This handler does whatever needs to be done to the hardware to
> acknowledge the interrupt (for PC-style ports there is nothing
> special to be done). It then tells the IEEE 1284 code about
> the interrupt, which may involve reacting to an IEEE 1284
> event depending on the current IEEE 1284 phase. After this,
> it calls **irq\_func**. Needless to say, **irq\_func** will be called
> from interrupt context, and may not block.
>
> The `PARPORT_DEV_EXCL` flag is for preventing port sharing, and
> so should only be used when sharing the port with other device
> drivers is impossible and would lead to incorrect behaviour.
> Use it sparingly! Normally, **flags** will be zero.
>
> This function returns a pointer to a structure that represents
> the device on the port, or `NULL` if there is not enough memory
> to allocate space for that structure.

void parport\_unregister\_device(struct pardevice \*dev)
:   deregister a device on a parallel port

**Parameters**

`struct pardevice *dev`
:   pointer to structure representing device

**Description**

> This undoes the effect of `parport_register_device()`.

struct parport \*parport\_find\_number(int number)
:   find a parallel port by number

**Parameters**

`int number`
:   parallel port number

**Description**

> This returns the parallel port with the specified number, or
> `NULL` if there is none.
>
> There is an implicit [`parport_get_port()`](#c.parport_get_port "parport_get_port") done already; to throw
> away the reference to the port that [`parport_find_number()`](#c.parport_find_number "parport_find_number")
> gives you, use [`parport_put_port()`](#c.parport_put_port "parport_put_port").

struct parport \*parport\_find\_base(unsigned long base)
:   find a parallel port by base address

**Parameters**

`unsigned long base`
:   base I/O address

**Description**

> This returns the parallel port with the specified base
> address, or `NULL` if there is none.
>
> There is an implicit [`parport_get_port()`](#c.parport_get_port "parport_get_port") done already; to throw
> away the reference to the port that [`parport_find_base()`](#c.parport_find_base "parport_find_base")
> gives you, use [`parport_put_port()`](#c.parport_put_port "parport_put_port").

int parport\_claim(struct pardevice \*dev)
:   claim access to a parallel port device

**Parameters**

`struct pardevice *dev`
:   pointer to structure representing a device on the port

**Description**

> This function will not block and so can be used from interrupt
> context. If [`parport_claim()`](#c.parport_claim "parport_claim") succeeds in claiming access to
> the port it returns zero and the port is available to use. It
> may fail (returning non-zero) if the port is in use by another
> driver and that driver is not willing to relinquish control of
> the port.

int parport\_claim\_or\_block(struct pardevice \*dev)
:   claim access to a parallel port device

**Parameters**

`struct pardevice *dev`
:   pointer to structure representing a device on the port

**Description**

> This behaves like [`parport_claim()`](#c.parport_claim "parport_claim"), but will block if necessary
> to wait for the port to be free. A return value of 1
> indicates that it slept; 0 means that it succeeded without
> needing to sleep. A negative error code indicates failure.

void parport\_release(struct pardevice \*dev)
:   give up access to a parallel port device

**Parameters**

`struct pardevice *dev`
:   pointer to structure representing parallel port device

**Description**

> This function cannot fail, but it should not be called without
> the port claimed. Similarly, if the port is already claimed
> you should not try claiming it again.

struct pardevice \*parport\_open(int devnum, const char \*name)
:   find a device by canonical device number

**Parameters**

`int devnum`
:   canonical device number

`const char *name`
:   name to associate with the device

**Description**

> This function is similar to `parport_register_device()`, except
> that it locates a device by its number rather than by the port
> it is attached to.
>
> All parameters except for **devnum** are the same as for
> `parport_register_device()`. The return value is the same as
> for `parport_register_device()`.

void parport\_close(struct pardevice \*dev)
:   close a device opened with [`parport_open()`](#c.parport_open "parport_open")

**Parameters**

`struct pardevice *dev`
:   device to close

**Description**

> This is to [`parport_open()`](#c.parport_open "parport_open") as [`parport_unregister_device()`](#c.parport_unregister_device "parport_unregister_device") is to
> `parport_register_device()`.

# 16x50 UART Driver

struct uart\_8250\_port \*serial8250\_get\_port(int line)
:   retrieve `struct uart_8250_port`

**Parameters**

`int line`
:   serial line number

**Description**

This function retrieves `struct uart_8250_port` for the specific line.
This struct *must* *not* be used to perform a 8250 or serial core operation
which is not accessible otherwise. Its only purpose is to make the `struct
accessible` to the runtime-pm callbacks for context suspend/restore.
The lock assumption made here is none because runtime-pm suspend/resume
callbacks should not be invoked if there is any operation performed on the
port.

void serial8250\_suspend\_port(int line)
:   suspend one serial port

**Parameters**

`int line`
:   serial line number

**Description**

> Suspend one serial port.

void serial8250\_resume\_port(int line)
:   resume one serial port

**Parameters**

`int line`
:   serial line number

**Description**

> Resume one serial port.

int serial8250\_register\_8250\_port(const struct uart\_8250\_port \*up)
:   register a serial port

**Parameters**

`const struct uart_8250_port *up`
:   serial port template

**Description**

> Configure the serial port specified by the request. If the
> port exists and is in use, it is hung up and unregistered
> first.
>
> The port is then probed and if necessary the IRQ is autodetected
> If this fails an error is returned.
>
> On success the port is ready to use and the line number is returned.

void serial8250\_unregister\_port(int line)
:   remove a 16x50 serial port at runtime

**Parameters**

`int line`
:   serial line number

**Description**

> Remove one serial port. This may not be called from interrupt
> context. We hand the port back to the our control.

See [Low Level Serial API](serial/driver.html) for related APIs.

# Pulse-Width Modulation (PWM)

Pulse-width modulation is a modulation technique primarily used to
control power supplied to electrical devices.

The PWM framework provides an abstraction for providers and consumers of
PWM signals. A controller that provides one or more PWM signals is
registered as [`struct pwm_chip`](#c.pwm_chip "pwm_chip"). Providers
are expected to embed this structure in a driver-specific structure.
This structure contains fields that describe a particular chip.

A chip exposes one or more PWM signal sources, each of which exposed as
a [`struct pwm_device`](#c.pwm_device "pwm_device"). Operations can be
performed on PWM devices to control the period, duty cycle, polarity and
active state of the signal.

Note that PWM devices are exclusive resources: they can always only be
used by one consumer at a time.

enum pwm\_polarity
:   polarity of a PWM signal

**Constants**

`PWM_POLARITY_NORMAL`
:   a high signal for the duration of the duty-
    cycle, followed by a low signal for the remainder of the pulse
    period

`PWM_POLARITY_INVERSED`
:   a low signal for the duration of the duty-
    cycle, followed by a high signal for the remainder of the pulse
    period

struct pwm\_args
:   board-dependent PWM arguments

> **Definition**:
>
> ```
> struct pwm_args {
>       u64 period;
>       enum pwm_polarity polarity;
> };
> ```

**Members**

`period`
:   reference period

`polarity`
:   reference polarity

**Description**

This structure describes board-dependent arguments attached to a PWM
device. These arguments are usually retrieved from the PWM lookup table or
device tree.

Do not confuse this with the PWM state: PWM arguments represent the initial
configuration that users want to use on this PWM device rather than the
current PWM hardware state.

struct pwm\_waveform
:   description of a PWM waveform

> **Definition**:
>
> ```
> struct pwm_waveform {
>       u64 period_length_ns;
>       u64 duty_length_ns;
>       u64 duty_offset_ns;
> };
> ```

**Members**

`period_length_ns`
:   PWM period

`duty_length_ns`
:   PWM duty cycle

`duty_offset_ns`
:   offset of the rising edge from the period’s start

**Description**

This is a representation of a PWM waveform alternative to `struct pwm_state`
below. It’s more expressive than `struct pwm_state` as it contains a
duty\_offset\_ns and so can represent offsets other than zero (with .polarity =
PWM\_POLARITY\_NORMAL) and period - duty\_cycle (.polarity =
PWM\_POLARITY\_INVERSED).

Note there is no explicit bool for enabled. A “disabled” PWM is represented
by .period\_length\_ns = 0. Note further that the behaviour of a “disabled” PWM
is undefined. Depending on the hardware’s capabilities it might drive the
active or inactive level, go high-z or even continue to toggle.

The unit for all three members is nanoseconds.

struct pwm\_device
:   PWM channel object

> **Definition**:
>
> ```
> struct pwm_device {
>       const char *label;
>       unsigned long flags;
>       unsigned int hwpwm;
>       struct pwm_chip *chip;
>       struct pwm_args args;
>       struct pwm_state state;
>       struct pwm_state last;
> };
> ```

**Members**

`label`
:   name of the PWM device

`flags`
:   flags associated with the PWM device

`hwpwm`
:   per-chip relative index of the PWM device

`chip`
:   PWM chip providing this PWM device

`args`
:   PWM arguments

`state`
:   last applied state

`last`
:   last implemented state (for PWM\_DEBUG)

void pwm\_get\_state(const struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, struct pwm\_state \*state)
:   retrieve the current PWM state

**Parameters**

`const struct pwm_device *pwm`
:   PWM device

`struct pwm_state *state`
:   state to fill with the current PWM state

**Description**

The returned PWM state represents the state that was applied by a previous call to
[`pwm_apply_might_sleep()`](#c.pwm_apply_might_sleep "pwm_apply_might_sleep"). Drivers may have to slightly tweak that state before programming it to
hardware. If [`pwm_apply_might_sleep()`](#c.pwm_apply_might_sleep "pwm_apply_might_sleep") was never called, this returns either the current hardware
state (if supported) or the default settings.

void pwm\_init\_state(const struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, struct pwm\_state \*state)
:   prepare a new state to be applied with [`pwm_apply_might_sleep()`](#c.pwm_apply_might_sleep "pwm_apply_might_sleep")

**Parameters**

`const struct pwm_device *pwm`
:   PWM device

`struct pwm_state *state`
:   state to fill with the prepared PWM state

**Description**

This functions prepares a state that can later be tweaked and applied
to the PWM device with [`pwm_apply_might_sleep()`](#c.pwm_apply_might_sleep "pwm_apply_might_sleep"). This is a convenient function
that first retrieves the current PWM state and the replaces the period
and polarity fields with the reference values defined in pwm->args.
Once the function returns, you can adjust the ->enabled and ->duty\_cycle
fields according to your needs before calling [`pwm_apply_might_sleep()`](#c.pwm_apply_might_sleep "pwm_apply_might_sleep").

->duty\_cycle is initially set to zero to avoid cases where the current
->duty\_cycle value exceed the pwm\_args->period one, which would trigger
an error if the user calls [`pwm_apply_might_sleep()`](#c.pwm_apply_might_sleep "pwm_apply_might_sleep") without adjusting ->duty\_cycle
first.

unsigned int pwm\_get\_relative\_duty\_cycle(const struct pwm\_state \*state, unsigned int scale)
:   Get a relative duty cycle value

**Parameters**

`const struct pwm_state *state`
:   PWM state to extract the duty cycle from

`unsigned int scale`
:   target scale of the relative duty cycle

**Description**

This functions converts the absolute duty cycle stored in **state** (expressed
in nanosecond) into a value relative to the period.

For example if you want to get the duty\_cycle expressed in percent, call:

pwm\_get\_state(pwm, `state`);
duty = pwm\_get\_relative\_duty\_cycle(`state`, 100);

**Return**

rounded relative duty cycle multiplied by **scale**

int pwm\_set\_relative\_duty\_cycle(struct pwm\_state \*state, unsigned int duty\_cycle, unsigned int scale)
:   Set a relative duty cycle value

**Parameters**

`struct pwm_state *state`
:   PWM state to fill

`unsigned int duty_cycle`
:   relative duty cycle value

`unsigned int scale`
:   scale in which **duty\_cycle** is expressed

**Description**

This functions converts a relative into an absolute duty cycle (expressed
in nanoseconds), and puts the result in state->duty\_cycle.

For example if you want to configure a 50% duty cycle, call:

pwm\_init\_state(pwm, `state`);
pwm\_set\_relative\_duty\_cycle(`state`, 50, 100);
pwm\_apply\_might\_sleep(pwm, `state`);

**Return**

0 on success or `-EINVAL` if **duty\_cycle** and/or **scale** are
inconsistent (**scale** == 0 or **duty\_cycle** > **scale**)

struct pwm\_capture
:   PWM capture data

> **Definition**:
>
> ```
> struct pwm_capture {
>       unsigned int period;
>       unsigned int duty_cycle;
> };
> ```

**Members**

`period`
:   period of the PWM signal (in nanoseconds)

`duty_cycle`
:   duty cycle of the PWM signal (in nanoseconds)

struct pwm\_ops
:   PWM controller operations

> **Definition**:
>
> ```
> struct pwm_ops {
>       int (*request)(struct pwm_chip *chip, struct pwm_device *pwm);
>       void (*free)(struct pwm_chip *chip, struct pwm_device *pwm);
>       int (*capture)(struct pwm_chip *chip, struct pwm_device *pwm, struct pwm_capture *result, unsigned long timeout);
>       size_t sizeof_wfhw;
>       int (*round_waveform_tohw)(struct pwm_chip *chip, struct pwm_device *pwm, const struct pwm_waveform *wf, void *wfhw);
>       int (*round_waveform_fromhw)(struct pwm_chip *chip, struct pwm_device *pwm, const void *wfhw, struct pwm_waveform *wf);
>       int (*read_waveform)(struct pwm_chip *chip, struct pwm_device *pwm, void *wfhw);
>       int (*write_waveform)(struct pwm_chip *chip, struct pwm_device *pwm, const void *wfhw);
>       int (*apply)(struct pwm_chip *chip, struct pwm_device *pwm, const struct pwm_state *state);
>       int (*get_state)(struct pwm_chip *chip, struct pwm_device *pwm, struct pwm_state *state);
> };
> ```

**Members**

`request`
:   optional hook for requesting a PWM

`free`
:   optional hook for freeing a PWM

`capture`
:   capture and report PWM signal

`sizeof_wfhw`
:   size (in bytes) of driver specific waveform presentation

`round_waveform_tohw`
:   convert a [`struct pwm_waveform`](#c.pwm_waveform "pwm_waveform") to driver specific presentation

`round_waveform_fromhw`
:   convert a driver specific waveform presentation to [`struct pwm_waveform`](#c.pwm_waveform "pwm_waveform")

`read_waveform`
:   read driver specific waveform presentation from hardware

`write_waveform`
:   write driver specific waveform presentation to hardware

`apply`
:   atomically apply a new PWM config

`get_state`
:   get the current PWM state.

struct pwm\_chip
:   abstract a PWM controller

> **Definition**:
>
> ```
> struct pwm_chip {
>       struct device dev;
>       struct cdev cdev;
>       const struct pwm_ops *ops;
>       struct module *owner;
>       unsigned int id;
>       unsigned int npwm;
>       struct pwm_device * (*of_xlate)(struct pwm_chip *chip, const struct of_phandle_args *args);
>       bool atomic;
>       struct gpio_chip gpio;
>       bool uses_pwmchip_alloc;
>       bool operational;
>       union {
>             struct mutex nonatomic_lock;
>             spinlock_t atomic_lock;
>       };
>       struct pwm_device pwms[];
> };
> ```

**Members**

`dev`
:   device providing the PWMs

`cdev`
:   `struct cdev` for this device

`ops`
:   callbacks for this PWM controller

`owner`
:   module providing this chip

`id`
:   unique number of this PWM chip

`npwm`
:   number of PWMs controlled by this chip

`of_xlate`
:   request a PWM device given a device tree PWM specifier

`atomic`
:   can the driver’s ->`apply()` be called in atomic context

`gpio`
:   [`struct gpio_chip`](gpio/index.html#c.gpio_chip "gpio_chip") to operate this PWM chip’s lines as GPO

`uses_pwmchip_alloc`
:   signals if pwmchip\_allow was used to allocate this chip

`operational`
:   signals if the chip can be used (or is already deregistered)

`{unnamed_union}`
:   anonymous

`nonatomic_lock`
:   mutex for nonatomic chips

`atomic_lock`
:   mutex for atomic chips

`pwms`
:   array of PWM devices allocated by the framework

bool pwmchip\_supports\_waveform(struct [pwm\_chip](#c.pwm_chip "pwm_chip") \*chip)
:   checks if the given chip supports waveform callbacks

**Parameters**

`struct pwm_chip *chip`
:   The pwm\_chip to test

**Return**

true iff the pwm chip support the waveform functions like
[`pwm_set_waveform_might_sleep()`](#c.pwm_set_waveform_might_sleep "pwm_set_waveform_might_sleep") and [`pwm_round_waveform_might_sleep()`](#c.pwm_round_waveform_might_sleep "pwm_round_waveform_might_sleep")

int pwm\_config(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, int duty\_ns, int period\_ns)
:   change a PWM device configuration

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`int duty_ns`
:   “on” time (in nanoseconds)

`int period_ns`
:   duration (in nanoseconds) of one cycle

**Return**

0 on success or a negative error code on failure.

int pwm\_enable(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm)
:   start a PWM output toggling

**Parameters**

`struct pwm_device *pwm`
:   PWM device

**Return**

0 on success or a negative error code on failure.

void pwm\_disable(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm)
:   stop a PWM output toggling

**Parameters**

`struct pwm_device *pwm`
:   PWM device

bool pwm\_might\_sleep(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm)
:   is [`pwm_apply_atomic()`](#c.pwm_apply_atomic "pwm_apply_atomic") supported?

**Parameters**

`struct pwm_device *pwm`
:   PWM device

**Return**

false if [`pwm_apply_atomic()`](#c.pwm_apply_atomic "pwm_apply_atomic") can be called from atomic context.

int pwm\_round\_waveform\_might\_sleep(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, struct [pwm\_waveform](#c.pwm_waveform "pwm_waveform") \*wf)
:   Query hardware capabilities Cannot be used in atomic context.

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`struct pwm_waveform *wf`
:   waveform to round and output parameter

**Description**

Typically a given waveform cannot be implemented exactly by hardware, e.g.
because hardware only supports coarse period resolution or no duty\_offset.
This function returns the actually implemented waveform if you pass **wf** to
[`pwm_set_waveform_might_sleep()`](#c.pwm_set_waveform_might_sleep "pwm_set_waveform_might_sleep") now.

Note however that the world doesn’t stop turning when you call it, so when
doing:

```
pwm_round_waveform_might_sleep(mypwm, &wf);
pwm_set_waveform_might_sleep(mypwm, &wf, true);
```

the latter might fail, e.g. because an input clock changed its rate between
these two calls and the waveform determined by
[`pwm_round_waveform_might_sleep()`](#c.pwm_round_waveform_might_sleep "pwm_round_waveform_might_sleep") cannot be implemented any more.

Usually all values passed in **wf** are rounded down to the nearest possible
value (in the order period\_length\_ns, duty\_length\_ns and then
duty\_offset\_ns). Only if this isn’t possible, a value might grow. See the
documentation for [`pwm_set_waveform_might_sleep()`](#c.pwm_set_waveform_might_sleep "pwm_set_waveform_might_sleep") for a more formal
description.

**Return**

0 on success, 1 if at least one value had to be rounded up or a
negative errno.

**Context**

May sleep.

int pwm\_get\_waveform\_might\_sleep(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, struct [pwm\_waveform](#c.pwm_waveform "pwm_waveform") \*wf)
:   Query hardware about current configuration Cannot be used in atomic context.

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`struct pwm_waveform *wf`
:   output parameter

**Description**

Stores the current configuration of the PWM in **wf**. Note this is the
equivalent of [`pwm_get_state_hw()`](#c.pwm_get_state_hw "pwm_get_state_hw") (and not [`pwm_get_state()`](#c.pwm_get_state "pwm_get_state")) for pwm\_waveform.

**Return**

0 on success or a negative errno

**Context**

May sleep.

int pwm\_set\_waveform\_might\_sleep(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, const struct [pwm\_waveform](#c.pwm_waveform "pwm_waveform") \*wf, bool exact)
:   Apply a new waveform Cannot be used in atomic context.

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`const struct pwm_waveform *wf`
:   The waveform to apply

`bool exact`
:   If true no rounding is allowed

**Description**

Typically a requested waveform cannot be implemented exactly, e.g. because
you requested .period\_length\_ns = 100 ns, but the hardware can only set
periods that are a multiple of 8.5 ns. With that hardware passing **exact** =
true results in [`pwm_set_waveform_might_sleep()`](#c.pwm_set_waveform_might_sleep "pwm_set_waveform_might_sleep") failing and returning -EDOM.
If **exact** = false you get a period of 93.5 ns (i.e. the biggest period not
bigger than the requested value).
Note that even with **exact** = true, some rounding by less than 1 ns is
possible/needed. In the above example requesting .period\_length\_ns = 94 and
**exact** = true, you get the hardware configured with period = 93.5 ns.

Let C be the set of possible hardware configurations for a given PWM device,
consisting of tuples (p, d, o) where p is the period length, d is the duty
length and o the duty offset.

The following algorithm is implemented to pick the hardware setting
(p, d, o) ∈ C for a given request (p’, d’, o’) with **exact** = false:

```
p = max( { ṗ | ∃ ḋ, ȯ : (ṗ, ḋ, ȯ) ∈ C ∧ ṗ ≤ p' } ∪ { min({ ṗ | ∃ ḋ, ȯ : (ṗ, ḋ, ȯ) ∈ C }) })
d = max( { ḋ | ∃ ȯ : (p, ḋ, ȯ) ∈ C ∧ ḋ ≤ d' } ∪ { min({ ḋ | ∃ ȯ : (p, ḋ, ȯ) ∈ C }) })
o = max( { ȯ | (p, d, ȯ) ∈ C ∧ ȯ ≤ o' } ∪ { min({ ȯ | (p, d, ȯ) ∈ C }) })
```

In words: The chosen period length is the maximal possible period length not
bigger than the requested period length and if that doesn’t exist, the
minimal period length. The chosen duty length is the maximal possible duty
length that is compatible with the chosen period length and isn’t bigger than
the requested duty length. Again if such a value doesn’t exist, the minimal
duty length compatible with the chosen period is picked. After that the duty
offset compatible with the chosen period and duty length is chosen in the
same way.

**Return**

0 on success, -EDOM if setting failed due to the exact waveform not
being possible (if **exact**), or a different negative errno on failure.

**Context**

May sleep.

int pwm\_apply\_might\_sleep(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, const struct pwm\_state \*state)
:   atomically apply a new state to a PWM device Cannot be used in atomic context.

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`const struct pwm_state *state`
:   new state to apply

**Return**

0 on success, or a negative errno

**Context**

May sleep.

int pwm\_apply\_atomic(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, const struct pwm\_state \*state)
:   apply a new state to a PWM device from atomic context Not all PWM devices support this function, check with [`pwm_might_sleep()`](#c.pwm_might_sleep "pwm_might_sleep").

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`const struct pwm_state *state`
:   new state to apply

**Return**

0 on success, or a negative errno

**Context**

Any

int pwm\_get\_state\_hw(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm, struct pwm\_state \*state)
:   get the current PWM state from hardware

**Parameters**

`struct pwm_device *pwm`
:   PWM device

`struct pwm_state *state`
:   state to fill with the current PWM state

**Description**

Similar to [`pwm_get_state()`](#c.pwm_get_state "pwm_get_state") but reads the current PWM state from hardware
instead of the requested state.

**Return**

0 on success or a negative error code on failure.

**Context**

May sleep.

int pwm\_adjust\_config(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm)
:   adjust the current PWM config to the PWM arguments

**Parameters**

`struct pwm_device *pwm`
:   PWM device

**Description**

This function will adjust the PWM config to the PWM arguments provided
by the DT or PWM lookup table. This is particularly useful to adapt
the bootloader config to the Linux one.

**Return**

0 on success or a negative error code on failure.

**Context**

May sleep.

struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm\_get(struct [device](infrastructure.html#c.device "device") \*dev, const char \*con\_id)
:   look up and request a PWM device

**Parameters**

`struct device *dev`
:   device for PWM consumer

`const char *con_id`
:   consumer name

**Description**

Lookup is first attempted using DT. If the device was not instantiated from
a device tree, a PWM chip and a relative index is looked up via a table
supplied by board setup code (see `pwm_add_table()`).

Once a PWM chip has been found the specified PWM device will be requested
and is ready to be used.

**Return**

A pointer to the requested PWM device or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded
error code on failure.

void pwm\_put(struct [pwm\_device](#c.pwm_device "pwm_device") \*pwm)
:   release a PWM device

**Parameters**

`struct pwm_device *pwm`
:   PWM device

struct [pwm\_device](#c.pwm_device "pwm_device") \*devm\_pwm\_get(struct [device](infrastructure.html#c.device "device") \*dev, const char \*con\_id)
:   resource managed [`pwm_get()`](#c.pwm_get "pwm_get")

**Parameters**

`struct device *dev`
:   device for PWM consumer

`const char *con_id`
:   consumer name

**Description**

This function performs like [`pwm_get()`](#c.pwm_get "pwm_get") but the acquired PWM device will
automatically be released on driver detach.

**Return**

A pointer to the requested PWM device or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded
error code on failure.

struct [pwm\_device](#c.pwm_device "pwm_device") \*devm\_fwnode\_pwm\_get(struct [device](infrastructure.html#c.device "device") \*dev, struct fwnode\_handle \*fwnode, const char \*con\_id)
:   request a resource managed PWM from firmware node

**Parameters**

`struct device *dev`
:   device for PWM consumer

`struct fwnode_handle *fwnode`
:   firmware node to get the PWM from

`const char *con_id`
:   consumer name

**Description**

Returns the PWM device parsed from the firmware node. See `of_pwm_get()` and
`acpi_pwm_get()` for a detailed description.

**Return**

A pointer to the requested PWM device or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded
error code on failure.

int \_\_pwmchip\_add(struct [pwm\_chip](#c.pwm_chip "pwm_chip") \*chip, struct module \*owner)
:   register a new PWM chip

**Parameters**

`struct pwm_chip *chip`
:   the PWM chip to add

`struct module *owner`
:   reference to the module providing the chip.

**Description**

Register a new PWM chip. **owner** is supposed to be THIS\_MODULE, use the
pwmchip\_add wrapper to do this right.

**Return**

0 on success or a negative error code on failure.

void pwmchip\_remove(struct [pwm\_chip](#c.pwm_chip "pwm_chip") \*chip)
:   remove a PWM chip

**Parameters**

`struct pwm_chip *chip`
:   the PWM chip to remove

**Description**

Removes a PWM chip.
