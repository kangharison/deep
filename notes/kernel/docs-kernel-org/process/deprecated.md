# Deprecated Interfaces, Language Features, Attributes, and Conventions

> 출처(원문): https://docs.kernel.org/process/deprecated.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Deprecated Interfaces, Language Features, Attributes, and Conventions

In a perfect world, it would be possible to convert all instances of
some deprecated API into the new API and entirely remove the old API in
a single development cycle. However, due to the size of the kernel, the
maintainership hierarchy, and timing, it’s not always feasible to do these
kinds of conversions at once. This means that new instances may sneak into
the kernel while old ones are being removed, only making the amount of
work to remove the API grow. In order to educate developers about what
has been deprecated and why, this list has been created as a place to
point when uses of deprecated things are proposed for inclusion in the
kernel.

## \_\_deprecated

While this attribute does visually mark an interface as deprecated,
it [does not produce warnings during builds any more](https://git.kernel.org/linus/771c035372a036f83353eef46dbb829780330234)
because one of the standing goals of the kernel is to build without
warnings and no one was actually doing anything to remove these deprecated
interfaces. While using \_\_deprecated is nice to note an old API in
a header file, it isn’t the full solution. Such interfaces must either
be fully removed from the kernel, or added to this file to discourage
others from using them in the future.

## BUG() and BUG\_ON()

Use `WARN()` and `WARN_ON()` instead, and handle the “impossible”
error condition as gracefully as possible. While the `BUG()`-family
of APIs were originally designed to act as an “impossible situation”
assert and to kill a kernel thread “safely”, they turn out to just be
too risky. (e.g. “In what order do locks need to be released? Have
various states been restored?”) Very commonly, using `BUG()` will
destabilize a system or entirely break it, which makes it impossible
to debug or even get viable crash reports. Linus has [very strong](https://lore.kernel.org/lkml/CA+55aFy6jNLsywVYdGp83AMrXBo_P-pkjkphPGrO=82SPKCpLQ@mail.gmail.com/)
feelings [about this](https://lore.kernel.org/lkml/CAHk-=whDHsbK3HTOpTF=ue_o04onRwTEaK_ZoJp_fjbqq4+=Jw@mail.gmail.com/).

Note that the `WARN()`-family should only be used for “expected to
be unreachable” situations. If you want to warn about “reachable
but undesirable” situations, please use the [`pr_warn()`](../core-api/printk-basics.html#c.pr_warn "pr_warn")-family of
functions. System owners may have set the *panic\_on\_warn* sysctl,
to make sure their systems do not continue running in the face of
“unreachable” conditions. (For example, see commits like [this one](https://git.kernel.org/linus/d4689846881d160a4d12a514e991a740bcb5d65a).)

## open-coded arithmetic in allocator arguments

Dynamic size calculations (especially multiplication) should not be
performed in memory allocator (or similar) function arguments due to the
risk of them overflowing. This could lead to values wrapping around and a
smaller allocation being made than the caller was expecting. Using those
allocations could lead to linear overflows of heap memory and other
misbehaviors. (One exception to this is literal values where the compiler
can warn if they might overflow. However, the preferred way in these
cases is to refactor the code as suggested below to avoid the open-coded
arithmetic.)

For example, do not use `count * size` as an argument, as in:

```
foo = kmalloc(count * size, GFP_KERNEL);
```

Instead, the 2-factor form of the allocator should be used:

```
foo = kmalloc_array(count, size, GFP_KERNEL);
```

Specifically, [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc") can be replaced with [`kmalloc_array()`](../core-api/mm-api.html#c.kmalloc_array "kmalloc_array"), and
[`kzalloc()`](../core-api/mm-api.html#c.kzalloc "kzalloc") can be replaced with [`kcalloc()`](../core-api/mm-api.html#c.kcalloc "kcalloc").

If no 2-factor form is available, the saturate-on-overflow helpers should
be used:

```
bar = dma_alloc_coherent(dev, array_size(count, size), &dma, GFP_KERNEL);
```

Another common case to avoid is calculating the size of a structure with
a trailing array of others structures, as in:

```
header = kzalloc(sizeof(*header) + count * sizeof(*header->item),
                 GFP_KERNEL);
```

Instead, use the helper:

```
header = kzalloc(struct_size(header, item, count), GFP_KERNEL);
```

Note

If you are using [`struct_size()`](../core-api/kernel-api.html#c.struct_size "struct_size") on a structure containing a zero-length
or a one-element array as a trailing array member, please refactor such
array usage and switch to a [flexible array member](#zero-length-and-one-element-arrays) instead.

For other calculations, please compose the use of the [`size_mul()`](../core-api/kernel-api.html#c.size_mul "size_mul"),
[`size_add()`](../core-api/kernel-api.html#c.size_add "size_add"), and [`size_sub()`](../core-api/kernel-api.html#c.size_sub "size_sub") helpers. For example, in the case of:

```
foo = krealloc(current_size + chunk_size * (count - 3), GFP_KERNEL);
```

Instead, use the helpers:

```
foo = krealloc(size_add(current_size,
                        size_mul(chunk_size,
                                 size_sub(count, 3))), GFP_KERNEL);
```

For more details, also see [`array3_size()`](../core-api/kernel-api.html#c.array3_size "array3_size") and [`flex_array_size()`](../core-api/kernel-api.html#c.flex_array_size "flex_array_size"),
as well as the related [`check_mul_overflow()`](../core-api/kernel-api.html#c.check_mul_overflow "check_mul_overflow"), [`check_add_overflow()`](../core-api/kernel-api.html#c.check_add_overflow "check_add_overflow"),
[`check_sub_overflow()`](../core-api/kernel-api.html#c.check_sub_overflow "check_sub_overflow"), and [`check_shl_overflow()`](../core-api/kernel-api.html#c.check_shl_overflow "check_shl_overflow") family of functions.

## simple\_strtol(), simple\_strtoll(), simple\_strtoul(), simple\_strtoull()

The [`simple_strtol()`](../core-api/kernel-api.html#c.simple_strtol "simple_strtol"), [`simple_strtoll()`](../core-api/kernel-api.html#c.simple_strtoll "simple_strtoll"),
[`simple_strtoul()`](../core-api/kernel-api.html#c.simple_strtoul "simple_strtoul"), and [`simple_strtoull()`](../core-api/kernel-api.html#c.simple_strtoull "simple_strtoull") functions
explicitly ignore overflows, which may lead to unexpected results
in callers. The respective [`kstrtol()`](../core-api/kernel-api.html#c.kstrtol "kstrtol"), [`kstrtoll()`](../core-api/kernel-api.html#c.kstrtoll "kstrtoll"),
[`kstrtoul()`](../core-api/kernel-api.html#c.kstrtoul "kstrtoul"), and [`kstrtoull()`](../core-api/kernel-api.html#c.kstrtoull "kstrtoull") functions tend to be the
correct replacements, though note that those require the string to be
NUL or newline terminated.

## strcpy()

[`strcpy()`](../core-api/kernel-api.html#c.strcpy "strcpy") performs no bounds checking on the destination buffer. This
could result in linear overflows beyond the end of the buffer, leading to
all kinds of misbehaviors. While CONFIG\_FORTIFY\_SOURCE=y and various
compiler flags help reduce the risk of using this function, there is
no good reason to add new uses of this function. The safe replacement
is [`strscpy()`](../core-api/kernel-api.html#c.strscpy "strscpy"), though care must be given to any cases where the return
value of [`strcpy()`](../core-api/kernel-api.html#c.strcpy "strcpy") was used, since [`strscpy()`](../core-api/kernel-api.html#c.strscpy "strscpy") does not return a pointer to
the destination, but rather a count of non-NUL bytes copied (or negative
errno when it truncates).

## strncpy() on NUL-terminated strings

Use of [`strncpy()`](../core-api/kernel-api.html#c.strncpy "strncpy") does not guarantee that the destination buffer will
be NUL terminated. This can lead to various linear read overflows and
other misbehavior due to the missing termination. It also NUL-pads
the destination buffer if the source contents are shorter than the
destination buffer size, which may be a needless performance penalty
for callers using only NUL-terminated strings.

When the destination is required to be NUL-terminated, the replacement is
[`strscpy()`](../core-api/kernel-api.html#c.strscpy "strscpy"), though care must be given to any cases where the return value
of [`strncpy()`](../core-api/kernel-api.html#c.strncpy "strncpy") was used, since [`strscpy()`](../core-api/kernel-api.html#c.strscpy "strscpy") does not return a pointer to the
destination, but rather a count of non-NUL bytes copied (or negative
errno when it truncates). Any cases still needing NUL-padding should
instead use [`strscpy_pad()`](../core-api/kernel-api.html#c.strscpy_pad "strscpy_pad").

If a caller is using non-NUL-terminated strings, [`strtomem()`](../core-api/kernel-api.html#c.strtomem "strtomem") should be
used, and the destinations should be marked with the [\_\_nonstring](https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html)
attribute to avoid future compiler warnings. For cases still needing
NUL-padding, [`strtomem_pad()`](../core-api/kernel-api.html#c.strtomem_pad "strtomem_pad") can be used.

## strlcpy()

`strlcpy()` reads the entire source buffer first (since the return value
is meant to match that of [`strlen()`](../core-api/kernel-api.html#c.strlen "strlen")). This read may exceed the destination
size limit. This is both inefficient and can lead to linear read overflows
if a source string is not NUL-terminated. The safe replacement is [`strscpy()`](../core-api/kernel-api.html#c.strscpy "strscpy"),
though care must be given to any cases where the return value of `strlcpy()`
is used, since [`strscpy()`](../core-api/kernel-api.html#c.strscpy "strscpy") will return negative errno values when it truncates.

## %p format specifier

Traditionally, using “%p” in format strings would lead to regular address
exposure flaws in dmesg, proc, sysfs, etc. Instead of leaving these to
be exploitable, all “%p” uses in the kernel are being printed as a hashed
value, rendering them unusable for addressing. New uses of “%p” should not
be added to the kernel. For text addresses, using “%pS” is likely better,
as it produces the more useful symbol name instead. For nearly everything
else, just do not add “%p” at all.

Paraphrasing Linus’s current [guidance](https://lore.kernel.org/lkml/CA+55aFwQEd_d40g4mUCSsVRZzrFPUJt74vc6PPpb675hYNXcKw@mail.gmail.com/):

* If the hashed “%p” value is pointless, ask yourself whether the pointer
  itself is important. Maybe it should be removed entirely?
* If you really think the true pointer value is important, why is some
  system state or user privilege level considered “special”? If you think
  you can justify it (in comments and commit log) well enough to stand
  up to Linus’s scrutiny, maybe you can use “%px”, along with making sure
  you have sensible permissions.

If you are debugging something where “%p” hashing is causing problems,
you can temporarily boot with the debug flag “[no\_hash\_pointers](https://git.kernel.org/linus/5ead723a20e0447bc7db33dc3070b420e5f80aa6)”.

## Variable Length Arrays (VLAs)

Using stack VLAs produces much worse machine code than statically
sized stack arrays. While these non-trivial [performance issues](https://git.kernel.org/linus/02361bc77888) are reason enough to
eliminate VLAs, they are also a security risk. Dynamic growth of a stack
array may exceed the remaining memory in the stack segment. This could
lead to a crash, possible overwriting sensitive contents at the end of the
stack (when built without CONFIG\_THREAD\_INFO\_IN\_TASK=y), or overwriting
memory adjacent to the stack (when built without CONFIG\_VMAP\_STACK=y)

## Implicit switch case fall-through

The C language allows switch cases to fall through to the next case
when a “break” statement is missing at the end of a case. This, however,
introduces ambiguity in the code, as it’s not always clear if the missing
break is intentional or a bug. For example, it’s not obvious just from
looking at the code if STATE\_ONE is intentionally designed to fall
through into STATE\_TWO:

```
switch (value) {
case STATE_ONE:
        do_something();
case STATE_TWO:
        do_other();
        break;
default:
        WARN("unknown state");
}
```

As there have been a long list of flaws [due to missing “break” statements](https://cwe.mitre.org/data/definitions/484.html), we no longer allow
implicit fall-through. In order to identify intentional fall-through
cases, we have adopted a pseudo-keyword macro “fallthrough” which
expands to gcc’s extension [\_\_attribute\_\_((\_\_fallthrough\_\_))](https://gcc.gnu.org/onlinedocs/gcc/Statement-Attributes.html).
(When the C17/C18 [[fallthrough]] syntax is more commonly supported by
C compilers, static analyzers, and IDEs, we can switch to using that syntax
for the macro pseudo-keyword.)

All switch/case blocks must end in one of:

* break;
* fallthrough;
* continue;
* goto <label>;
* return [expression];

## Zero-length and one-element arrays

There is a regular need in the kernel to provide a way to declare having
a dynamically sized set of trailing elements in a structure. Kernel code
should always use [“flexible array members”](https://en.wikipedia.org/wiki/Flexible_array_member)
for these cases. The older style of one-element or zero-length arrays should
no longer be used.

In older C code, dynamically sized trailing elements were done by specifying
a one-element array at the end of a structure:

```
struct something {
        size_t count;
        struct foo items[1];
};
```

This led to fragile size calculations via sizeof() (which would need to
remove the size of the single trailing element to get a correct size of
the “header”). A [GNU C extension](https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html)
was introduced to allow for zero-length arrays, to avoid these kinds of
size problems:

```
struct something {
        size_t count;
        struct foo items[0];
};
```

But this led to other problems, and didn’t solve some problems shared by
both styles, like not being able to detect when such an array is accidentally
being used \_not\_ at the end of a structure (which could happen directly, or
when such a `struct was` in unions, structs of structs, etc).

C99 introduced “flexible array members”, which lacks a numeric size for
the array declaration entirely:

```
struct something {
        size_t count;
        struct foo items[];
};
```

This is the way the kernel expects dynamically sized trailing elements
to be declared. It allows the compiler to generate errors when the
flexible array does not occur last in the structure, which helps to prevent
some kind of [undefined behavior](https://git.kernel.org/linus/76497732932f15e7323dc805e8ea8dc11bb587cf)
bugs from being inadvertently introduced to the codebase. It also allows
the compiler to correctly analyze array sizes (via sizeof(),
CONFIG\_FORTIFY\_SOURCE, and CONFIG\_UBSAN\_BOUNDS). For instance,
there is no mechanism that warns us that the following application of the
sizeof() operator to a zero-length array always results in zero:

```
struct something {
        size_t count;
        struct foo items[0];
};

struct something *instance;

instance = kmalloc(struct_size(instance, items, count), GFP_KERNEL);
instance->count = count;

size = sizeof(instance->items) * instance->count;
memcpy(instance->items, source, size);
```

At the last line of code above, `size` turns out to be `zero`, when one might
have thought it represents the total size in bytes of the dynamic memory recently
allocated for the trailing array `items`. Here are a couple examples of this
issue: [link 1](https://git.kernel.org/linus/f2cd32a443da694ac4e28fbf4ac6f9d5cc63a539),
[link 2](https://git.kernel.org/linus/ab91c2a89f86be2898cee208d492816ec238b2cf).
Instead, [flexible array members have incomplete type, and so the sizeof()
operator may not be applied](https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html),
so any misuse of such operators will be immediately noticed at build time.

With respect to one-element arrays, one has to be acutely aware that [such arrays
occupy at least as much space as a single object of the type](https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html),
hence they contribute to the size of the enclosing structure. This is prone
to error every time people want to calculate the total size of dynamic memory
to allocate for a structure containing an array of this kind as a member:

```
struct something {
        size_t count;
        struct foo items[1];
};

struct something *instance;

instance = kmalloc(struct_size(instance, items, count - 1), GFP_KERNEL);
instance->count = count;

size = sizeof(instance->items) * instance->count;
memcpy(instance->items, source, size);
```

In the example above, we had to remember to calculate `count - 1` when using
the [`struct_size()`](../core-api/kernel-api.html#c.struct_size "struct_size") helper, otherwise we would have --unintentionally-- allocated
memory for one too many `items` objects. The cleanest and least error-prone way
to implement this is through the use of a flexible array member, together with
[`struct_size()`](../core-api/kernel-api.html#c.struct_size "struct_size") and [`flex_array_size()`](../core-api/kernel-api.html#c.flex_array_size "flex_array_size") helpers:

```
struct something {
        size_t count;
        struct foo items[];
};

struct something *instance;

instance = kmalloc(struct_size(instance, items, count), GFP_KERNEL);
instance->count = count;

memcpy(instance->items, source, flex_array_size(instance, items, instance->count));
```

There are two special cases of replacement where the [`DECLARE_FLEX_ARRAY()`](../driver-api/basics.html#c.DECLARE_FLEX_ARRAY "DECLARE_FLEX_ARRAY")
helper needs to be used. (Note that it is named `__DECLARE_FLEX_ARRAY()` for
use in UAPI headers.) Those cases are when the flexible array is either
alone in a `struct or` is part of a union. These are disallowed by the C99
specification, but for no technical reason (as can be seen by both the
existing use of such arrays in those places and the work-around that
[`DECLARE_FLEX_ARRAY()`](../driver-api/basics.html#c.DECLARE_FLEX_ARRAY "DECLARE_FLEX_ARRAY") uses). For example, to convert this:

```
struct something {
        ...
        union {
                struct type1 one[0];
                struct type2 two[0];
        };
};
```

The helper must be used:

```
struct something {
        ...
        union {
                DECLARE_FLEX_ARRAY(struct type1, one);
                DECLARE_FLEX_ARRAY(struct type2, two);
        };
};
```

## Open-coded kmalloc assignments for struct objects

Performing open-coded [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc")-family allocation assignments prevents
the kernel (and compiler) from being able to examine the type of the
variable being assigned, which limits any related introspection that
may help with alignment, wrap-around, or additional hardening. The
[`kmalloc_obj()`](../core-api/mm-api.html#c.kmalloc_obj "kmalloc_obj")-family of macros provide this introspection, which can be
used for the common code patterns for single, array, and flexible object
allocations. For example, these open coded assignments:

```
ptr = kmalloc(sizeof(*ptr), gfp);
ptr = kzalloc(sizeof(*ptr), gfp);
ptr = kmalloc_array(count, sizeof(*ptr), gfp);
ptr = kcalloc(count, sizeof(*ptr), gfp);
ptr = kmalloc(struct_size(ptr, flex_member, count), gfp);
ptr = kmalloc(sizeof(struct foo, gfp);
```

become, respectively:

```
ptr = kmalloc_obj(*ptr, gfp);
ptr = kzalloc_obj(*ptr, gfp);
ptr = kmalloc_objs(*ptr, count, gfp);
ptr = kzalloc_objs(*ptr, count, gfp);
ptr = kmalloc_flex(*ptr, flex_member, count, gfp);
__auto_type ptr = kmalloc_obj(struct foo, gfp);
```

If ptr->flex\_member is annotated with `__counted_by()`, the allocation
will automatically fail if count is larger than the maximum
representable value that can be stored in the counter member associated
with flex\_member.
