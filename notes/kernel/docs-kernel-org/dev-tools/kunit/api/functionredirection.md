# Function Redirection API

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/api/functionredirection.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Function Redirection API

## Overview

When writing unit tests, it’s important to be able to isolate the code being
tested from other parts of the kernel. This ensures the reliability of the test
(it won’t be affected by external factors), reduces dependencies on specific
hardware or config options (making the test easier to run), and protects the
stability of the rest of the system (making it less likely for test-specific
state to interfere with the rest of the system).

While for some code (typically generic data structures, helpers, and other
“pure functions”) this is trivial, for others (like device drivers,
filesystems, core subsystems) the code is heavily coupled with other parts of
the kernel.

This coupling is often due to global state in some way: be it a global list of
devices, the filesystem, or some hardware state. Tests need to either carefully
manage, isolate, and restore state, or they can avoid it altogether by
replacing access to and mutation of this state with a “fake” or “mock” variant.

By refactoring access to such state, such as by introducing a layer of
indirection which can use or emulate a separate set of test state. However,
such refactoring comes with its own costs (and undertaking significant
refactoring before being able to write tests is suboptimal).

A simpler way to intercept and replace some of the function calls is to use
function redirection via static stubs.

## Static Stubs

Static stubs are a way of redirecting calls to one function (the “real”
function) to another function (the “replacement” function).

It works by adding a macro to the “real” function which checks to see if a test
is running, and if a replacement function is available. If so, that function is
called in place of the original.

Using static stubs is pretty straightforward:

1. Add the [`KUNIT_STATIC_STUB_REDIRECT()`](#c.KUNIT_STATIC_STUB_REDIRECT "KUNIT_STATIC_STUB_REDIRECT") macro to the start of the “real”
   function.

   This should be the first statement in the function, after any variable
   declarations. [`KUNIT_STATIC_STUB_REDIRECT()`](#c.KUNIT_STATIC_STUB_REDIRECT "KUNIT_STATIC_STUB_REDIRECT") takes the name of the
   function, followed by all of the arguments passed to the real function.

   For example:

   ```
   void send_data_to_hardware(const char *str)
   {
           KUNIT_STATIC_STUB_REDIRECT(send_data_to_hardware, str);
           /* real implementation */
   }
   ```
2. Write one or more replacement functions.

   These functions should have the same function signature as the real function.
   In the event they need to access or modify test-specific state, they can use
   `kunit_get_current_test()` to get a [`struct kunit`](test.html#c.kunit "kunit") pointer. This can then
   be passed to the expectation/assertion macros, or used to look up KUnit
   resources.

   For example:

   ```
   void fake_send_data_to_hardware(const char *str)
   {
           struct kunit *test = kunit_get_current_test();
           KUNIT_EXPECT_STREQ(test, str, "Hello World!");
   }
   ```
3. Activate the static stub from your test.

   From within a test, the redirection can be enabled with
   [`kunit_activate_static_stub()`](#c.kunit_activate_static_stub "kunit_activate_static_stub"), which accepts a [`struct kunit`](test.html#c.kunit "kunit") pointer,
   the real function, and the replacement function. You can call this several
   times with different replacement functions to swap out implementations of the
   function.

   In our example, this would be

   ```
   kunit_activate_static_stub(test,
                              send_data_to_hardware,
                              fake_send_data_to_hardware);
   ```
4. Call (perhaps indirectly) the real function.

   Once the redirection is activated, any call to the real function will call
   the replacement function instead. Such calls may be buried deep in the
   implementation of another function, but must occur from the test’s kthread.

   For example:

   ```
   send_data_to_hardware("Hello World!"); /* Succeeds */
   send_data_to_hardware("Something else"); /* Fails the test. */
   ```
5. (Optionally) disable the stub.

   When you no longer need it, disable the redirection (and hence resume the
   original behaviour of the ‘real’ function) using
   [`kunit_deactivate_static_stub()`](#c.kunit_deactivate_static_stub "kunit_deactivate_static_stub"). Otherwise, it will be automatically disabled
   when the test exits.

   For example:

   ```
   kunit_deactivate_static_stub(test, send_data_to_hardware);
   ```

It’s also possible to use these replacement functions to test to see if a
function is called at all, for example:

```
void send_data_to_hardware(const char *str)
{
        KUNIT_STATIC_STUB_REDIRECT(send_data_to_hardware, str);
        /* real implementation */
}

/* In test file */
int times_called = 0;
void fake_send_data_to_hardware(const char *str)
{
        times_called++;
}
...
/* In the test case, redirect calls for the duration of the test */
kunit_activate_static_stub(test, send_data_to_hardware, fake_send_data_to_hardware);

send_data_to_hardware("hello");
KUNIT_EXPECT_EQ(test, times_called, 1);

/* Can also deactivate the stub early, if wanted */
kunit_deactivate_static_stub(test, send_data_to_hardware);

send_data_to_hardware("hello again");
KUNIT_EXPECT_EQ(test, times_called, 1);
```

## API Reference

KUNIT\_STATIC\_STUB\_REDIRECT

`KUNIT_STATIC_STUB_REDIRECT (real_fn_name, args...)`

> call a replacement ‘static stub’ if one exists

**Parameters**

`real_fn_name`
:   The name of this function (as an identifier, not a string)

`args...`
:   All of the arguments passed to this function

**Description**

This is a function prologue which is used to allow calls to the current
function to be redirected by a KUnit test. KUnit tests can call
[`kunit_activate_static_stub()`](#c.kunit_activate_static_stub "kunit_activate_static_stub") to pass a replacement function in. The
replacement function will be called by [`KUNIT_STATIC_STUB_REDIRECT()`](#c.KUNIT_STATIC_STUB_REDIRECT "KUNIT_STATIC_STUB_REDIRECT"), which
will then return from the function. If the caller is not in a KUnit context,
the function will continue execution as normal.

**Example**

```
int real_func(int n)
{
        KUNIT_STATIC_STUB_REDIRECT(real_func, n);
        return 0;
}

int replacement_func(int n)
{
        return 42;
}

void example_test(struct kunit *test)
{
        kunit_activate_static_stub(test, real_func, replacement_func);
        KUNIT_EXPECT_EQ(test, real_func(1), 42);
}
```

kunit\_activate\_static\_stub

`kunit_activate_static_stub (test, real_fn_addr, replacement_addr)`

> replace a function using static stubs.

**Parameters**

`test`
:   A pointer to the ‘[`struct kunit`](test.html#c.kunit "kunit")’ test context for the current test.

`real_fn_addr`
:   The address of the function to replace.

`replacement_addr`
:   The address of the function to replace it with.

**Description**

When activated, calls to real\_fn\_addr from within this test (even if called
indirectly) will instead call replacement\_addr. The function pointed to by
real\_fn\_addr must begin with the static stub prologue in
[`KUNIT_STATIC_STUB_REDIRECT()`](#c.KUNIT_STATIC_STUB_REDIRECT "KUNIT_STATIC_STUB_REDIRECT") for this to work. real\_fn\_addr and
replacement\_addr must have the same type.

The redirection can be disabled again with [`kunit_deactivate_static_stub()`](#c.kunit_deactivate_static_stub "kunit_deactivate_static_stub").

void kunit\_deactivate\_static\_stub(struct [kunit](test.html#c.kunit "kunit") \*test, void \*real\_fn\_addr)
:   disable a function redirection

**Parameters**

`struct kunit *test`
:   A pointer to the ‘[`struct kunit`](test.html#c.kunit "kunit")’ test context for the current test.

`void *real_fn_addr`
:   The address of the function to no-longer redirect

**Description**

Deactivates a redirection configured with [`kunit_activate_static_stub()`](#c.kunit_activate_static_stub "kunit_activate_static_stub"). After
this function returns, calls to `real_fn_addr()` will execute the original
real\_fn, not any previously-configured replacement.
