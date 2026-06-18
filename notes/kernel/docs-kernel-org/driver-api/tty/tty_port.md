# TTY Port

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_port.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY Port

The TTY drivers are advised to use [`struct tty_port`](#c.tty_port "tty_port") helpers as much as possible.
If the drivers implement `tty_port.ops.activate()` and
`tty_port.ops.shutdown()`, they can use [`tty_port_open()`](#c.tty_port_open "tty_port_open"),
[`tty_port_close()`](#c.tty_port_close "tty_port_close"), and [`tty_port_hangup()`](#c.tty_port_hangup "tty_port_hangup") in respective
`tty_struct.ops` hooks.

The reference and details are contained in the [TTY Port Reference](#tty-port-reference) and [TTY
Port Operations Reference](#tty-port-operations-reference) sections at the bottom.

## [TTY Port Functions](#id1)

### [Init & Destroy](#id2)

void tty\_port\_init(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   initialize tty\_port

**Parameters**

`struct tty_port *port`
:   tty\_port to initialize

**Description**

Initializes the state of [`struct tty_port`](#c.tty_port "tty_port"). When a port was initialized using
this function, one has to destroy the port by [`tty_port_destroy()`](#c.tty_port_destroy "tty_port_destroy"). Either
indirectly by using [`tty_port`](#c.tty_port "tty_port") refcounting ([`tty_port_put()`](#c.tty_port_put "tty_port_put")) or directly if
refcounting is not used.

void tty\_port\_destroy(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   destroy inited port

**Parameters**

`struct tty_port *port`
:   tty port to be destroyed

**Description**

When a port was initialized using [`tty_port_init()`](#c.tty_port_init "tty_port_init"), one has to destroy the
port by this function. Either indirectly by using [`tty_port`](#c.tty_port "tty_port") refcounting
([`tty_port_put()`](#c.tty_port_put "tty_port_put")) or directly if refcounting is not used.

void tty\_port\_put(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   drop a reference to tty\_port

**Parameters**

`struct tty_port *port`
:   port to drop a reference of (can be NULL)

**Description**

The final put will destroy and free up the **port** using
**port->ops->`destruct()`** hook, or using [`kfree()`](../../core-api/mm-api.html#c.kfree "kfree") if not provided.

### [Open/Close/Hangup Helpers](#id3)

void tty\_port\_shutdown(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   internal helper to shutdown the device

**Parameters**

`struct tty_port *port`
:   tty port to be shut down

`struct tty_struct *tty`
:   the associated tty

**Description**

It is used by [`tty_port_hangup()`](#c.tty_port_hangup "tty_port_hangup") and [`tty_port_close()`](#c.tty_port_close "tty_port_close"). Its task is to
shutdown the device if it was initialized (note consoles remain
functioning). It lowers DTR/RTS (if **tty** has HUPCL set) and invokes
**port->ops->`shutdown()`**.

void tty\_port\_hangup(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   hangup helper

**Parameters**

`struct tty_port *port`
:   tty port

**Description**

Perform port level tty hangup flag and count changes. Drop the tty
reference.

Caller holds tty lock.

int tty\_port\_block\_til\_ready(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Waiting logic for tty open

**Parameters**

`struct tty_port *port`
:   the tty port being opened

`struct tty_struct *tty`
:   the tty device being bound

`struct file *filp`
:   the file pointer of the opener or `NULL`

**Description**

Implement the core POSIX/SuS tty behaviour when opening a tty device.
Handles:

> * hangup (both before and during)
> * non blocking open
> * rts/dtr/dcd
> * signals
> * port flags and counts

The passed **port** must implement the **port->ops->carrier\_raised** method if it
can do carrier detect and the **port->ops->dtr\_rts** method if it supports
software management of these lines. Note that the dtr/rts raise is done each
iteration as a hangup may have previously dropped them while we wait.

Caller holds tty lock.

**Note**

May drop and reacquire tty lock when blocking, so **tty** and **port** may
have changed state (eg., may have been hung up).

int tty\_port\_close\_start(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   helper for tty->ops->close, part 1/2

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_struct *tty`
:   tty being closed

`struct file *filp`
:   passed file pointer

**Description**

Decrements and checks open count. Flushes the port if this is the last
close. That means, dropping the data from the outpu buffer on the device and
waiting for sending logic to finish. The rest of close handling is performed
in [`tty_port_close_end()`](#c.tty_port_close_end "tty_port_close_end").

Locking: Caller holds tty lock.

**Return**

1 if this is the last close, otherwise 0

void tty\_port\_close\_end(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   helper for tty->ops->close, part 2/2

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_struct *tty`
:   tty being closed

**Description**

This is a continuation of the first part: [`tty_port_close_start()`](#c.tty_port_close_start "tty_port_close_start"). This
should be called after turning off the device. It flushes the data from the
line discipline and delays the close by **port->close\_delay**.

Locking: Caller holds tty lock.

void tty\_port\_close(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   generic tty->ops->close handler

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_struct *tty`
:   tty being closed

`struct file *filp`
:   passed file pointer

**Description**

It is a generic helper to be used in driver’s **tty->ops->close**. It wraps a
sequence of [`tty_port_close_start()`](#c.tty_port_close_start "tty_port_close_start"), [`tty_port_shutdown()`](#c.tty_port_shutdown "tty_port_shutdown"), and
[`tty_port_close_end()`](#c.tty_port_close_end "tty_port_close_end"). The latter two are called only if this is the last
close. See the respective functions for the details.

Locking: Caller holds tty lock

int tty\_port\_install(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_driver](tty_driver.html#c.tty_driver "tty_driver") \*driver, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   generic tty->ops->install handler

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_driver *driver`
:   tty\_driver for this device

`struct tty_struct *tty`
:   tty to be installed

**Description**

It is the same as [`tty_standard_install()`](tty_struct.html#c.tty_standard_install "tty_standard_install") except the provided **port** is linked
to a concrete tty specified by **tty**. Use this or [`tty_port_register_device()`](tty_driver.html#c.tty_port_register_device "tty_port_register_device")
(or both). Call [`tty_port_link_device()`](tty_driver.html#c.tty_port_link_device "tty_port_link_device") as a last resort.

int tty\_port\_open(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   generic tty->ops->open handler

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_struct *tty`
:   tty to be opened

`struct file *filp`
:   passed file pointer

**Description**

It is a generic helper to be used in driver’s **tty->ops->open**. It activates
the devices using **port->ops->activate** if not active already. And waits for
the device to be ready using [`tty_port_block_til_ready()`](#c.tty_port_block_til_ready "tty_port_block_til_ready") (e.g. raises
DTR/CTS and waits for carrier).

Note that **port->ops->shutdown** is not called when **port->ops->activate**
returns an error (on the contrary, **tty->ops->close** is).

Locking: Caller holds tty lock.

**Note**

may drop and reacquire tty lock (in [`tty_port_block_til_ready()`](#c.tty_port_block_til_ready "tty_port_block_til_ready")) so
**tty** and **port** may have changed state (eg., may be hung up now).

### [TTY Refcounting](#id4)

struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty\_port\_tty\_get(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   get a tty reference

**Parameters**

`struct tty_port *port`
:   tty port

**Description**

Return a refcount protected tty instance or `NULL` if the port is not
associated with a tty (eg due to close or hangup).

void tty\_port\_tty\_set(struct [tty\_port](#c.tty_port "tty_port") \*port, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   set the tty of a port

**Parameters**

`struct tty_port *port`
:   tty port

`struct tty_struct *tty`
:   the tty

**Description**

Associate the port and tty pair. Manages any internal refcounts. Pass `NULL`
to deassociate a port.

### [TTY Helpers](#id5)

void tty\_port\_tty\_hangup(struct [tty\_port](#c.tty_port "tty_port") \*port, bool check\_clocal)
:   helper to hang up a tty asynchronously

**Parameters**

`struct tty_port *port`
:   tty port

`bool check_clocal`
:   hang only ttys with `CLOCAL` unset?

void tty\_port\_tty\_vhangup(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   helper to hang up a tty synchronously

**Parameters**

`struct tty_port *port`
:   tty port

void tty\_port\_tty\_wakeup(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   helper to wake up a tty

**Parameters**

`struct tty_port *port`
:   tty port

### [Modem Signals](#id6)

bool tty\_port\_carrier\_raised(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   carrier raised check

**Parameters**

`struct tty_port *port`
:   tty port

**Description**

Wrapper for the carrier detect logic. For the moment this is used
to hide some internal details. This will eventually become entirely
internal to the tty port.

void tty\_port\_raise\_dtr\_rts(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   Raise DTR/RTS

**Parameters**

`struct tty_port *port`
:   tty port

**Description**

Wrapper for the DTR/RTS raise logic. For the moment this is used to hide
some internal details. This will eventually become entirely internal to the
tty port.

void tty\_port\_lower\_dtr\_rts(struct [tty\_port](#c.tty_port "tty_port") \*port)
:   Lower DTR/RTS

**Parameters**

`struct tty_port *port`
:   tty port

**Description**

Wrapper for the DTR/RTS raise logic. For the moment this is used to hide
some internal details. This will eventually become entirely internal to the
tty port.

---

## [TTY Port Reference](#id7)

struct tty\_port
:   * port level information

**Definition**:

```
struct tty_port {
    struct tty_bufhead      buf;
    struct tty_struct       *tty;
    struct tty_struct       *itty;
    const struct tty_port_operations *ops;
    const struct tty_port_client_operations *client_ops;
    spinlock_t lock;
    int blocked_open;
    int count;
    wait_queue_head_t open_wait;
    wait_queue_head_t delta_msr_wait;
    unsigned long           flags;
    unsigned long           iflags;
    unsigned char           console:1;
    struct mutex            mutex;
    struct mutex            buf_mutex;
    u8 *xmit_buf;
    u8 *xmit_fifo;
    unsigned int            close_delay;
    unsigned int            closing_wait;
    int drain_delay;
    struct kref             kref;
    void *client_data;
};
```

**Members**

`buf`
:   buffer for this port, locked internally

`tty`
:   back pointer to [`struct tty_struct`](tty_struct.html#c.tty_struct "tty_struct"), valid only if the tty is open. Use
    [`tty_port_tty_get()`](#c.tty_port_tty_get "tty_port_tty_get") to obtain it (and [`tty_kref_put()`](tty_struct.html#c.tty_kref_put "tty_kref_put") to release).

`itty`
:   internal back pointer to [`struct tty_struct`](tty_struct.html#c.tty_struct "tty_struct"). Avoid this. It should be
    eliminated in the long term.

`ops`
:   tty port operations (like activate, shutdown), see [`struct
    tty_port_operations`](#c.tty_port_operations "tty_port_operations")

`client_ops`
:   tty port client operations (like receive\_buf, write\_wakeup).
    By default, tty\_port\_default\_client\_ops is used.

`lock`
:   lock protecting **tty**

`blocked_open`
:   # of procs waiting for open in [`tty_port_block_til_ready()`](#c.tty_port_block_til_ready "tty_port_block_til_ready")

`count`
:   usage count

`open_wait`
:   open waiters queue (waiting e.g. for a carrier)

`delta_msr_wait`
:   modem status change queue (waiting for MSR changes)

`flags`
:   user TTY flags (`ASYNC_`)

`iflags`
:   internal flags (`TTY_PORT_`)

`console`
:   when set, the port is a console

`mutex`
:   locking, for open, shutdown and other port operations

`buf_mutex`
:   **xmit\_buf** alloc lock

`xmit_buf`
:   optional xmit buffer used by some drivers

`xmit_fifo`
:   optional xmit buffer used by some drivers

`close_delay`
:   delay in jiffies to wait when closing the port

`closing_wait`
:   delay in jiffies for output to be sent before closing

`drain_delay`
:   set to zero if no pure time based drain is needed else set to
    size of fifo

`kref`
:   references counter. Reaching zero calls **ops->`destruct()`** if non-`NULL`
    or frees the port otherwise.

`client_data`
:   pointer to private data, for **client\_ops**

**Description**

Each device keeps its own port level information. [`struct tty_port`](#c.tty_port "tty_port") was
introduced as a common structure for such information. As every TTY device
shall have a backing tty\_port structure, every driver can use these members.

The tty port has a different lifetime to the tty so must be kept apart.
In addition be careful as tty -> port mappings are valid for the life
of the tty object but in many cases port -> tty mappings are valid only
until a hangup so don’t use the wrong path.

Tty port shall be initialized by [`tty_port_init()`](#c.tty_port_init "tty_port_init") and shut down either by
[`tty_port_destroy()`](#c.tty_port_destroy "tty_port_destroy") (refcounting not used), or [`tty_port_put()`](#c.tty_port_put "tty_port_put") (refcounting).

There is a lot of helpers around [`struct tty_port`](#c.tty_port "tty_port") too. To name the most
significant ones: [`tty_port_open()`](#c.tty_port_open "tty_port_open"), [`tty_port_close()`](#c.tty_port_close "tty_port_close") (or
[`tty_port_close_start()`](#c.tty_port_close_start "tty_port_close_start") and [`tty_port_close_end()`](#c.tty_port_close_end "tty_port_close_end") separately if need be), and
[`tty_port_hangup()`](#c.tty_port_hangup "tty_port_hangup"). These call **ops->`activate()`** and **ops->`shutdown()`** as
needed.

---

## [TTY Port Operations Reference](#id8)

struct tty\_port\_operations
:   * operations on tty\_port

**Definition**:

```
struct tty_port_operations {
    bool (*carrier_raised)(struct tty_port *port);
    void (*dtr_rts)(struct tty_port *port, bool active);
    void (*shutdown)(struct tty_port *port);
    int (*activate)(struct tty_port *port, struct tty_struct *tty);
    void (*destruct)(struct tty_port *port);
};
```

**Members**

`carrier_raised`
:   return true if the carrier is raised on **port**

`dtr_rts`
:   raise the DTR line if **active** is true, otherwise lower DTR

`shutdown`
:   called when the last close completes or a hangup finishes IFF the
    port was initialized. Do not use to free resources. Turn off the device
    only. Called under the port mutex to serialize against **activate** and
    **shutdown**.

`activate`
:   called under the port mutex from [`tty_port_open()`](#c.tty_port_open "tty_port_open"), serialized using
    the port mutex. Supposed to turn on the device.

`destruct`
:   called on the final put of a port. Free resources, possibly incl.
    the port itself.

**Description**

> FIXME: long term getting the tty argument *out* of this would be good
> for consoles.
