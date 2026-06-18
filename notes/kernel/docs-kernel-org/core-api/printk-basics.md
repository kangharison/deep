# Message logging with printk

> 출처(원문): https://docs.kernel.org/core-api/printk-basics.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Message logging with printk

[`printk()`](#c.printk "printk") is one of the most widely known functions in the Linux kernel. It’s the
standard tool we have for printing messages and usually the most basic way of
tracing and debugging. If you’re familiar with printf(3) you can tell [`printk()`](#c.printk "printk")
is based on it, although it has some functional differences:

> * [`printk()`](#c.printk "printk") messages can specify a log level.
> * the format string, while largely compatible with C99, doesn’t follow the
>   exact same specification. It has some extensions and a few limitations
>   (no `%n` or floating point conversion specifiers). See [How to get
>   printk format specifiers right](printk-formats.html#printk-specifiers).

All [`printk()`](#c.printk "printk") messages are printed to the kernel log buffer, which is a ring
buffer exported to userspace through /dev/kmsg. The usual way to read it is
using `dmesg`.

[`printk()`](#c.printk "printk") is typically used like this:

```
printk(KERN_INFO "Message: %s\n", arg);
```

where `KERN_INFO` is the log level (note that it’s concatenated to the format
string, the log level is not a separate argument). The available log levels are:

| Name | String | Alias function |
| --- | --- | --- |
| KERN\_EMERG | “0” | [`pr_emerg()`](#c.pr_emerg "pr_emerg") |
| KERN\_ALERT | “1” | [`pr_alert()`](#c.pr_alert "pr_alert") |
| KERN\_CRIT | “2” | [`pr_crit()`](#c.pr_crit "pr_crit") |
| KERN\_ERR | “3” | [`pr_err()`](#c.pr_err "pr_err") |
| KERN\_WARNING | “4” | [`pr_warn()`](#c.pr_warn "pr_warn") |
| KERN\_NOTICE | “5” | [`pr_notice()`](#c.pr_notice "pr_notice") |
| KERN\_INFO | “6” | [`pr_info()`](#c.pr_info "pr_info") |
| KERN\_DEBUG | “7” | [`pr_debug()`](#c.pr_debug "pr_debug") and [`pr_devel()`](#c.pr_devel "pr_devel") if DEBUG is defined |
| KERN\_DEFAULT | “” |  |
| KERN\_CONT | “c” | [`pr_cont()`](#c.pr_cont "pr_cont") |

The log level specifies the importance of a message. The kernel decides whether
to show the message immediately (printing it to the current console) depending
on its log level and the current *console\_loglevel* (a kernel variable). If the
message priority is higher (lower log level value) than the *console\_loglevel*
the message will be printed to the console.

If the log level is omitted, the message is printed with `KERN_DEFAULT`
level.

You can check the current *console\_loglevel* with:

```
$ cat /proc/sys/kernel/printk
4        4        1        7
```

The result shows the *current*, *default*, *minimum* and *boot-time-default* log
levels.

To change the current console\_loglevel simply write the desired level to
`/proc/sys/kernel/printk`. For example, to print all messages to the console:

```
# echo 8 > /proc/sys/kernel/printk
```

Another way, using `dmesg`:

```
# dmesg -n 5
```

sets the console\_loglevel to print KERN\_WARNING (4) or more severe messages to
console. See `dmesg(1)` for more information.

As an alternative to [`printk()`](#c.printk "printk") you can use the `pr_*()` aliases for
logging. This family of macros embed the log level in the macro names. For
example:

```
pr_info("Info message no. %d\n", msg_num);
```

prints a `KERN_INFO` message.

Besides being more concise than the equivalent [`printk()`](#c.printk "printk") calls, they can use a
common definition for the format string through the [`pr_fmt()`](#c.pr_fmt "pr_fmt") macro. For
instance, defining this at the top of a source file (before any `#include`
directive):

```
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
```

would prefix every pr\_\*() message in that file with the module and function name
that originated the message.

For debugging purposes there are also two conditionally-compiled macros:
[`pr_debug()`](#c.pr_debug "pr_debug") and [`pr_devel()`](#c.pr_devel "pr_devel"), which are compiled-out unless `DEBUG` (or
also `CONFIG_DYNAMIC_DEBUG` in the case of [`pr_debug()`](#c.pr_debug "pr_debug")) is defined.

## Avoiding lockups from excessive printk() use

Note

This section is relevant only for legacy console drivers (those not
using the nbcon API) and !PREEMPT\_RT kernels. Once all console drivers
are updated to nbcon, this documentation can be removed.

Using `printk()` in hot paths (such as interrupt handlers, timer
callbacks, or high-frequency network receive routines) with legacy
consoles (e.g., `console=ttyS0`) may cause lockups. Legacy consoles
synchronously acquire `console_sem` and block while flushing messages,
potentially disabling interrupts long enough to trigger hard or soft
lockup detectors.

To avoid this:

* Use rate-limited variants (e.g., `pr_*_ratelimited()`) or one-time
  macros (e.g., `pr_*_once()`) to reduce message frequency.
* Assign lower log levels (e.g., `KERN_DEBUG`) to non-essential messages
  and filter console output via `console_loglevel`.
* Use `printk_deferred()` to log messages immediately to the ringbuffer
  and defer console printing. This is a workaround for legacy consoles.
* Port legacy console drivers to the non-blocking `nbcon` API (indicated
  by `CON_NBCON`). This is the preferred solution, as nbcon consoles
  offload message printing to a dedicated kernel thread.

For temporary debugging, `trace_printk()` can be used, but it must not
appear in mainline code. See `Documentation/trace/debugging.rst` for
more information.

If more permanent output is needed in a hot path, trace events can be used.
See `Documentation/trace/events.rst` and
`samples/trace_events/trace-events-sample.[ch]`.

## Function reference

pr\_fmt

`pr_fmt (fmt)`

> used by the pr\_\*() macros to generate the printk format string

**Parameters**

`fmt`
:   format string passed from a pr\_\*() macro

**Description**

This macro can be used to generate a unified format string for pr\_\*()
macros. A common use is to prefix all pr\_\*() messages in a file with a common
string. For example, defining this at the top of a source file:

> #define pr\_fmt(fmt) KBUILD\_MODNAME “: “ fmt

would prefix all pr\_info, pr\_emerg... messages in the file with the module
name.

printk

`printk (fmt, ...)`

> print a kernel message

**Parameters**

`fmt`
:   format string

`...`
:   variable arguments

**Description**

This is [`printk()`](#c.printk "printk"). It can be called from any context. We want it to work.

If printk indexing is enabled, `_printk()` is called from printk\_index\_wrap.
Otherwise, printk is simply #defined to \_printk.

We try to grab the console\_lock. If we succeed, it’s easy - we log the
output and call the console drivers. If we fail to get the semaphore, we
place the output into the log buffer and return. The current holder of
the console\_sem will notice the new output in [`console_unlock()`](../driver-api/basics.html#c.console_unlock "console_unlock"); and will
send it to the consoles before releasing the lock.

One effect of this deferred printing is that code which calls [`printk()`](#c.printk "printk") and
then changes console\_loglevel may break. This is because console\_loglevel
is inspected when the actual printing occurs.

See also:
printf(3)

See the [`vsnprintf()`](kernel-api.html#c.vsnprintf "vsnprintf") documentation for format string extensions over C99.

pr\_emerg

`pr_emerg (fmt, ...)`

> Print an emergency-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_EMERG loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to
generate the format string.

pr\_alert

`pr_alert (fmt, ...)`

> Print an alert-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_ALERT loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to
generate the format string.

pr\_crit

`pr_crit (fmt, ...)`

> Print a critical-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_CRIT loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to
generate the format string.

pr\_err

`pr_err (fmt, ...)`

> Print an error-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_ERR loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to
generate the format string.

pr\_warn

`pr_warn (fmt, ...)`

> Print a warning-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_WARNING loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt")
to generate the format string.

pr\_notice

`pr_notice (fmt, ...)`

> Print a notice-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_NOTICE loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to
generate the format string.

pr\_info

`pr_info (fmt, ...)`

> Print an info-level message

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_INFO loglevel. It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to
generate the format string.

pr\_cont

`pr_cont (fmt, ...)`

> Continues a previous log message in the same line.

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_CONT loglevel. It should only be
used when continuing a log message with no newline (’n’) enclosed. Otherwise
it defaults back to KERN\_DEFAULT loglevel.

pr\_devel

`pr_devel (fmt, ...)`

> Print a debug-level message conditionally

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to a printk with KERN\_DEBUG loglevel if DEBUG is
defined. Otherwise it does nothing.

It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to generate the format string.

pr\_debug

`pr_debug (fmt, ...)`

> Print a debug-level message conditionally

**Parameters**

`fmt`
:   format string

`...`
:   arguments for the format string

**Description**

This macro expands to `dynamic_pr_debug()` if CONFIG\_DYNAMIC\_DEBUG is
set. Otherwise, if DEBUG is defined, it’s equivalent to a printk with
KERN\_DEBUG loglevel. If DEBUG is not defined it does nothing.

It uses [`pr_fmt()`](#c.pr_fmt "pr_fmt") to generate the format string (`dynamic_pr_debug()` uses
[`pr_fmt()`](#c.pr_fmt "pr_fmt") internally).
