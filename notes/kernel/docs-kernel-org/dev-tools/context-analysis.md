# Compiler-Based Context Analysis

> 출처(원문): https://docs.kernel.org/dev-tools/context-analysis.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Compiler-Based Context Analysis

Context Analysis is a language extension, which enables statically checking
that required contexts are active (or inactive) by acquiring and releasing
user-definable “context locks”. An obvious application is lock-safety checking
for the kernel’s various synchronization primitives (each of which represents a
“context lock”), and checking that locking rules are not violated.

The Clang compiler currently supports the full set of context analysis
features. To enable for Clang, configure the kernel with:

```
CONFIG_WARN_CONTEXT_ANALYSIS=y
```

The feature requires Clang 22 or later.

The analysis is *opt-in by default*, and requires declaring which modules and
subsystems should be analyzed in the respective Makefile:

```
CONTEXT_ANALYSIS_mymodule.o := y
```

Or for all translation units in the directory:

```
CONTEXT_ANALYSIS := y
```

It is possible to enable the analysis tree-wide, however, which will result in
numerous false positive warnings currently and is *not* generally recommended:

```
CONFIG_WARN_CONTEXT_ANALYSIS_ALL=y
```

## Programming Model

The below describes the programming model around using context lock types.

Note

Enabling context analysis can be seen as enabling a dialect of Linux C with
a Context System. Some valid patterns involving complex control-flow are
constrained (such as conditional acquisition and later conditional release
in the same function).

Context analysis is a way to specify permissibility of operations to depend on
context locks being held (or not held). Typically we are interested in
protecting data and code in a critical section by requiring a specific context
to be active, for example by holding a specific lock. The analysis ensures that
callers cannot perform an operation without the required context being active.

Context locks are associated with named structs, along with functions that
operate on `struct instances` to acquire and release the associated context lock.

Context locks can be held either exclusively or shared. This mechanism allows
assigning more precise privileges when a context is active, typically to
distinguish where a thread may only read (shared) or also write (exclusive) to
data guarded within a context.

The set of contexts that are actually active in a given thread at a given point
in program execution is a run-time concept. The static analysis works by
calculating an approximation of that set, called the context environment. The
context environment is calculated for every program point, and describes the
set of contexts that are statically known to be active, or inactive, at that
particular point. This environment is a conservative approximation of the full
set of contexts that will actually be active in a thread at run-time.

More details are also documented [here](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html).

Note

Clang’s analysis explicitly does not infer context locks acquired or
released by inline functions. It requires explicit annotations to (a) assert
that it’s not a bug if a context lock is released or acquired, and (b) to
retain consistency between inline and non-inline function declarations.

### Supported Kernel Primitives

Currently the following synchronization primitives are supported:
raw\_spinlock\_t, spinlock\_t, rwlock\_t, mutex, seqlock\_t,
bit\_spinlock, RCU, SRCU (srcu\_struct), rw\_semaphore, local\_lock\_t,
ww\_mutex.

To initialize variables guarded by a context lock with an initialization
function (`type_init(&lock)`), prefer using `guard(type_init)(&lock)` or
`scoped_guard(type_init, &lock) { ... }` to initialize such guarded members
or globals in the enclosing scope. This initializes the context lock and treats
the context as active within the initialization scope (initialization implies
exclusive access to the underlying object).

For example:

```
struct my_data {
        spinlock_t lock;
        int counter __guarded_by(&lock);
};

void init_my_data(struct my_data *d)
{
        ...
        guard(spinlock_init)(&d->lock);
        d->counter = 0;
        ...
}
```

Alternatively, initializing guarded variables can be done with context analysis
disabled, preferably in the smallest possible scope (due to lack of any other
checking): either with a `context_unsafe(var = init)` expression, or by
marking small initialization functions with the `__context_unsafe(init)`
attribute.

Lockdep assertions, such as `lockdep_assert_held()`, inform the compiler’s
context analysis that the associated synchronization primitive is held after
the assertion. This avoids false positives in complex control-flow scenarios
and encourages the use of Lockdep where static analysis is limited. For
example, this is useful when a function doesn’t *always* require a lock, making
[`__must_hold()`](#c.__must_hold "__must_hold") inappropriate.

### Keywords

\_\_guarded\_by

`__guarded_by (...)`

> `struct member` and globals attribute, declares variable only accessible within active context

**Parameters**

`...`
:   variable arguments

**Description**

Declares that the `struct member` or global variable is only accessible within
the context entered by the given context lock. Read operations on the data
require shared access, while write operations require exclusive access.

```
struct some_state {
        spinlock_t lock;
        long counter __guarded_by(&lock);
};
```

\_\_pt\_guarded\_by

`__pt_guarded_by (...)`

> `struct member` and globals attribute, declares pointed-to data only accessible within active context

**Parameters**

`...`
:   variable arguments

**Description**

Declares that the data pointed to by the `struct member` pointer or global
pointer is only accessible within the context entered by the given context
lock. Read operations on the data require shared access, while write
operations require exclusive access.

```
struct some_state {
        spinlock_t lock;
        long *counter __pt_guarded_by(&lock);
};
```

context\_lock\_struct

`context_lock_struct (name, ...)`

> declare or define a context lock struct

**Parameters**

`name`
:   `struct name`

`...`
:   variable arguments

**Description**

Helper to declare or define a `struct type` that is also a context lock.

```
context_lock_struct(my_handle) {
        int foo;
        long bar;
};

struct some_state {
        ...
};
// ... declared elsewhere ...
context_lock_struct(some_state);
```

**Note**

The implementation defines several helper functions that can acquire
and release the context lock.

disable\_context\_analysis

`disable_context_analysis ()`

> > disables context analysis
>
> **Description**
>
> Disables context analysis. Must be paired with a later
> [`enable_context_analysis()`](#c.enable_context_analysis "enable_context_analysis").

enable\_context\_analysis

`enable_context_analysis ()`

> > re-enables context analysis
>
> **Description**
>
> Re-enables context analysis. Must be paired with a prior
> [`disable_context_analysis()`](#c.disable_context_analysis "disable_context_analysis").

context\_unsafe

`context_unsafe (...)`

> disable context checking for contained code

**Parameters**

`...`
:   variable arguments

**Description**

Disables context checking for contained statements or expression.

```
struct some_data {
        spinlock_t lock;
        int counter __guarded_by(&lock);
};

int foo(struct some_data *d)
{
        // ...
        // other code that is still checked ...
        // ...
        return context_unsafe(d->counter);
}
```

\_\_context\_unsafe

`__context_unsafe (comment)`

> function attribute, disable context checking

**Parameters**

`comment`
:   comment explaining why opt-out is safe

**Description**

Function attribute denoting that context analysis is disabled for the
whole function. Forces adding an inline comment as argument.

token\_context\_lock

`token_context_lock (name, ...)`

> declare an abstract global context lock instance

**Parameters**

`name`
:   token context lock name

`...`
:   variable arguments

**Description**

Helper that declares an abstract global context lock instance **name**, but not
backed by a real data structure (linker error if accidentally referenced).
The type name is \_\_ctx\_lock\_\*\*name\*\*.

token\_context\_lock\_instance

`token_context_lock_instance (ctx, name)`

> declare another instance of a global context lock

**Parameters**

`ctx`
:   token context lock previously declared with [`token_context_lock()`](#c.token_context_lock "token_context_lock")

`name`
:   name of additional global context lock instance

**Description**

Helper that declares an additional instance **name** of the same token context
lock class **ctx**. This is helpful where multiple related token contexts are
declared, to allow using the same underlying type (\_\_ctx\_lock\_\*\*ctx\*\*) as
function arguments.

\_\_must\_hold

`__must_hold (...)`

> function attribute, caller must hold exclusive context lock

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the caller must hold the given context
lock instance(s) exclusively.

\_\_must\_not\_hold

`__must_not_hold (...)`

> function attribute, caller must not hold context lock

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the caller must not hold the given context
lock instance(s).

\_\_acquires

`__acquires (...)`

> function attribute, function acquires context lock exclusively

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the function acquires the given context
lock instance(s) exclusively, but does not release them.

\_\_cond\_acquires

`__cond_acquires (ret, x)`

> function attribute, function conditionally acquires a context lock exclusively

**Parameters**

`ret`
:   abstract value returned by function if context lock acquired

`x`
:   context lock instance pointer

**Description**

Function attribute declaring that the function conditionally acquires the
given context lock instance **x** exclusively, but does not release it. The
function return value **ret** denotes when the context lock is acquired.

**ret** may be one of: true, false, nonzero, 0, nonnull, NULL.

\_\_releases

`__releases (...)`

> function attribute, function releases a context lock exclusively

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the function releases the given context
lock instance(s) exclusively. The associated context(s) must be active on
entry.

\_\_acquire

`__acquire (x)`

> function to acquire context lock exclusively

**Parameters**

`x`
:   context lock instance pointer

**Description**

No-op function that acquires the given context lock instance **x** exclusively.

\_\_release

`__release (x)`

> function to release context lock exclusively

**Parameters**

`x`
:   context lock instance pointer

**Description**

No-op function that releases the given context lock instance **x**.

\_\_must\_hold\_shared

`__must_hold_shared (...)`

> function attribute, caller must hold shared context lock

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the caller must hold the given context
lock instance(s) with shared access.

\_\_acquires\_shared

`__acquires_shared (...)`

> function attribute, function acquires context lock shared

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the function acquires the given
context lock instance(s) with shared access, but does not release them.

\_\_cond\_acquires\_shared

`__cond_acquires_shared (ret, x)`

> function attribute, function conditionally acquires a context lock shared

**Parameters**

`ret`
:   abstract value returned by function if context lock acquired

`x`
:   context lock instance pointer

**Description**

Function attribute declaring that the function conditionally acquires the
given context lock instance **x** with shared access, but does not release it.
The function return value **ret** denotes when the context lock is acquired.

**ret** may be one of: true, false, nonzero, 0, nonnull, NULL.

\_\_releases\_shared

`__releases_shared (...)`

> function attribute, function releases a context lock shared

**Parameters**

`...`
:   variable arguments

**Description**

Function attribute declaring that the function releases the given context
lock instance(s) with shared access. The associated context(s) must be
active on entry.

\_\_acquire\_shared

`__acquire_shared (x)`

> function to acquire context lock shared

**Parameters**

`x`
:   context lock instance pointer

**Description**

No-op function that acquires the given context lock instance **x** with shared
access.

\_\_release\_shared

`__release_shared (x)`

> function to release context lock shared

**Parameters**

`x`
:   context lock instance pointer

**Description**

No-op function that releases the given context lock instance **x** with shared
access.

\_\_acquire\_ret

`__acquire_ret (call, ret_expr)`

> helper to acquire context lock of return value

**Parameters**

`call`
:   call expression

`ret_expr`
:   acquire expression that uses \_\_ret

\_\_acquire\_shared\_ret

`__acquire_shared_ret (call, ret_expr)`

> helper to acquire context lock shared of return value

**Parameters**

`call`
:   call expression

`ret_expr`
:   acquire shared expression that uses \_\_ret

Note

The function attribute \_\_no\_context\_analysis is reserved for internal
implementation of context lock types, and should be avoided in normal code.

## Background

Clang originally called the feature [Thread Safety Analysis](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html), with some keywords
and documentation still using the thread-safety-analysis-only terminology. This
was later changed and the feature became more flexible, gaining the ability to
define custom “capabilities”. Its foundations can be found in [Capability
Systems](https://www.cs.cornell.edu/talc/papers/capabilities.pdf), used to
specify the permissibility of operations to depend on some “capability” being
held (or not held).

Because the feature is not just able to express capabilities related to
synchronization primitives, and “capability” is already overloaded in the
kernel, the naming chosen for the kernel departs from Clang’s initial “Thread
Safety” and “capability” nomenclature; we refer to the feature as “Context
Analysis” to avoid confusion. The internal implementation still makes
references to Clang’s terminology in a few places, such as -Wthread-safety
being the warning option that also still appears in diagnostic messages.
