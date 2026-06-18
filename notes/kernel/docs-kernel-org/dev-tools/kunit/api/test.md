# Test API

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/api/test.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Test API

This file documents all of the standard testing API.

enum kunit\_status
:   Type of result for a test or test suite

**Constants**

`KUNIT_SUCCESS`
:   Denotes the test suite has not failed nor been skipped

`KUNIT_FAILURE`
:   Denotes the test has failed.

`KUNIT_SKIPPED`
:   Denotes the test has been skipped.

struct kunit\_case
:   represents an individual test case.

**Definition**:

```
struct kunit_case {
    void (*run_case)(struct kunit *test);
    const char *name;
    const void* (*generate_params)(struct kunit *test, const void *prev, char *desc);
    struct kunit_attributes attr;
    int (*param_init)(struct kunit *test);
    void (*param_exit)(struct kunit *test);
};
```

**Members**

`run_case`
:   the function representing the actual test case.

`name`
:   the name of the test case.

`generate_params`
:   the generator function for parameterized tests.

`attr`
:   the attributes associated with the test

`param_init`
:   The init function to run before a parameterized test.

`param_exit`
:   The exit function to run after a parameterized test.

**Description**

A test case is a function with the signature,
`void (*)(struct kunit *)`
that makes expectations and assertions (see [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") and
[`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) about code under test. Each test case is associated
with a [`struct kunit_suite`](#c.kunit_suite "kunit_suite") and will be run after the suite’s init
function and followed by the suite’s exit function.

A test case should be static and should only be created with the
[`KUNIT_CASE()`](#c.KUNIT_CASE "KUNIT_CASE") macro; additionally, every array of test cases should be
terminated with an empty test case.

**Example**

```
void add_test_basic(struct kunit *test)
{
        KUNIT_EXPECT_EQ(test, 1, add(1, 0));
        KUNIT_EXPECT_EQ(test, 2, add(1, 1));
        KUNIT_EXPECT_EQ(test, 0, add(-1, 1));
        KUNIT_EXPECT_EQ(test, INT_MAX, add(0, INT_MAX));
        KUNIT_EXPECT_EQ(test, -1, add(INT_MAX, INT_MIN));
}

static struct kunit_case example_test_cases[] = {
        KUNIT_CASE(add_test_basic),
        {}
};
```

KUNIT\_CASE

`KUNIT_CASE (test_name)`

> A helper for creating a [`struct kunit_case`](#c.kunit_case "kunit_case")

**Parameters**

`test_name`
:   a reference to a test case function.

**Description**

Takes a symbol for a function representing a test case and creates a
[`struct kunit_case`](#c.kunit_case "kunit_case") object from it. See the documentation for
[`struct kunit_case`](#c.kunit_case "kunit_case") for an example on how to use it.

KUNIT\_CASE\_ATTR

`KUNIT_CASE_ATTR (test_name, attributes)`

> A helper for creating a [`struct kunit_case`](#c.kunit_case "kunit_case") with attributes

**Parameters**

`test_name`
:   a reference to a test case function.

`attributes`
:   a reference to a `struct kunit_attributes` object containing
    test attributes

KUNIT\_CASE\_SLOW

`KUNIT_CASE_SLOW (test_name)`

> A helper for creating a [`struct kunit_case`](#c.kunit_case "kunit_case") with the slow attribute

**Parameters**

`test_name`
:   a reference to a test case function.

KUNIT\_CASE\_PARAM

`KUNIT_CASE_PARAM (test_name, gen_params)`

> A helper for creation a parameterized [`struct kunit_case`](#c.kunit_case "kunit_case")

**Parameters**

`test_name`
:   a reference to a test case function.

`gen_params`
:   a reference to a parameter generator function.

**Description**

The generator function:

```
const void* gen_params(const void *prev, char *desc)
```

is used to lazily generate a series of arbitrarily typed values that fit into
a void\*. The argument **prev** is the previously returned value, which should be
used to derive the next value; **prev** is set to NULL on the initial generator
call. When no more values are available, the generator must return NULL.
Optionally write a string into **desc** (size of KUNIT\_PARAM\_DESC\_SIZE)
describing the parameter.

KUNIT\_CASE\_PARAM\_ATTR

`KUNIT_CASE_PARAM_ATTR (test_name, gen_params, attributes)`

> A helper for creating a parameterized [`struct kunit_case`](#c.kunit_case "kunit_case") with attributes

**Parameters**

`test_name`
:   a reference to a test case function.

`gen_params`
:   a reference to a parameter generator function.

`attributes`
:   a reference to a `struct kunit_attributes` object containing
    test attributes

KUNIT\_CASE\_PARAM\_WITH\_INIT

`KUNIT_CASE_PARAM_WITH_INIT (test_name, gen_params, init, exit)`

> Define a parameterized KUnit test case with custom `param_init()` and `param_exit()` functions.

**Parameters**

`test_name`
:   The function implementing the test case.

`gen_params`
:   The function to generate parameters for the test case.

`init`
:   A reference to the `param_init()` function to run before a parameterized test.

`exit`
:   A reference to the `param_exit()` function to run after a parameterized test.

**Description**

Provides the option to register `param_init()` and `param_exit()` functions.
param\_init/exit will be passed the parameterized test context and run once
before and once after the parameterized test. The init function can be used
to add resources to share between parameter runs, pass parameter arrays,
and any other setup logic. The exit function can be used to clean up resources
that were not managed by the parameterized test, and any other teardown logic.

**Note**

If you are registering a parameter array in `param_init()` with
`kunit_register_param_array()` then you need to pass `kunit_array_gen_params()`
to this as the generator function.

struct kunit\_suite
:   describes a related collection of [`struct kunit_case`](#c.kunit_case "kunit_case")

**Definition**:

```
struct kunit_suite {
    const char name[256];
    int (*suite_init)(struct kunit_suite *suite);
    void (*suite_exit)(struct kunit_suite *suite);
    int (*init)(struct kunit *test);
    void (*exit)(struct kunit *test);
    struct kunit_case *test_cases;
    struct kunit_attributes attr;
};
```

**Members**

`name`
:   the name of the test. Purely informational.

`suite_init`
:   called once per test suite before the test cases.

`suite_exit`
:   called once per test suite after all test cases.

`init`
:   called before every test case.

`exit`
:   called after every test case.

`test_cases`
:   a null terminated array of test cases.

`attr`
:   the attributes associated with the test suite

**Description**

A kunit\_suite is a collection of related [`struct kunit_case`](#c.kunit_case "kunit_case") s, such that
**init** is called before every test case and **exit** is called after every
test case, similar to the notion of a *test fixture* or a *test class*
in other unit testing frameworks like JUnit or Googletest.

Note that **exit** and **suite\_exit** will run even if **init** or **suite\_init**
fail: make sure they can handle any inconsistent state which may result.

Every [`struct kunit_case`](#c.kunit_case "kunit_case") must be associated with a kunit\_suite for KUnit
to run it.

struct kunit
:   represents a running instance of a test.

**Definition**:

```
struct kunit {
    void *priv;
    struct kunit *parent;
    struct kunit_params params_array;
};
```

**Members**

`priv`
:   for user to store arbitrary data. Commonly used to pass data
    created in the init function (see [`struct kunit_suite`](#c.kunit_suite "kunit_suite")).

`parent`
:   reference to the parent context of type [`struct kunit`](#c.kunit "kunit") that can
    be used for storing shared resources.

`params_array`
:   for storing the parameter array.

**Description**

Used to store information about the current context under which the test
is running. Most of this data is private and should only be accessed
indirectly via public functions; the exceptions are **priv**, **parent** and
**params\_array** which can be used by the test writer to store arbitrary data,
access the parent context, and to store the parameter array, respectively.

kunit\_test\_suites

`kunit_test_suites (__suites...)`

> used to register one or more [`struct kunit_suite`](#c.kunit_suite "kunit_suite") with KUnit.

**Parameters**

`__suites...`
:   a statically allocated list of [`struct kunit_suite`](#c.kunit_suite "kunit_suite").

**Description**

Registers **suites** with the test framework.
This is done by placing the array of [`struct kunit_suite`](#c.kunit_suite "kunit_suite") \* in the
.kunit\_test\_suites ELF section.

When builtin, KUnit tests are all run via the executor at boot, and when
built as a module, they run on module load.

kunit\_test\_init\_section\_suites

`kunit_test_init_section_suites (__suites...)`

> used to register one or more [`struct kunit_suite`](#c.kunit_suite "kunit_suite") containing init functions or init data.

**Parameters**

`__suites...`
:   a statically allocated list of [`struct kunit_suite`](#c.kunit_suite "kunit_suite").

**Description**

This functions similar to [`kunit_test_suites()`](#c.kunit_test_suites "kunit_test_suites") except that it compiles the
list of suites during init phase.

This macro also suffixes the array and suite declarations it makes with
\_probe; so that modpost suppresses warnings about referencing init data
for symbols named in this manner.

**Note**

these init tests are not able to be run after boot so there is no
“run” debugfs file generated for these tests.

Also, do not mark the suite or test case structs with \_\_initdata because
they will be used after the init phase with debugfs.

void \*kunit\_kmalloc\_array(struct [kunit](#c.kunit "kunit") \*test, size\_t n, size\_t size, gfp\_t gfp)
:   Like [`kmalloc_array()`](../../../core-api/mm-api.html#c.kmalloc_array "kmalloc_array") except the allocation is *test managed*.

**Parameters**

`struct kunit *test`
:   The test context object.

`size_t n`
:   number of elements.

`size_t size`
:   The size in bytes of the desired memory.

`gfp_t gfp`
:   flags passed to underlying [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc").

**Description**

Just like kmalloc\_array(...), except the allocation is managed by the test case
and is automatically cleaned up after the test case concludes. See [`kunit_add_action()`](resource.html#c.kunit_add_action "kunit_add_action")
for more information.

Note that some internal context data is also allocated with GFP\_KERNEL,
regardless of the gfp passed in.

void \*kunit\_kmalloc(struct [kunit](#c.kunit "kunit") \*test, size\_t size, gfp\_t gfp)
:   Like [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc") except the allocation is *test managed*.

**Parameters**

`struct kunit *test`
:   The test context object.

`size_t size`
:   The size in bytes of the desired memory.

`gfp_t gfp`
:   flags passed to underlying [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc").

**Description**

See [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc") and [`kunit_kmalloc_array()`](#c.kunit_kmalloc_array "kunit_kmalloc_array") for more information.

Note that some internal context data is also allocated with GFP\_KERNEL,
regardless of the gfp passed in.

void kunit\_kfree(struct [kunit](#c.kunit "kunit") \*test, const void \*ptr)
:   Like kfree except for allocations managed by KUnit.

**Parameters**

`struct kunit *test`
:   The test case to which the resource belongs.

`const void *ptr`
:   The memory allocation to free.

void \*kunit\_kzalloc(struct [kunit](#c.kunit "kunit") \*test, size\_t size, gfp\_t gfp)
:   Just like [`kunit_kmalloc()`](#c.kunit_kmalloc "kunit_kmalloc"), but zeroes the allocation.

**Parameters**

`struct kunit *test`
:   The test context object.

`size_t size`
:   The size in bytes of the desired memory.

`gfp_t gfp`
:   flags passed to underlying [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc").

**Description**

See [`kzalloc()`](../../../core-api/mm-api.html#c.kzalloc "kzalloc") and [`kunit_kmalloc_array()`](#c.kunit_kmalloc_array "kunit_kmalloc_array") for more information.

void \*kunit\_kcalloc(struct [kunit](#c.kunit "kunit") \*test, size\_t n, size\_t size, gfp\_t gfp)
:   Just like [`kunit_kmalloc_array()`](#c.kunit_kmalloc_array "kunit_kmalloc_array"), but zeroes the allocation.

**Parameters**

`struct kunit *test`
:   The test context object.

`size_t n`
:   number of elements.

`size_t size`
:   The size in bytes of the desired memory.

`gfp_t gfp`
:   flags passed to underlying [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc").

**Description**

See [`kcalloc()`](../../../core-api/mm-api.html#c.kcalloc "kcalloc") and [`kunit_kmalloc_array()`](#c.kunit_kmalloc_array "kunit_kmalloc_array") for more information.

void kunit\_kfree\_const(struct [kunit](#c.kunit "kunit") \*test, const void \*x)
:   conditionally free test managed memory

**Parameters**

`struct kunit *test`
:   The test context object.

`const void *x`
:   pointer to the memory

**Description**

Calls [`kunit_kfree()`](#c.kunit_kfree "kunit_kfree") only if **x** is not in .rodata section.
See [`kunit_kstrdup_const()`](#c.kunit_kstrdup_const "kunit_kstrdup_const") for more information.

char \*kunit\_kstrdup(struct [kunit](#c.kunit "kunit") \*test, const char \*str, gfp\_t gfp)
:   Duplicates a string into a test managed allocation.

**Parameters**

`struct kunit *test`
:   The test context object.

`const char *str`
:   The NULL-terminated string to duplicate.

`gfp_t gfp`
:   flags passed to underlying [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc").

**Description**

See [`kstrdup()`](../../../core-api/kernel-api.html#c.kstrdup "kstrdup") and [`kunit_kmalloc_array()`](#c.kunit_kmalloc_array "kunit_kmalloc_array") for more information.

const char \*kunit\_kstrdup\_const(struct [kunit](#c.kunit "kunit") \*test, const char \*str, gfp\_t gfp)
:   Conditionally duplicates a string into a test managed allocation.

**Parameters**

`struct kunit *test`
:   The test context object.

`const char *str`
:   The NULL-terminated string to duplicate.

`gfp_t gfp`
:   flags passed to underlying [`kmalloc()`](../../../core-api/mm-api.html#c.kmalloc "kmalloc").

**Description**

Calls [`kunit_kstrdup()`](#c.kunit_kstrdup "kunit_kstrdup") only if **str** is not in the rodata section. Must be freed with
[`kunit_kfree_const()`](#c.kunit_kfree_const "kunit_kfree_const") -- not [`kunit_kfree()`](#c.kunit_kfree "kunit_kfree").
See [`kstrdup_const()`](../../../core-api/kernel-api.html#c.kstrdup_const "kstrdup_const") and [`kunit_kmalloc_array()`](#c.kunit_kmalloc_array "kunit_kmalloc_array") for more information.

int kunit\_attach\_mm(void)
:   Create and attach a new mm if it doesn’t already exist.

**Parameters**

`void`
:   no arguments

**Description**

Allocates a `struct mm_struct` and attaches it to **current**. In most cases, call
[`kunit_vm_mmap()`](#c.kunit_vm_mmap "kunit_vm_mmap") without calling [`kunit_attach_mm()`](#c.kunit_attach_mm "kunit_attach_mm") directly. Only necessary when
code under test accesses the mm before executing the mmap (e.g., to perform
additional initialization beforehand).

**Return**

0 on success, -errno on failure.

unsigned long kunit\_vm\_mmap(struct [kunit](#c.kunit "kunit") \*test, struct [file](#c.kunit_vm_mmap "file") \*file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flag, unsigned long offset)
:   Allocate KUnit-tracked `vm_mmap()` area

**Parameters**

`struct kunit *test`
:   The test context object.

`struct file *file`
:   [`struct file`](../../../filesystems/api-summary.html#c.file "file") pointer to map from, if any

`unsigned long addr`
:   desired address, if any

`unsigned long len`
:   how many bytes to allocate

`unsigned long prot`
:   mmap PROT\_\* bits

`unsigned long flag`
:   mmap flags

`unsigned long offset`
:   offset into **file** to start mapping from.

**Description**

See `vm_mmap()` for more information.

kunit\_mark\_skipped

`kunit_mark_skipped (test, fmt, ...)`

> Marks **test** as skipped

**Parameters**

`test`
:   The test context object.

`fmt`
:   A [`printk()`](../../../core-api/printk-basics.html#c.printk "printk") style format string.

`...`
:   variable arguments

**Description**

Marks the test as skipped. **fmt** is given output as the test status
comment, typically the reason the test was skipped.

Test execution continues after [`kunit_mark_skipped()`](#c.kunit_mark_skipped "kunit_mark_skipped") is called.

kunit\_skip

`kunit_skip (test, fmt, ...)`

> Marks **test** as skipped

**Parameters**

`test`
:   The test context object.

`fmt`
:   A [`printk()`](../../../core-api/printk-basics.html#c.printk "printk") style format string.

`...`
:   variable arguments

**Description**

Skips the test. **fmt** is given output as the test status
comment, typically the reason the test was skipped.

Test execution is halted after [`kunit_skip()`](#c.kunit_skip "kunit_skip") is called.

kunit\_info

`kunit_info (test, fmt, ...)`

> Prints an INFO level message associated with **test**.

**Parameters**

`test`
:   The test context object.

`fmt`
:   A [`printk()`](../../../core-api/printk-basics.html#c.printk "printk") style format string.

`...`
:   variable arguments

**Description**

Prints an info level message associated with the test suite being run.
Takes a variable number of format parameters just like [`printk()`](../../../core-api/printk-basics.html#c.printk "printk").

kunit\_warn

`kunit_warn (test, fmt, ...)`

> Prints a WARN level message associated with **test**.

**Parameters**

`test`
:   The test context object.

`fmt`
:   A [`printk()`](../../../core-api/printk-basics.html#c.printk "printk") style format string.

`...`
:   variable arguments

**Description**

Prints a warning level message.

kunit\_err

`kunit_err (test, fmt, ...)`

> Prints an ERROR level message associated with **test**.

**Parameters**

`test`
:   The test context object.

`fmt`
:   A [`printk()`](../../../core-api/printk-basics.html#c.printk "printk") style format string.

`...`
:   variable arguments

**Description**

Prints an error level message.

KUNIT\_SUCCEED

`KUNIT_SUCCEED (test)`

> A no-op expectation. Only exists for code clarity.

**Parameters**

`test`
:   The test context object.

**Description**

The opposite of [`KUNIT_FAIL()`](#c.KUNIT_FAIL "KUNIT_FAIL"), it is an expectation that cannot fail. In other
words, it does nothing and only exists for code clarity. See
[`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for more information.

KUNIT\_FAIL

`KUNIT_FAIL (test, fmt, ...)`

> Always causes a test to fail when evaluated.

**Parameters**

`test`
:   The test context object.

`fmt`
:   an informational message to be printed when the assertion is made.

`...`
:   string format arguments.

**Description**

The opposite of [`KUNIT_SUCCEED()`](#c.KUNIT_SUCCEED "KUNIT_SUCCEED"), it is an expectation that always fails. In
other words, it always results in a failed expectation, and consequently
always causes the test case to fail when evaluated. See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE")
for more information.

KUNIT\_EXPECT\_TRUE

`KUNIT_EXPECT_TRUE (test, condition)`

> Causes a test failure when the expression is not true.

**Parameters**

`test`
:   The test context object.

`condition`
:   an arbitrary boolean expression. The test fails when this does
    not evaluate to true.

**Description**

This and expectations of the form KUNIT\_EXPECT\_\* will cause the test case
to fail when the specified condition is not met; however, it will not prevent
the test case from continuing to run; this is otherwise known as an
*expectation failure*.

KUNIT\_EXPECT\_FALSE

`KUNIT_EXPECT_FALSE (test, condition)`

> Makes a test failure when the expression is not false.

**Parameters**

`test`
:   The test context object.

`condition`
:   an arbitrary boolean expression. The test fails when this does
    not evaluate to false.

**Description**

Sets an expectation that **condition** evaluates to false. See
[`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for more information.

KUNIT\_EXPECT\_EQ

`KUNIT_EXPECT_EQ (test, left, right)`

> Sets an expectation that **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are
equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) == (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_PTR\_EQ

`KUNIT_EXPECT_PTR_EQ (test, left, right)`

> Expects that pointers **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a pointer.

`right`
:   an arbitrary expression that evaluates to a pointer.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are
equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) == (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_NE

`KUNIT_EXPECT_NE (test, left, right)`

> An expectation that **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are not
equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) != (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_PTR\_NE

`KUNIT_EXPECT_PTR_NE (test, left, right)`

> Expects that pointers **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a pointer.

`right`
:   an arbitrary expression that evaluates to a pointer.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are not
equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) != (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_LT

`KUNIT_EXPECT_LT (test, left, right)`

> An expectation that **left** is less than **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an expectation that the value that **left** evaluates to is less than the
value that **right** evaluates to. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) < (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_LE

`KUNIT_EXPECT_LE (test, left, right)`

> Expects that **left** is less than or equal to **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an expectation that the value that **left** evaluates to is less than or
equal to the value that **right** evaluates to. Semantically this is equivalent
to KUNIT\_EXPECT\_TRUE(**test**, (**left**) <= (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_GT

`KUNIT_EXPECT_GT (test, left, right)`

> An expectation that **left** is greater than **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an expectation that the value that **left** evaluates to is greater than
the value that **right** evaluates to. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) > (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_GE

`KUNIT_EXPECT_GE (test, left, right)`

> Expects that **left** is greater than or equal to **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an expectation that the value that **left** evaluates to is greater than
the value that **right** evaluates to. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, (**left**) >= (**right**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_EXPECT\_STREQ

`KUNIT_EXPECT_STREQ (test, left, right)`

> Expects that strings **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a null terminated string.

`right`
:   an arbitrary expression that evaluates to a null terminated string.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are
equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, !strcmp((**left**), (**right**))). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE")
for more information.

KUNIT\_EXPECT\_STRNEQ

`KUNIT_EXPECT_STRNEQ (test, left, right)`

> Expects that strings **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a null terminated string.

`right`
:   an arbitrary expression that evaluates to a null terminated string.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are
not equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, strcmp((**left**), (**right**))). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE")
for more information.

KUNIT\_EXPECT\_MEMEQ

`KUNIT_EXPECT_MEMEQ (test, left, right, size)`

> Expects that the first **size** bytes of **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   An arbitrary expression that evaluates to the specified size.

`right`
:   An arbitrary expression that evaluates to the specified size.

`size`
:   Number of bytes compared.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are
equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, !memcmp((**left**), (**right**), (**size**))). See
[`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for more information.

Although this expectation works for any memory block, it is not recommended
for comparing more structured data, such as structs. This expectation is
recommended for comparing, for example, data arrays.

KUNIT\_EXPECT\_MEMNEQ

`KUNIT_EXPECT_MEMNEQ (test, left, right, size)`

> Expects that the first **size** bytes of **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   An arbitrary expression that evaluates to the specified size.

`right`
:   An arbitrary expression that evaluates to the specified size.

`size`
:   Number of bytes compared.

**Description**

Sets an expectation that the values that **left** and **right** evaluate to are
not equal. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, memcmp((**left**), (**right**), (**size**))). See
[`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for more information.

Although this expectation works for any memory block, it is not recommended
for comparing more structured data, such as structs. This expectation is
recommended for comparing, for example, data arrays.

KUNIT\_EXPECT\_NULL

`KUNIT_EXPECT_NULL (test, ptr)`

> Expects that **ptr** is null.

**Parameters**

`test`
:   The test context object.

`ptr`
:   an arbitrary pointer.

**Description**

Sets an expectation that the value that **ptr** evaluates to is null. This is
semantically equivalent to KUNIT\_EXPECT\_PTR\_EQ(**test**, ptr, NULL).
See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for more information.

KUNIT\_EXPECT\_NOT\_NULL

`KUNIT_EXPECT_NOT_NULL (test, ptr)`

> Expects that **ptr** is not null.

**Parameters**

`test`
:   The test context object.

`ptr`
:   an arbitrary pointer.

**Description**

Sets an expectation that the value that **ptr** evaluates to is not null. This
is semantically equivalent to KUNIT\_EXPECT\_PTR\_NE(**test**, ptr, NULL).
See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for more information.

KUNIT\_EXPECT\_NOT\_ERR\_OR\_NULL

`KUNIT_EXPECT_NOT_ERR_OR_NULL (test, ptr)`

> Expects that **ptr** is not null and not err.

**Parameters**

`test`
:   The test context object.

`ptr`
:   an arbitrary pointer.

**Description**

Sets an expectation that the value that **ptr** evaluates to is not null and not
an errno stored in a pointer. This is semantically equivalent to
KUNIT\_EXPECT\_TRUE(**test**, !IS\_ERR\_OR\_NULL(**ptr**)). See [`KUNIT_EXPECT_TRUE()`](#c.KUNIT_EXPECT_TRUE "KUNIT_EXPECT_TRUE") for
more information.

KUNIT\_FAIL\_AND\_ABORT

`KUNIT_FAIL_AND_ABORT (test, fmt, ...)`

> Always causes a test to fail and abort when evaluated.

**Parameters**

`test`
:   The test context object.

`fmt`
:   an informational message to be printed when the assertion is made.

`...`
:   string format arguments.

**Description**

The opposite of [`KUNIT_SUCCEED()`](#c.KUNIT_SUCCEED "KUNIT_SUCCEED"), it is an assertion that always fails. In
other words, it always results in a failed assertion, and consequently
always causes the test case to fail and abort when evaluated.
See [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE") for more information.

KUNIT\_ASSERT\_TRUE

`KUNIT_ASSERT_TRUE (test, condition)`

> Sets an assertion that **condition** is true.

**Parameters**

`test`
:   The test context object.

`condition`
:   an arbitrary boolean expression. The test fails and aborts when
    this does not evaluate to true.

**Description**

This and assertions of the form KUNIT\_ASSERT\_\* will cause the test case to
fail *and immediately abort* when the specified condition is not met. Unlike
an expectation failure, it will prevent the test case from continuing to run;
this is otherwise known as an *assertion failure*.

KUNIT\_ASSERT\_FALSE

`KUNIT_ASSERT_FALSE (test, condition)`

> Sets an assertion that **condition** is false.

**Parameters**

`test`
:   The test context object.

`condition`
:   an arbitrary boolean expression.

**Description**

Sets an assertion that the value that **condition** evaluates to is false. This
is the same as [`KUNIT_EXPECT_FALSE()`](#c.KUNIT_EXPECT_FALSE "KUNIT_EXPECT_FALSE"), except it causes an assertion failure
(see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_EQ

`KUNIT_ASSERT_EQ (test, left, right)`

> Sets an assertion that **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are
equal. This is the same as [`KUNIT_EXPECT_EQ()`](#c.KUNIT_EXPECT_EQ "KUNIT_EXPECT_EQ"), except it causes an assertion
failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_PTR\_EQ

`KUNIT_ASSERT_PTR_EQ (test, left, right)`

> Asserts that pointers **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a pointer.

`right`
:   an arbitrary expression that evaluates to a pointer.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are
equal. This is the same as [`KUNIT_EXPECT_EQ()`](#c.KUNIT_EXPECT_EQ "KUNIT_EXPECT_EQ"), except it causes an assertion
failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_NE

`KUNIT_ASSERT_NE (test, left, right)`

> An assertion that **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are not
equal. This is the same as [`KUNIT_EXPECT_NE()`](#c.KUNIT_EXPECT_NE "KUNIT_EXPECT_NE"), except it causes an assertion
failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_PTR\_NE

`KUNIT_ASSERT_PTR_NE (test, left, right)`

> Asserts that pointers **left** and **right** are not equal. [`KUNIT_ASSERT_PTR_EQ()`](#c.KUNIT_ASSERT_PTR_EQ "KUNIT_ASSERT_PTR_EQ") - Asserts that pointers **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a pointer.

`right`
:   an arbitrary expression that evaluates to a pointer.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are not
equal. This is the same as [`KUNIT_EXPECT_NE()`](#c.KUNIT_EXPECT_NE "KUNIT_EXPECT_NE"), except it causes an assertion
failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_LT

`KUNIT_ASSERT_LT (test, left, right)`

> An assertion that **left** is less than **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an assertion that the value that **left** evaluates to is less than the
value that **right** evaluates to. This is the same as [`KUNIT_EXPECT_LT()`](#c.KUNIT_EXPECT_LT "KUNIT_EXPECT_LT"), except
it causes an assertion failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion
is not met.

KUNIT\_ASSERT\_LE

`KUNIT_ASSERT_LE (test, left, right)`

> An assertion that **left** is less than or equal to **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an assertion that the value that **left** evaluates to is less than or
equal to the value that **right** evaluates to. This is the same as
[`KUNIT_EXPECT_LE()`](#c.KUNIT_EXPECT_LE "KUNIT_EXPECT_LE"), except it causes an assertion failure (see
[`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_GT

`KUNIT_ASSERT_GT (test, left, right)`

> An assertion that **left** is greater than **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an assertion that the value that **left** evaluates to is greater than the
value that **right** evaluates to. This is the same as [`KUNIT_EXPECT_GT()`](#c.KUNIT_EXPECT_GT "KUNIT_EXPECT_GT"), except
it causes an assertion failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion
is not met.

KUNIT\_ASSERT\_GE

`KUNIT_ASSERT_GE (test, left, right)`

> Assertion that **left** is greater than or equal to **right**.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a primitive C type.

`right`
:   an arbitrary expression that evaluates to a primitive C type.

**Description**

Sets an assertion that the value that **left** evaluates to is greater than the
value that **right** evaluates to. This is the same as [`KUNIT_EXPECT_GE()`](#c.KUNIT_EXPECT_GE "KUNIT_EXPECT_GE"), except
it causes an assertion failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion
is not met.

KUNIT\_ASSERT\_STREQ

`KUNIT_ASSERT_STREQ (test, left, right)`

> An assertion that strings **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a null terminated string.

`right`
:   an arbitrary expression that evaluates to a null terminated string.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are
equal. This is the same as [`KUNIT_EXPECT_STREQ()`](#c.KUNIT_EXPECT_STREQ "KUNIT_EXPECT_STREQ"), except it causes an
assertion failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_STRNEQ

`KUNIT_ASSERT_STRNEQ (test, left, right)`

> An assertion that strings **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   an arbitrary expression that evaluates to a null terminated string.

`right`
:   an arbitrary expression that evaluates to a null terminated string.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are
not equal. This is semantically equivalent to
KUNIT\_ASSERT\_TRUE(**test**, strcmp((**left**), (**right**))). See [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")
for more information.

KUNIT\_ASSERT\_MEMEQ

`KUNIT_ASSERT_MEMEQ (test, left, right, size)`

> Asserts that the first **size** bytes of **left** and **right** are equal.

**Parameters**

`test`
:   The test context object.

`left`
:   An arbitrary expression that evaluates to the specified size.

`right`
:   An arbitrary expression that evaluates to the specified size.

`size`
:   Number of bytes compared.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are
equal. This is semantically equivalent to
KUNIT\_ASSERT\_TRUE(**test**, !memcmp((**left**), (**right**), (**size**))). See
[`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE") for more information.

Although this assertion works for any memory block, it is not recommended
for comparing more structured data, such as structs. This assertion is
recommended for comparing, for example, data arrays.

KUNIT\_ASSERT\_MEMNEQ

`KUNIT_ASSERT_MEMNEQ (test, left, right, size)`

> Asserts that the first **size** bytes of **left** and **right** are not equal.

**Parameters**

`test`
:   The test context object.

`left`
:   An arbitrary expression that evaluates to the specified size.

`right`
:   An arbitrary expression that evaluates to the specified size.

`size`
:   Number of bytes compared.

**Description**

Sets an assertion that the values that **left** and **right** evaluate to are
not equal. This is semantically equivalent to
KUNIT\_ASSERT\_TRUE(**test**, memcmp((**left**), (**right**), (**size**))). See
[`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE") for more information.

Although this assertion works for any memory block, it is not recommended
for comparing more structured data, such as structs. This assertion is
recommended for comparing, for example, data arrays.

KUNIT\_ASSERT\_NULL

`KUNIT_ASSERT_NULL (test, ptr)`

> Asserts that pointers **ptr** is null.

**Parameters**

`test`
:   The test context object.

`ptr`
:   an arbitrary pointer.

**Description**

Sets an assertion that the values that **ptr** evaluates to is null. This is
the same as [`KUNIT_EXPECT_NULL()`](#c.KUNIT_EXPECT_NULL "KUNIT_EXPECT_NULL"), except it causes an assertion
failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_NOT\_NULL

`KUNIT_ASSERT_NOT_NULL (test, ptr)`

> Asserts that pointers **ptr** is not null.

**Parameters**

`test`
:   The test context object.

`ptr`
:   an arbitrary pointer.

**Description**

Sets an assertion that the values that **ptr** evaluates to is not null. This
is the same as [`KUNIT_EXPECT_NOT_NULL()`](#c.KUNIT_EXPECT_NOT_NULL "KUNIT_EXPECT_NOT_NULL"), except it causes an assertion
failure (see [`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ASSERT\_NOT\_ERR\_OR\_NULL

`KUNIT_ASSERT_NOT_ERR_OR_NULL (test, ptr)`

> Assertion that **ptr** is not null and not err.

**Parameters**

`test`
:   The test context object.

`ptr`
:   an arbitrary pointer.

**Description**

Sets an assertion that the value that **ptr** evaluates to is not null and not
an errno stored in a pointer. This is the same as
[`KUNIT_EXPECT_NOT_ERR_OR_NULL()`](#c.KUNIT_EXPECT_NOT_ERR_OR_NULL "KUNIT_EXPECT_NOT_ERR_OR_NULL"), except it causes an assertion failure (see
[`KUNIT_ASSERT_TRUE()`](#c.KUNIT_ASSERT_TRUE "KUNIT_ASSERT_TRUE")) when the assertion is not met.

KUNIT\_ARRAY\_PARAM

`KUNIT_ARRAY_PARAM (name, array, get_desc)`

> Define test parameter generator from an array.

**Parameters**

`name`
:   prefix for the test parameter generator function.

`array`
:   array of test parameters.

`get_desc`
:   function to convert param to description; NULL to use default

**Description**

Define function **name\_gen\_params** which uses **array** to generate parameters.

KUNIT\_ARRAY\_PARAM\_DESC

`KUNIT_ARRAY_PARAM_DESC (name, array, desc_member)`

> Define test parameter generator from an array.

**Parameters**

`name`
:   prefix for the test parameter generator function.

`array`
:   array of test parameters.

`desc_member`
:   structure member from array element to use as description

**Description**

Define function **name\_gen\_params** which uses **array** to generate parameters.

kunit\_register\_params\_array

`kunit_register_params_array (test, array, param_count, get_desc)`

> Register parameter array for a KUnit test.

**Parameters**

`test`
:   The KUnit test structure to which parameters will be added.

`array`
:   An array of test parameters.

`param_count`
:   Number of parameters.

`get_desc`
:   Function that generates a string description for a given parameter
    element.

**Description**

This macro initializes the **test**’s parameter array data, storing information
including the parameter array, its count, the element size, and the parameter
description function within test->params\_array.

**Note**

If using this macro in `param_init()`, `kunit_array_gen_params()`
will then need to be manually provided as the parameter generator function to
[`KUNIT_CASE_PARAM_WITH_INIT()`](#c.KUNIT_CASE_PARAM_WITH_INIT "KUNIT_CASE_PARAM_WITH_INIT"). `kunit_array_gen_params()` is a KUnit
function that uses the registered array to generate parameters
