# Console

> 출처(원문): https://docs.kernel.org/driver-api/tty/console.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Console

## [Struct Console](#id2)

enum cons\_flags
:   General console flags

**Constants**

`CON_PRINTBUFFER`
:   Used by newly registered consoles to avoid duplicate
    output of messages that were already shown by boot
    consoles or read by userspace via `syslog()` syscall.

`CON_CONSDEV`
:   Indicates that the console driver is backing
    /dev/console.

`CON_ENABLED`
:   Indicates if a console is allowed to print records. If
    false, the console also will not advance to later
    records.

`CON_BOOT`
:   Marks the console driver as early console driver which
    is used during boot before the real driver becomes
    available. It will be automatically unregistered
    when the real console driver is registered unless
    “keep\_bootcon” parameter is used.

`CON_ANYTIME`
:   A misnomed historical flag which tells the core code
    that the legacy **console**::write callback can be invoked
    on a CPU which is marked OFFLINE. That is misleading as
    it suggests that there is no contextual limit for
    invoking the callback. The original motivation was
    readiness of the per-CPU areas.

`CON_BRL`
:   Indicates a braille device which is exempt from
    receiving the printk spam for obvious reasons.

`CON_EXTENDED`
:   The console supports the extended output format of
    /dev/kmesg which requires a larger output buffer.

`CON_SUSPENDED`
:   Indicates if a console is suspended. If true, the
    printing callbacks must not be called.

`CON_NBCON`
:   Console can operate outside of the legacy style console\_lock
    constraints.

`CON_NBCON_ATOMIC_UNSAFE`
:   The `write_atomic()` callback is not safe and is
    therefore only used by `nbcon_atomic_flush_unsafe()`.

struct console
:   The console descriptor structure

**Definition**:

```
struct console {
    char name[16];
    void (*write)(struct console *co, const char *s, unsigned int count);
    int (*read)(struct console *co, char *s, unsigned int count);
    struct tty_driver       *(*device)(struct console *co, int *index);
    void (*unblank)(void);
    int (*setup)(struct console *co, char *options);
    int (*exit)(struct console *co);
    int (*match)(struct console *co, char *name, int idx, char *options);
    short flags;
    short index;
    int cflag;
    uint ispeed;
    uint ospeed;
    u64 seq;
    unsigned long           dropped;
    void *data;
    struct hlist_node       node;
    void (*write_atomic)(struct console *con, struct nbcon_write_context *wctxt);
    void (*write_thread)(struct console *con, struct nbcon_write_context *wctxt);
    void (*device_lock)(struct console *con, unsigned long *flags);
    void (*device_unlock)(struct console *con, unsigned long flags);
    atomic_t nbcon_state;
    atomic_long_t nbcon_seq;
    struct nbcon_context    nbcon_device_ctxt;
    atomic_long_t nbcon_prev_seq;
    struct printk_buffers   *pbufs;
    struct task_struct      *kthread;
    struct rcuwait          rcuwait;
    struct irq_work         irq_work;
};
```

**Members**

`name`
:   The name of the console driver

`write`
:   Legacy write callback to output messages (Optional)

`read`
:   Read callback for console input (Optional)

`device`
:   The underlying TTY device driver (Optional)

`unblank`
:   Callback to unblank the console (Optional)

`setup`
:   Callback for initializing the console (Optional)

`exit`
:   Callback for teardown of the console (Optional)

`match`
:   Callback for matching a console (Optional)

`flags`
:   Console flags. See [`enum cons_flags`](#c.cons_flags "cons_flags")

`index`
:   Console index, e.g. port number

`cflag`
:   TTY control mode flags

`ispeed`
:   TTY input speed

`ospeed`
:   TTY output speed

`seq`
:   Sequence number of the next ringbuffer record to print

`dropped`
:   Number of unreported dropped ringbuffer records

`data`
:   Driver private data

`node`
:   hlist node for the console list

`write_atomic`
:   NBCON callback to write out text in any context. (Optional)

    This callback is called with the console already acquired. However,
    a higher priority context is allowed to take it over by default.

    The callback must call `nbcon_enter_unsafe()` and `nbcon_exit_unsafe()`
    around any code where the takeover is not safe, for example, when
    manipulating the serial port registers.

    `nbcon_enter_unsafe()` will fail if the context has lost the console
    ownership in the meantime. In this case, the callback is no longer
    allowed to go forward. It must back out immediately and carefully.
    The buffer content is also no longer trusted since it no longer
    belongs to the context.

    The callback should allow the takeover whenever it is safe. It
    increases the chance to see messages when the system is in trouble.
    If the driver must reacquire ownership in order to finalize or
    revert hardware changes, `nbcon_reacquire_nobuf()` can be used.
    However, on reacquire the buffer content is no longer available. A
    reacquire cannot be used to resume printing.

    The callback can be called from any context (including NMI).
    Therefore it must avoid usage of any locking and instead rely
    on the console ownership for synchronization.

`write_thread`
:   NBCON callback to write out text in task context.

    This callback must be called only in task context with both
    `device_lock()` and the nbcon console acquired with
    NBCON\_PRIO\_NORMAL.

    The same rules for console ownership verification and unsafe
    sections handling applies as with `write_atomic()`.

    The console ownership handling is necessary for synchronization
    against `write_atomic()` which is synchronized only via the context.

    The `device_lock()` provides the primary serialization for operations
    on the device. It might be as relaxed (mutex)[\*] or as tight
    (disabled preemption and interrupts) as needed. It allows
    the kthread to operate in the least restrictive mode[\*\*].

    [\*] Standalone nbcon\_context\_try\_acquire() is not safe with
    :   the preemption enabled, see `nbcon_owner_matches()`. But it
        can be safe when always called in the preemptive context
        under the `device_lock()`.

    [\*\*] The device\_lock() makes sure that nbcon\_context\_try\_acquire()
    :   would never need to spin which is important especially with
        PREEMPT\_RT.

`device_lock`
:   NBCON callback to begin synchronization with driver code.

    Console drivers typically must deal with access to the hardware
    via user input/output (such as an interactive login shell) and
    output of kernel messages via [`printk()`](../../core-api/printk-basics.html#c.printk "printk") calls. This callback is
    called by the printk-subsystem whenever it needs to synchronize
    with hardware access by the driver. It should be implemented to
    use whatever synchronization mechanism the driver is using for
    itself (for example, the port lock for uart serial consoles).

    The callback is always called from task context. It may use any
    synchronization method required by the driver.

    IMPORTANT: The callback MUST disable migration. The console driver
    :   may be using a synchronization mechanism that already takes
        care of this (such as spinlocks). Otherwise this function must
        explicitly call `migrate_disable()`.

    The flags argument is provided as a convenience to the driver. It
    will be passed again to `device_unlock()`. It can be ignored if the
    driver does not need it.

`device_unlock`
:   NBCON callback to finish synchronization with driver code.

    It is the counterpart to `device_lock()`.

    This callback is always called from task context. It must
    appropriately re-enable migration (depending on how `device_lock()`
    disabled migration).

    The flags argument is the value of the same variable that was
    passed to `device_lock()`.

`nbcon_state`
:   State for nbcon consoles

`nbcon_seq`
:   Sequence number of the next record for nbcon to print

`nbcon_device_ctxt`
:   Context available for non-printing operations

`nbcon_prev_seq`
:   Seq num the previous nbcon owner was assigned to print

`pbufs`
:   Pointer to nbcon private buffer

`kthread`
:   Printer kthread for this console

`rcuwait`
:   RCU-safe wait object for **kthread** waking

`irq_work`
:   Defer **kthread** waking to IRQ work context

### [Internals](#id3)

struct nbcon\_state
:   console state for nbcon consoles

**Definition**:

```
struct nbcon_state {
    union {
        unsigned int    atom;
        struct {
            unsigned int prio               :  2;
            unsigned int req_prio           :  2;
            unsigned int unsafe             :  1;
            unsigned int unsafe_takeover    :  1;
            unsigned int cpu                : 24;
        };
    };
};
```

**Members**

`{unnamed_union}`
:   anonymous

`atom`
:   Compound of the state fields for atomic operations

`{unnamed_struct}`
:   anonymous

`prio`
:   The priority of the current owner

`req_prio`
:   The priority of a handover request

`unsafe`
:   Console is busy in a non takeover region

`unsafe_takeover`
:   A hostile takeover in an unsafe state happened in the
    past. The console cannot be safe until re-initialized.

`cpu`
:   The CPU on which the owner runs

**Description**

To be used for reading and preparing of the value stored in the nbcon
state variable **console**::nbcon\_state.

The **prio** and **req\_prio** fields are particularly important to allow
spin-waiting to timeout and give up without the risk of a waiter being
assigned the lock after giving up.

enum nbcon\_prio
:   console owner priority for nbcon consoles

**Constants**

`NBCON_PRIO_NONE`
:   Unused

`NBCON_PRIO_NORMAL`
:   Normal (non-emergency) usage

`NBCON_PRIO_EMERGENCY`
:   Emergency output (WARN/OOPS...)

`NBCON_PRIO_PANIC`
:   Panic output

`NBCON_PRIO_MAX`
:   The number of priority levels

**Description**

A higher priority context can takeover the console when it is
in the safe state. The final attempt to flush consoles in `panic()`
can be allowed to do so even in an unsafe state (Hope and pray).

struct nbcon\_context
:   Context for console acquire/release

**Definition**:

```
struct nbcon_context {
    struct console          *console;
    unsigned int            spinwait_max_us;
    enum nbcon_prio         prio;
    unsigned int            allow_unsafe_takeover   : 1;
    unsigned int            backlog                 : 1;
    struct printk_buffers   *pbufs;
    u64 seq;
};
```

**Members**

`console`
:   The associated console

`spinwait_max_us`
:   Limit for spin-wait acquire

`prio`
:   Priority of the context

`allow_unsafe_takeover`
:   Allow performing takeover even if unsafe. Can
    be used only with NBCON\_PRIO\_PANIC **prio**. It
    might cause a system freeze when the console
    is used later.

`backlog`
:   Ringbuffer has pending records

`pbufs`
:   Pointer to the text buffer for this context

`seq`
:   The sequence number to print for this context

struct nbcon\_write\_context
:   Context handed to the nbcon write callbacks

**Definition**:

```
struct nbcon_write_context {
    struct nbcon_context    ctxt;
    char *outbuf;
    unsigned int            len;
    bool unsafe_takeover;
#ifdef CONFIG_PRINTK_EXECUTION_CTX;
    int cpu;
    pid_t pid;
    char comm[TASK_COMM_LEN];
#endif;
};
```

**Members**

`ctxt`
:   The core console context

`outbuf`
:   Pointer to the text buffer for output

`len`
:   Length to write

`unsafe_takeover`
:   If a hostile takeover in an unsafe state has occurred

`cpu`
:   CPU on which the message was generated

`pid`
:   PID of the task that generated the message

`comm`
:   Name of the task that generated the message

## [Struct Consw](#id4)

struct consw
:   callbacks for consoles

**Definition**:

```
struct consw {
    struct module *owner;
    const char *(*con_startup)(void);
    void (*con_init)(struct vc_data *vc, bool init);
    void (*con_deinit)(struct vc_data *vc);
    void (*con_clear)(struct vc_data *vc, unsigned int y, unsigned int x, unsigned int count);
    void (*con_putc)(struct vc_data *vc, u16 ca, unsigned int y, unsigned int x);
    void (*con_putcs)(struct vc_data *vc, const u16 *s, unsigned int count, unsigned int ypos, unsigned int xpos);
    void (*con_cursor)(struct vc_data *vc, bool enable);
    bool (*con_scroll)(struct vc_data *vc, unsigned int top, unsigned int bottom, enum con_scroll dir, unsigned int lines);
    bool (*con_switch)(struct vc_data *vc);
    bool (*con_blank)(struct vc_data *vc, enum vesa_blank_mode blank, bool mode_switch);
    int (*con_font_set)(struct vc_data *vc, const struct console_font *font, unsigned int vpitch, unsigned int flags);
    int (*con_font_get)(struct vc_data *vc, struct console_font *font, unsigned int vpitch);
    int (*con_font_default)(struct vc_data *vc, struct console_font *font, const char *name);
    int (*con_resize)(struct vc_data *vc, unsigned int width, unsigned int height, bool from_user);
    void (*con_set_palette)(struct vc_data *vc, const unsigned char *table);
    void (*con_scrolldelta)(struct vc_data *vc, int lines);
    bool (*con_set_origin)(struct vc_data *vc);
    void (*con_save_screen)(struct vc_data *vc);
    u8 (*con_build_attr)(struct vc_data *vc, u8 color, enum vc_intensity intensity, bool blink, bool underline, bool reverse, bool italic);
    void (*con_invert_region)(struct vc_data *vc, u16 *p, int count);
};
```

**Members**

`owner`
:   the module to get references of when this console is used

`con_startup`
:   set up the console and return its name (like VGA, EGA, ...)

`con_init`
:   initialize the console on **vc**. **init** is true for the very first
    call on this **vc**.

`con_deinit`
:   deinitialize the console from **vc**.

`con_clear`
:   erase **count** characters at [**x**, **y**] on **vc**. **count** >= 1.

`con_putc`
:   emit one character with attributes **ca** to [**x**, **y**] on **vc**.
    (optional -- **con\_putcs** would be called instead)

`con_putcs`
:   emit **count** characters with attributes **s** to [**x**, **y**] on **vc**.

`con_cursor`
:   enable/disable cursor depending on **enable**

`con_scroll`
:   move lines from **top** to **bottom** in direction **dir** by **lines**.
    Return true if no generic handling should be done.
    Invoked by csi\_M and printing to the console.

`con_switch`
:   notifier about the console switch; it is supposed to return
    true if a redraw is needed.

`con_blank`
:   blank/unblank the console. The target mode is passed in **blank**.
    **mode\_switch** is set if changing from/to text/graphics. The hook
    is supposed to return true if a redraw is needed.

`con_font_set`
:   set console **vc** font to **font** with height **vpitch**. **flags** can
    be `KD_FONT_FLAG_DONT_RECALC`. (optional)

`con_font_get`
:   fetch the current font on **vc** of height **vpitch** into **font**.
    (optional)

`con_font_default`
:   set default font on **vc**. **name** can be `NULL` or font name
    to search for. **font** can be filled back. (optional)

`con_resize`
:   resize the **vc** console to **width** x **height**. **from\_user** is true
    when this change comes from the user space.

`con_set_palette`
:   sets the palette of the console **vc** to **table** (optional)

`con_scrolldelta`
:   the contents of the console should be scrolled by **lines**.
    Invoked by user. (optional)

`con_set_origin`
:   set origin (see `vc_data`::vc\_origin) of the **vc**. If not
    provided or returns false, the origin is set to
    **vc->vc\_screenbuf**. (optional)

`con_save_screen`
:   save screen content into **vc->vc\_screenbuf**. Called e.g.
    upon entering graphics. (optional)

`con_build_attr`
:   build attributes based on **color**, **intensity** and other
    parameters. The result is used for both normal and erase
    characters. (optional)

`con_invert_region`
:   invert a region of length **count** on **vc** starting at **p**.
    (optional)

## [Console functions](#id5)

short console\_srcu\_read\_flags(const struct [console](#c.console "console") \*con)
:   Locklessly read flags of a possibly registered console

**Parameters**

`const struct console *con`
:   [`struct console`](#c.console "console") pointer of console to read flags from

**Description**

Locklessly reading **con->flags** provides a consistent read value because
there is at most one CPU modifying **con->flags** and that CPU is using only
read-modify-write operations to do so.

Requires console\_srcu\_read\_lock to be held, which implies that **con** might
be a registered console. The purpose of holding console\_srcu\_read\_lock is
to guarantee that the console state is valid (CON\_SUSPENDED/CON\_ENABLED)
and that no exit/cleanup routines will run if the console is currently
undergoing unregistration.

If the caller is holding the console\_list\_lock or it is \_certain\_ that
**con** is not and will not become registered, the caller may read
**con->flags** directly instead.

**Context**

Any context.

**Return**

The current value of the **con->flags** field.

void console\_srcu\_write\_flags(struct [console](#c.console "console") \*con, short flags)
:   Write flags for a registered console

**Parameters**

`struct console *con`
:   [`struct console`](#c.console "console") pointer of console to write flags to

`short flags`
:   new flags value to write

**Description**

Only use this function to write flags for registered consoles. It
requires holding the console\_list\_lock.

**Context**

Any context.

for\_each\_console\_srcu

`for_each_console_srcu (con)`

> Iterator over registered consoles

**Parameters**

`con`
:   [`struct console`](#c.console "console") pointer used as loop cursor

**Description**

Although SRCU guarantees the console list will be consistent, the
[`struct console`](#c.console "console") fields may be updated by other CPUs while iterating.

Requires console\_srcu\_read\_lock to be held. Can be invoked from
any context.

for\_each\_console

`for_each_console (con)`

> Iterator over registered consoles

**Parameters**

`con`
:   [`struct console`](#c.console "console") pointer used as loop cursor

**Description**

The console list and the [`console.flags`](#c.console "console") are immutable while iterating.

Requires console\_list\_lock to be held.

void clear\_selection(void)
:   remove current selection

**Parameters**

`void`
:   no arguments

**Description**

Remove the current selection highlight, if any from the console holding the
selection.

Locking: The caller must hold the console lock.

unsigned int vc\_font\_pitch(const struct vc\_font \*font)
:   Calculates the number of bytes between two adjacent scanlines

**Parameters**

`const struct vc_font *font`
:   The VC font

**Return**

The number of bytes between two adjacent scanlines in the font data

unsigned int vc\_font\_size(const struct vc\_font \*font)
:   Calculates the size of the font data in bytes

**Parameters**

`const struct vc_font *font`
:   The VC font

**Description**

[`vc_font_size()`](#c.vc_font_size "vc_font_size") calculates the number of bytes of font data in the
font specified by **font**. The function calculates the size from the
font parameters.

**Return**

The size of the font data in bytes.

int \_\_vc\_resize(struct vc\_data \*vc, unsigned int cols, unsigned int rows, bool from\_user)
:   resize a VT

**Parameters**

`struct vc_data *vc`
:   virtual console

`unsigned int cols`
:   columns

`unsigned int rows`
:   rows

`bool from_user`
:   invoked by a user?

**Description**

Resize a virtual console as seen from the console end of things. We use the
common [`vc_do_resize()`](#c.vc_do_resize "vc_do_resize") method to update the structures.

Locking: The caller must hold the console sem to protect console internals
and **vc->port.tty**.

int con\_is\_bound(const struct [consw](#c.consw "consw") \*csw)
:   checks if driver is bound to the console

**Parameters**

`const struct consw *csw`
:   console driver

**Return**

zero if unbound, nonzero if bound

**Description**

Drivers can call this and if zero, they should release
all resources allocated on [`consw.con_startup()`](#c.consw "consw")

bool con\_is\_visible(const struct vc\_data \*vc)
:   checks whether the current console is visible

**Parameters**

`const struct vc_data *vc`
:   virtual console

**Return**

zero if not visible, nonzero if visible

void con\_debug\_enter(struct vc\_data \*vc)
:   prepare the console for the kernel debugger

**Parameters**

`struct vc_data *vc`
:   virtual console

**Description**

Called when the console is taken over by the kernel debugger, this
function needs to save the current console state, then put the console
into a state suitable for the kernel debugger.

void con\_debug\_leave(void)
:   restore console state

**Parameters**

`void`
:   no arguments

**Description**

Restore the console state to what it was before the kernel debugger
was invoked.

int do\_unregister\_con\_driver(const struct [consw](#c.consw "consw") \*csw)
:   unregister console driver from console layer

**Parameters**

`const struct consw *csw`
:   console driver

**Description**

All drivers that registers to the console layer must
call this function upon exit, or if the console driver is in a state
where it won’t be able to handle console services, such as the
framebuffer console without loaded framebuffer drivers.

The driver must unbind first prior to unregistration.

### [Internals](#id6)

int sel\_loadlut(u32 \_\_user \*lut)
:   load the LUT table

**Parameters**

`u32 __user *lut`
:   user table

**Description**

Load the LUT table from user space. Make a temporary copy so a partial
update doesn’t make a mess.

Locking: The console lock is acquired.

int set\_selection\_user(const struct tiocl\_selection \_\_user \*sel, struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty)
:   set the current selection.

**Parameters**

`const struct tiocl_selection __user *sel`
:   user selection info

`struct tty_struct *tty`
:   the console tty

**Description**

Invoked by the ioctl handle for the vt layer.

Locking: The entire selection process is managed under the console\_lock.
It’s a lot under the lock but its hardly a performance path.

int vc\_do\_resize(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct vc\_data \*vc, unsigned int cols, unsigned int lines, bool from\_user)
:   resizing method for the tty

**Parameters**

`struct tty_struct *tty`
:   tty being resized

`struct vc_data *vc`
:   virtual console private data

`unsigned int cols`
:   columns

`unsigned int lines`
:   lines

`bool from_user`
:   invoked by a user?

**Description**

Resize a virtual console, clipping according to the actual constraints. If
the caller passes a tty structure then update the termios winsize
information and perform any necessary signal handling.

Locking: Caller must hold the console semaphore. Takes the termios rwsem and
ctrl.lock of the tty IFF a tty is passed.

int vt\_resize(struct [tty\_struct](tty_struct.html#c.tty_struct "tty_struct") \*tty, struct winsize \*ws)
:   resize a VT

**Parameters**

`struct tty_struct *tty`
:   tty to resize

`struct winsize *ws`
:   winsize attributes

**Description**

Resize a virtual terminal. This is called by the tty layer as we register
our own handler for resizing. The mutual helper does all the actual work.

Locking: Takes the console sem and the called methods then take the tty
termios\_rwsem and the tty ctrl.lock in that order.

enum vc\_ctl\_state
:   control characters state of a vt

**Constants**

`ESnormal`
:   initial state, no control characters parsed

`ESesc`
:   ESC parsed

`ESsquare`
:   CSI parsed -- modifiers/parameters/ctrl chars expected

`ESgetpars`
:   CSI parsed -- parameters/ctrl chars expected

`ESgetsubpars`
:   CSI m parsed -- subparameters expected

`ESfunckey`
:   CSI [ parsed

`EShash`
:   ESC # parsed

`ESsetG0`
:   ESC ( parsed

`ESsetG1`
:   ESC ) parsed

`ESpercent`
:   ESC % parsed

`EScsiignore`
:   CSI [0x20-0x3f] parsed

`ESnonstd`
:   OSC parsed

`ESpalette`
:   OSC P parsed

`ESosc`
:   OSC [0-9] parsed

`ESANSI_first`
:   first state for ignoring ansi control sequences

`ESapc`
:   ESC \_ parsed

`ESpm`
:   ESC ^ parsed

`ESdcs`
:   ESC P parsed

`ESANSI_last`
:   last state for ignoring ansi control sequences

int vc\_sanitize\_unicode(const int c)
:   Replace invalid Unicode code points with `U+FFFD`

**Parameters**

`const int c`
:   the received code point

int vc\_translate\_unicode(struct vc\_data \*vc, int c, bool \*rescan)
:   Combine UTF-8 into Unicode in `vc_data.vc_utf_char`

**Parameters**

`struct vc_data *vc`
:   virtual console

`int c`
:   UTF-8 byte to translate

`bool *rescan`
:   set to true iff **c** wasn’t consumed here and needs to be re-processed

**Description**

* `vc_data.vc_utf_char` is the being-constructed Unicode code point.
* `vc_data.vc_utf_count` is the number of continuation bytes still expected to
  arrive.
* `vc_data.vc_npar` is the number of continuation bytes arrived so far.

**Return**

* `-1` - Input OK so far, **c** consumed, further bytes expected.
* `0xFFFD` - Possibility 1: input invalid, **c** may have been consumed (see
  :   desc. of **rescan**). Possibility 2: input OK, **c** consumed,
      `U+FFFD` is the resulting code point. `U+FFFD` is valid,
      `REPLACEMENT CHARACTER`.
* otherwise - Input OK, **c** consumed, resulting code point returned.

int vt\_kmsg\_redirect(int new)
:   sets/gets the kernel message console

**Parameters**

`int new`
:   the new virtual terminal number or -1 if the console should stay
    unchanged

**Description**

By default, the kernel messages are always printed on the current virtual
console. However, the user may modify that default with the
`TIOCL_SETKMSGREDIRECT` ioctl call.

This function sets the kernel message console to be **new**. It returns the old
virtual console number. The virtual terminal number `0` (both as parameter and
return value) means no redirection (i.e. always printed on the currently
active console).

The parameter -1 means that only the current console is returned, but the
value is not modified. You may use the macro `vt_get_kmsg_redirect()` in that
case to make the code more understandable.

When the kernel is compiled without `CONFIG_VT_CONSOLE`, this function ignores
the parameter and always returns `0`.
