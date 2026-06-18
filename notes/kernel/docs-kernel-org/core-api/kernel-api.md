# The Linux Kernel API

> 출처(원문): https://docs.kernel.org/core-api/kernel-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux Kernel API

## Basic C Library Functions

When writing drivers, you cannot in general use routines which are from
the C Library. Some of the functions have been found generally useful
and they are listed below. The behaviour of these functions may vary
slightly from those defined by ANSI, and these deviations are noted in
the text.

### String Conversions

unsigned long long simple\_strtoull(const char \*cp, char \*\*endp, unsigned int base)
:   convert a string to an unsigned long long

**Parameters**

`const char *cp`
:   The start of the string

`char **endp`
:   A pointer to the end of the parsed string will be placed here

`unsigned int base`
:   The number base to use

**Description**

This function has caveats. Please use kstrtoull instead.

unsigned long simple\_strtoul(const char \*cp, char \*\*endp, unsigned int base)
:   convert a string to an unsigned long

**Parameters**

`const char *cp`
:   The start of the string

`char **endp`
:   A pointer to the end of the parsed string will be placed here

`unsigned int base`
:   The number base to use

**Description**

This function has caveats. Please use kstrtoul instead.

long simple\_strtol(const char \*cp, char \*\*endp, unsigned int base)
:   convert a string to a signed long

**Parameters**

`const char *cp`
:   The start of the string

`char **endp`
:   A pointer to the end of the parsed string will be placed here

`unsigned int base`
:   The number base to use

**Description**

This function has caveats. Please use kstrtol instead.

long long simple\_strtoll(const char \*cp, char \*\*endp, unsigned int base)
:   convert a string to a signed long long

**Parameters**

`const char *cp`
:   The start of the string

`char **endp`
:   A pointer to the end of the parsed string will be placed here

`unsigned int base`
:   The number base to use

**Description**

This function has caveats. Please use kstrtoll instead.

int vsnprintf(char \*buf, size\_t size, const char \*fmt\_str, va\_list args)
:   Format a string and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`size_t size`
:   The size of the buffer, including the trailing null space

`const char *fmt_str`
:   The format string to use

`va_list args`
:   Arguments for the format string

**Description**

This function generally follows C99 vsnprintf, but has some
extensions and a few limitations:

> * ``` ``n`` ``` is unsupported
> * ``` ``p*`` ``` is handled by `pointer()`

See `pointer()` or [How to get printk format specifiers right](printk-formats.html) for more
extensive description.

**Please update the documentation in both places when making changes**

The return value is the number of characters which would
be generated for the given input, excluding the trailing
‘0’, as per ISO C99. If you want to have the exact
number of characters written into **buf** as return value
(not including the trailing ‘0’), use [`vscnprintf()`](#c.vscnprintf "vscnprintf"). If the
return is greater than or equal to **size**, the resulting
string is truncated.

If you’re not already dealing with a va\_list consider using [`snprintf()`](#c.snprintf "snprintf").

int vscnprintf(char \*buf, size\_t size, const char \*fmt, va\_list args)
:   Format a string and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`size_t size`
:   The size of the buffer, including the trailing null space

`const char *fmt`
:   The format string to use

`va_list args`
:   Arguments for the format string

**Description**

The return value is the number of characters which have been written into
the **buf** not including the trailing ‘0’. If **size** is == 0 the function
returns 0.

If you’re not already dealing with a va\_list consider using [`scnprintf()`](#c.scnprintf "scnprintf").

See the [`vsnprintf()`](#c.vsnprintf "vsnprintf") documentation for format string extensions over C99.

int snprintf(char \*buf, size\_t size, const char \*fmt, ...)
:   Format a string and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`size_t size`
:   The size of the buffer, including the trailing null space

`const char *fmt`
:   The format string to use

`...`
:   Arguments for the format string

**Description**

The return value is the number of characters which would be
generated for the given input, excluding the trailing null,
as per ISO C99. If the return is greater than or equal to
**size**, the resulting string is truncated.

See the [`vsnprintf()`](#c.vsnprintf "vsnprintf") documentation for format string extensions over C99.

int scnprintf(char \*buf, size\_t size, const char \*fmt, ...)
:   Format a string and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`size_t size`
:   The size of the buffer, including the trailing null space

`const char *fmt`
:   The format string to use

`...`
:   Arguments for the format string

**Description**

The return value is the number of characters written into **buf** not including
the trailing ‘0’. If **size** is == 0 the function returns 0.

int vsprintf(char \*buf, const char \*fmt, va\_list args)
:   Format a string and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`const char *fmt`
:   The format string to use

`va_list args`
:   Arguments for the format string

**Description**

The return value is the number of characters written into **buf** not including
the trailing ‘0’. Use [`vsnprintf()`](#c.vsnprintf "vsnprintf") or [`vscnprintf()`](#c.vscnprintf "vscnprintf") in order to avoid
buffer overflows.

If you’re not already dealing with a va\_list consider using [`sprintf()`](#c.sprintf "sprintf").

See the [`vsnprintf()`](#c.vsnprintf "vsnprintf") documentation for format string extensions over C99.

int sprintf(char \*buf, const char \*fmt, ...)
:   Format a string and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`const char *fmt`
:   The format string to use

`...`
:   Arguments for the format string

**Description**

The return value is the number of characters written into **buf** not including
the trailing ‘0’. Use [`snprintf()`](#c.snprintf "snprintf") or [`scnprintf()`](#c.scnprintf "scnprintf") in order to avoid
buffer overflows.

See the [`vsnprintf()`](#c.vsnprintf "vsnprintf") documentation for format string extensions over C99.

int vbin\_printf(u32 \*bin\_buf, size\_t size, const char \*fmt\_str, va\_list args)
:   Parse a format string and place args’ binary value in a buffer

**Parameters**

`u32 *bin_buf`
:   The buffer to place args’ binary value

`size_t size`
:   The size of the buffer(by words(32bits), not characters)

`const char *fmt_str`
:   The format string to use

`va_list args`
:   Arguments for the format string

**Description**

The format follows C99 vsnprintf, except `n` is ignored, and its argument
is skipped.

The return value is the number of words(32bits) which would be generated for
the given input.

**NOTE**

If the return value is greater than **size**, the resulting bin\_buf is NOT
valid for [`bstr_printf()`](#c.bstr_printf "bstr_printf").

int bstr\_printf(char \*buf, size\_t size, const char \*fmt\_str, const u32 \*bin\_buf)
:   Format a string from binary arguments and place it in a buffer

**Parameters**

`char *buf`
:   The buffer to place the result into

`size_t size`
:   The size of the buffer, including the trailing null space

`const char *fmt_str`
:   The format string to use

`const u32 *bin_buf`
:   Binary arguments for the format string

**Description**

This function like C99 vsnprintf, but the difference is that vsnprintf gets
arguments from stack, and bstr\_printf gets arguments from **bin\_buf** which is
a binary buffer that generated by vbin\_printf.

The format follows C99 vsnprintf, but has some extensions:
:   see vsnprintf comment for details.

The return value is the number of characters which would
be generated for the given input, excluding the trailing
‘0’, as per ISO C99. If you want to have the exact
number of characters written into **buf** as return value
(not including the trailing ‘0’), use [`vscnprintf()`](#c.vscnprintf "vscnprintf"). If the
return is greater than or equal to **size**, the resulting
string is truncated.

int vsscanf(const char \*buf, const char \*fmt, va\_list args)
:   Unformat a buffer into a list of arguments

**Parameters**

`const char *buf`
:   input buffer

`const char *fmt`
:   format of buffer

`va_list args`
:   arguments

int sscanf(const char \*buf, const char \*fmt, ...)
:   Unformat a buffer into a list of arguments

**Parameters**

`const char *buf`
:   input buffer

`const char *fmt`
:   formatting of buffer

`...`
:   resulting arguments

int kstrtoul(const char \*s, unsigned int base, unsigned long \*res)
:   convert a string to an unsigned long

**Parameters**

`const char *s`
:   The start of the string. The string must be null-terminated, and may also
    include a single newline before its terminating null. The first character
    may also be a plus sign, but not a minus sign.

`unsigned int base`
:   The number base to use. The maximum supported base is 16. If base is
    given as 0, then the base of the string is automatically detected with the
    conventional semantics - If it begins with 0x the number will be parsed as a
    hexadecimal (case insensitive), if it otherwise begins with 0, it will be
    parsed as an octal number. Otherwise it will be parsed as a decimal.

`unsigned long *res`
:   Where to write the result of the conversion on success.

**Description**

Returns 0 on success, -ERANGE on overflow and -EINVAL on parsing error.
Preferred over [`simple_strtoul()`](#c.simple_strtoul "simple_strtoul"). Return code must be checked.

int kstrtol(const char \*s, unsigned int base, long \*res)
:   convert a string to a long

**Parameters**

`const char *s`
:   The start of the string. The string must be null-terminated, and may also
    include a single newline before its terminating null. The first character
    may also be a plus sign or a minus sign.

`unsigned int base`
:   The number base to use. The maximum supported base is 16. If base is
    given as 0, then the base of the string is automatically detected with the
    conventional semantics - If it begins with 0x the number will be parsed as a
    hexadecimal (case insensitive), if it otherwise begins with 0, it will be
    parsed as an octal number. Otherwise it will be parsed as a decimal.

`long *res`
:   Where to write the result of the conversion on success.

**Description**

Returns 0 on success, -ERANGE on overflow and -EINVAL on parsing error.
Preferred over [`simple_strtol()`](#c.simple_strtol "simple_strtol"). Return code must be checked.

int kstrtoull(const char \*s, unsigned int base, unsigned long long \*res)
:   convert a string to an unsigned long long

**Parameters**

`const char *s`
:   The start of the string. The string must be null-terminated, and may also
    include a single newline before its terminating null. The first character
    may also be a plus sign, but not a minus sign.

`unsigned int base`
:   The number base to use. The maximum supported base is 16. If base is
    given as 0, then the base of the string is automatically detected with the
    conventional semantics - If it begins with 0x the number will be parsed as a
    hexadecimal (case insensitive), if it otherwise begins with 0, it will be
    parsed as an octal number. Otherwise it will be parsed as a decimal.

`unsigned long long *res`
:   Where to write the result of the conversion on success.

**Description**

Returns 0 on success, -ERANGE on overflow and -EINVAL on parsing error.
Preferred over [`simple_strtoull()`](#c.simple_strtoull "simple_strtoull"). Return code must be checked.

int kstrtoll(const char \*s, unsigned int base, long long \*res)
:   convert a string to a long long

**Parameters**

`const char *s`
:   The start of the string. The string must be null-terminated, and may also
    include a single newline before its terminating null. The first character
    may also be a plus sign or a minus sign.

`unsigned int base`
:   The number base to use. The maximum supported base is 16. If base is
    given as 0, then the base of the string is automatically detected with the
    conventional semantics - If it begins with 0x the number will be parsed as a
    hexadecimal (case insensitive), if it otherwise begins with 0, it will be
    parsed as an octal number. Otherwise it will be parsed as a decimal.

`long long *res`
:   Where to write the result of the conversion on success.

**Description**

Returns 0 on success, -ERANGE on overflow and -EINVAL on parsing error.
Preferred over [`simple_strtoll()`](#c.simple_strtoll "simple_strtoll"). Return code must be checked.

int kstrtouint(const char \*s, unsigned int base, unsigned int \*res)
:   convert a string to an unsigned int

**Parameters**

`const char *s`
:   The start of the string. The string must be null-terminated, and may also
    include a single newline before its terminating null. The first character
    may also be a plus sign, but not a minus sign.

`unsigned int base`
:   The number base to use. The maximum supported base is 16. If base is
    given as 0, then the base of the string is automatically detected with the
    conventional semantics - If it begins with 0x the number will be parsed as a
    hexadecimal (case insensitive), if it otherwise begins with 0, it will be
    parsed as an octal number. Otherwise it will be parsed as a decimal.

`unsigned int *res`
:   Where to write the result of the conversion on success.

**Description**

Returns 0 on success, -ERANGE on overflow and -EINVAL on parsing error.
Preferred over [`simple_strtoul()`](#c.simple_strtoul "simple_strtoul"). Return code must be checked.

int kstrtoint(const char \*s, unsigned int base, int \*res)
:   convert a string to an int

**Parameters**

`const char *s`
:   The start of the string. The string must be null-terminated, and may also
    include a single newline before its terminating null. The first character
    may also be a plus sign or a minus sign.

`unsigned int base`
:   The number base to use. The maximum supported base is 16. If base is
    given as 0, then the base of the string is automatically detected with the
    conventional semantics - If it begins with 0x the number will be parsed as a
    hexadecimal (case insensitive), if it otherwise begins with 0, it will be
    parsed as an octal number. Otherwise it will be parsed as a decimal.

`int *res`
:   Where to write the result of the conversion on success.

**Description**

Returns 0 on success, -ERANGE on overflow and -EINVAL on parsing error.
Preferred over [`simple_strtol()`](#c.simple_strtol "simple_strtol"). Return code must be checked.

int kstrtobool(const char \*s, bool \*res)
:   convert common user inputs into boolean values

**Parameters**

`const char *s`
:   input string

`bool *res`
:   result

**Description**

This routine returns 0 iff the first character is one of ‘EeYyTt1DdNnFf0’,
or [oO][NnFf] for “on” and “off”. Otherwise it will return -EINVAL. Value
pointed to by res is updated upon finding a match.

int string\_get\_size(u64 size, u64 blk\_size, const enum string\_size\_units units, char \*buf, int len)
:   get the size in the specified units

**Parameters**

`u64 size`
:   The size to be converted in blocks

`u64 blk_size`
:   Size of the block (use 1 for size in bytes)

`const enum string_size_units units`
:   Units to use (powers of 1000 or 1024), whether to include space separator

`char *buf`
:   buffer to format to

`int len`
:   length of buffer

**Description**

This function returns a string formatted to 3 significant figures
giving the size in the required units. **buf** should have room for
at least 9 bytes and will always be zero terminated.

Return value: number of characters of output that would have been written
(which may be greater than len, if output was truncated).

int parse\_int\_array\_user(const char \_\_user \*from, size\_t count, int \*\*array)
:   Split string into a sequence of integers

**Parameters**

`const char __user *from`
:   The user space buffer to read from

`size_t count`
:   The maximum number of bytes to read

`int **array`
:   Returned pointer to sequence of integers

**Description**

On success **array** is allocated and initialized with a sequence of
integers extracted from the **from** plus an additional element that
begins the sequence and specifies the integers count.

Caller takes responsibility for freeing **array** when it is no longer
needed.

int string\_unescape(char \*src, char \*dst, size\_t size, unsigned int flags)
:   unquote characters in the given string

**Parameters**

`char *src`
:   source buffer (escaped)

`char *dst`
:   destination buffer (unescaped)

`size_t size`
:   size of the destination buffer (0 to unlimit)

`unsigned int flags`
:   combination of the flags.

**Description**

The function unquotes characters in the given string.

Because the size of the output will be the same as or less than the size of
the input, the transformation may be performed in place.

Caller must provide valid source and destination pointers. Be aware that
destination buffer will always be NULL-terminated. Source string must be
NULL-terminated as well. The supported flags are:

```
UNESCAPE_SPACE:
        '\f' - form feed
        '\n' - new line
        '\r' - carriage return
        '\t' - horizontal tab
        '\v' - vertical tab
UNESCAPE_OCTAL:
        '\NNN' - byte with octal value NNN (1 to 3 digits)
UNESCAPE_HEX:
        '\xHH' - byte with hexadecimal value HH (1 to 2 digits)
UNESCAPE_SPECIAL:
        '\"' - double quote
        '\\' - backslash
        '\a' - alert (BEL)
        '\e' - escape
UNESCAPE_ANY:
        all previous together
```

**Return**

The amount of the characters processed to the destination buffer excluding
trailing ‘0’ is returned.

int string\_escape\_mem(const char \*src, size\_t isz, char \*dst, size\_t osz, unsigned int flags, const char \*only)
:   quote characters in the given memory buffer

**Parameters**

`const char *src`
:   source buffer (unescaped)

`size_t isz`
:   source buffer size

`char *dst`
:   destination buffer (escaped)

`size_t osz`
:   destination buffer size

`unsigned int flags`
:   combination of the flags

`const char *only`
:   NULL-terminated string containing characters used to limit
    the selected escape class. If characters are included in **only**
    that would not normally be escaped by the classes selected
    in **flags**, they will be copied to **dst** unescaped.

**Description**

The process of escaping byte buffer includes several parts. They are applied
in the following sequence.

> 1. The character is not matched to the one from **only** string and thus
>    must go as-is to the output.
> 2. The character is matched to the printable and ASCII classes, if asked,
>    and in case of match it passes through to the output.
> 3. The character is matched to the printable or ASCII class, if asked,
>    and in case of match it passes through to the output.
> 4. The character is checked if it falls into the class given by **flags**.
>    `ESCAPE_OCTAL` and `ESCAPE_HEX` are going last since they cover any
>    character. Note that they actually can’t go together, otherwise
>    `ESCAPE_HEX` will be ignored.

Caller must provide valid source and destination pointers. Be aware that
destination buffer will not be NULL-terminated, thus caller have to append
it if needs. The supported flags are:

```
%ESCAPE_SPACE: (special white space, not space itself)
        '\f' - form feed
        '\n' - new line
        '\r' - carriage return
        '\t' - horizontal tab
        '\v' - vertical tab
%ESCAPE_SPECIAL:
        '\"' - double quote
        '\\' - backslash
        '\a' - alert (BEL)
        '\e' - escape
%ESCAPE_NULL:
        '\0' - null
%ESCAPE_OCTAL:
        '\NNN' - byte with octal value NNN (3 digits)
%ESCAPE_ANY:
        all previous together
%ESCAPE_NP:
        escape only non-printable characters, checked by isprint()
%ESCAPE_ANY_NP:
        all previous together
%ESCAPE_HEX:
        '\xHH' - byte with hexadecimal value HH (2 digits)
%ESCAPE_NA:
        escape only non-ascii characters, checked by isascii()
%ESCAPE_NAP:
        escape only non-printable or non-ascii characters
%ESCAPE_APPEND:
        append characters from @only to be escaped by the given classes
```

`ESCAPE_APPEND` would help to pass additional characters to the escaped, when
one of `ESCAPE_NP`, `ESCAPE_NA`, or `ESCAPE_NAP` is provided.

One notable caveat, the `ESCAPE_NAP`, `ESCAPE_NP` and `ESCAPE_NA` have the
higher priority than the rest of the flags (`ESCAPE_NAP` is the highest).
It doesn’t make much sense to use either of them without `ESCAPE_OCTAL`
or `ESCAPE_HEX`, because they cover most of the other character classes.
`ESCAPE_NAP` can utilize `ESCAPE_SPACE` or `ESCAPE_SPECIAL` in addition to
the above.

**Return**

The total size of the escaped output that would be generated for
the given input and flags. To check whether the output was
truncated, compare the return value to osz. There is room left in
dst for a ‘0’ terminator if and only if ret < osz.

char \*\*kasprintf\_strarray(gfp\_t gfp, const char \*prefix, size\_t n)
:   allocate and fill array of sequential strings

**Parameters**

`gfp_t gfp`
:   flags for the slab allocator

`const char *prefix`
:   prefix to be used

`size_t n`
:   amount of lines to be allocated and filled

**Description**

Allocates and fills **n** strings using pattern “````` s-````zu `````”, where prefix
is provided by caller. The caller is responsible to free them with
[`kfree_strarray()`](#c.kfree_strarray "kfree_strarray") after use.

Returns array of strings or NULL when memory can’t be allocated.

void kfree\_strarray(char \*\*array, size\_t n)
:   free a number of dynamically allocated strings contained in an array and the array itself

**Parameters**

`char **array`
:   Dynamically allocated array of strings to free.

`size_t n`
:   Number of strings (starting from the beginning of the array) to free.

**Description**

Passing a non-NULL **array** and **n** == 0 as well as NULL **array** are valid
use-cases. If **array** is NULL, the function does nothing.

char \*skip\_spaces(const char \*str)
:   Removes leading whitespace from **str**.

**Parameters**

`const char *str`
:   The string to be stripped.

**Description**

Returns a pointer to the first non-whitespace character in **str**.

char \*strim(char \*s)
:   Removes leading and trailing whitespace from **s**.

**Parameters**

`char *s`
:   The string to be stripped.

**Description**

Note that the first trailing whitespace is replaced with a `NUL-terminator`
in the given string **s**. Returns a pointer to the first non-whitespace
character in **s**.

bool sysfs\_streq(const char \*s1, const char \*s2)
:   return true if strings are equal, modulo trailing newline

**Parameters**

`const char *s1`
:   one string

`const char *s2`
:   another string

**Description**

This routine returns true iff two strings are equal, treating both
NUL and newline-then-NUL as equivalent string terminations. It’s
geared for use with sysfs input strings, which generally terminate
with newlines but are compared against values without newlines.

int match\_string(const char \*const \*array, size\_t n, const char \*string)
:   matches given string in an array

**Parameters**

`const char * const *array`
:   array of strings

`size_t n`
:   number of strings in the array or -1 for NULL terminated arrays

`const char *string`
:   string to match with

**Description**

This routine will look for a string in an array of strings up to the
n-th element in the array or until the first NULL element.

Historically the value of -1 for **n**, was used to search in arrays that
are NULL terminated. However, the function does not make a distinction
when finishing the search: either **n** elements have been compared OR
the first NULL element was found.

**Return**

index of a **string** in the **array** if matches, or `-EINVAL` otherwise.

int \_\_sysfs\_match\_string(const char \*const \*array, size\_t n, const char \*str)
:   matches given string in an array

**Parameters**

`const char * const *array`
:   array of strings

`size_t n`
:   number of strings in the array or -1 for NULL terminated arrays

`const char *str`
:   string to match with

**Description**

Returns index of **str** in the **array** or -EINVAL, just like [`match_string()`](#c.match_string "match_string").
Uses sysfs\_streq instead of strcmp for matching.

This routine will look for a string in an array of strings up to the
n-th element in the array or until the first NULL element.

Historically the value of -1 for **n**, was used to search in arrays that
are NULL terminated. However, the function does not make a distinction
when finishing the search: either **n** elements have been compared OR
the first NULL element was found.

char \*strreplace(char \*str, char old, char new)
:   Replace all occurrences of character in string.

**Parameters**

`char *str`
:   The string to operate on.

`char old`
:   The character being replaced.

`char new`
:   The character **old** is replaced with.

**Description**

Replaces the each **old** character with a **new** one in the given string **str**.

**Return**

pointer to the string **str** itself.

void memcpy\_and\_pad(void \*dest, size\_t dest\_len, const void \*src, size\_t count, int pad)
:   Copy one buffer to another with padding

**Parameters**

`void *dest`
:   Where to copy to

`size_t dest_len`
:   The destination buffer size

`const void *src`
:   Where to copy from

`size_t count`
:   The number of bytes to copy

`int pad`
:   Character to use for padding if space is left in destination.

### String Manipulation

unsafe\_memcpy

`unsafe_memcpy (dst, src, bytes, justification)`

> memcpy implementation with no FORTIFY bounds checking

**Parameters**

`dst`
:   Destination memory address to write to

`src`
:   Source memory address to read from

`bytes`
:   How many bytes to write to **dst** from **src**

`justification`
:   Free-form text or comment describing why the use is needed

**Description**

This should be used for corner cases where the compiler cannot do the
right thing, or during transitions between APIs, etc. It should be used
very rarely, and includes a place for justification detailing where bounds
checking has happened, and why existing solutions cannot be employed.

char \*strncpy(char \*const p, const char \*q, \_\_kernel\_size\_t size)
:   Copy a string to memory with non-guaranteed NUL padding

**Parameters**

`char * const p`
:   pointer to destination of copy

`const char *q`
:   pointer to NUL-terminated source string to copy

`__kernel_size_t size`
:   bytes to write at **p**

**Description**

If strlen(**q**) >= **size**, the copy of **q** will stop after **size** bytes,
and **p** will NOT be NUL-terminated

If strlen(**q**) < **size**, following the copy of **q**, trailing NUL bytes
will be written to **p** until **size** total bytes have been written.

Do not use this function. While FORTIFY\_SOURCE tries to avoid
over-reads of **q**, it cannot defend against writing unterminated
results to **p**. Using [`strncpy()`](#c.strncpy "strncpy") remains ambiguous and fragile.
Instead, please choose an alternative, so that the expectation
of **p**’s contents is unambiguous:

| **p** needs to be: | padded to **size** | not padded |
| --- | --- | --- |
| NUL-terminated | [`strscpy_pad()`](#c.strscpy_pad "strscpy_pad") | [`strscpy()`](#c.strscpy "strscpy") |
| not NUL-terminated | [`strtomem_pad()`](#c.strtomem_pad "strtomem_pad") | [`strtomem()`](#c.strtomem "strtomem") |

Note strscpy\*()’s differing return values for detecting truncation,
and strtomem\*()’s expectation that the destination is marked with
\_\_nonstring when it is a character array.

\_\_kernel\_size\_t strnlen(const char \*const p, \_\_kernel\_size\_t maxlen)
:   Return bounded count of characters in a NUL-terminated string

**Parameters**

`const char * const p`
:   pointer to NUL-terminated string to count.

`__kernel_size_t maxlen`
:   maximum number of characters to count.

**Description**

Returns number of characters in **p** (NOT including the final NUL), or
**maxlen**, if no NUL has been found up to there.

strlen

`strlen (p)`

> Return count of characters in a NUL-terminated string

**Parameters**

`p`
:   pointer to NUL-terminated string to count.

**Description**

Do not use this function unless the string length is known at
compile-time. When **p** is unterminated, this function may crash
or return unexpected counts that could lead to memory content
exposures. Prefer [`strnlen()`](#c.strnlen "strnlen").

Returns number of characters in **p** (NOT including the final NUL).

size\_t strlcat(char \*const p, const char \*const q, size\_t avail)
:   Append a string to an existing string

**Parameters**

`char * const p`
:   pointer to `NUL-terminated` string to append to

`const char * const q`
:   pointer to `NUL-terminated` string to append from

`size_t avail`
:   Maximum bytes available in **p**

**Description**

Appends `NUL-terminated` string **q** after the `NUL-terminated`
string at **p**, but will not write beyond **avail** bytes total,
potentially truncating the copy from **q**. **p** will stay
`NUL-terminated` only if a `NUL` already existed within
the **avail** bytes of **p**. If so, the resulting number of
bytes copied from **q** will be at most “**avail** - strlen(**p**) - 1”.

Do not use this function. While FORTIFY\_SOURCE tries to avoid
read and write overflows, this is only possible when the sizes
of **p** and **q** are known to the compiler. Prefer building the
string with formatting, via [`scnprintf()`](#c.scnprintf "scnprintf"), seq\_buf, or similar.

Returns total bytes that \_would\_ have been contained by **p**
regardless of truncation, similar to [`snprintf()`](#c.snprintf "snprintf"). If return
value is >= **avail**, the string has been truncated.

char \*strcat(char \*const p, const char \*q)
:   Append a string to an existing string

**Parameters**

`char * const p`
:   pointer to NUL-terminated string to append to

`const char *q`
:   pointer to NUL-terminated source string to append from

**Description**

Do not use this function. While FORTIFY\_SOURCE tries to avoid
read and write overflows, this is only possible when the
destination buffer size is known to the compiler. Prefer
building the string with formatting, via [`scnprintf()`](#c.scnprintf "scnprintf") or similar.
At the very least, use [`strncat()`](#c.strncat "strncat").

Returns **p**.

char \*strncat(char \*const p, const char \*const q, \_\_kernel\_size\_t count)
:   Append a string to an existing string

**Parameters**

`char * const p`
:   pointer to NUL-terminated string to append to

`const char * const q`
:   pointer to source string to append from

`__kernel_size_t count`
:   Maximum bytes to read from **q**

**Description**

Appends at most **count** bytes from **q** (stopping at the first
NUL byte) after the NUL-terminated string at **p**. **p** will be
NUL-terminated.

Do not use this function. While FORTIFY\_SOURCE tries to avoid
read and write overflows, this is only possible when the sizes
of **p** and **q** are known to the compiler. Prefer building the
string with formatting, via [`scnprintf()`](#c.scnprintf "scnprintf") or similar.

Returns **p**.

char \*strcpy(char \*const p, const char \*const q)
:   Copy a string into another string buffer

**Parameters**

`char * const p`
:   pointer to destination of copy

`const char * const q`
:   pointer to NUL-terminated source string to copy

**Description**

Do not use this function. While FORTIFY\_SOURCE tries to avoid
overflows, this is only possible when the sizes of **q** and **p** are
known to the compiler. Prefer [`strscpy()`](#c.strscpy "strscpy"), though note its different
return values for detecting truncation.

Returns **p**.

int strncasecmp(const char \*s1, const char \*s2, size\_t len)
:   Case insensitive, length-limited string comparison

**Parameters**

`const char *s1`
:   One string

`const char *s2`
:   The other string

`size_t len`
:   the maximum number of characters to compare

char \*stpcpy(char \*\_\_restrict\_\_ dest, const char \*\_\_restrict\_\_ src)
:   copy a string from src to dest returning a pointer to the new end of dest, including src’s `NUL-terminator`. May overrun dest.

**Parameters**

`char *__restrict__ dest`
:   pointer to end of string being copied into. Must be large enough
    to receive copy.

`const char *__restrict__ src`
:   pointer to the beginning of string being copied from. Must not overlap
    dest.

**Description**

stpcpy differs from strcpy in a key way: the return value is a pointer
to the new `NUL-terminating` character in **dest**. (For strcpy, the return
value is a pointer to the start of **dest**). This interface is considered
unsafe as it doesn’t perform bounds checking of the inputs. As such it’s
not recommended for usage. Instead, its definition is provided in case
the compiler lowers other libcalls to stpcpy.

int strcmp(const char \*cs, const char \*ct)
:   Compare two strings

**Parameters**

`const char *cs`
:   One string

`const char *ct`
:   Another string

int strncmp(const char \*cs, const char \*ct, size\_t count)
:   Compare two length-limited strings

**Parameters**

`const char *cs`
:   One string

`const char *ct`
:   Another string

`size_t count`
:   The maximum number of bytes to compare

char \*strchr(const char \*s, int c)
:   Find the first occurrence of a character in a string

**Parameters**

`const char *s`
:   The string to be searched

`int c`
:   The character to search for

**Description**

Note that the `NUL-terminator` is considered part of the string, and can
be searched for.

char \*strchrnul(const char \*s, int c)
:   Find and return a character in a string, or end of string

**Parameters**

`const char *s`
:   The string to be searched

`int c`
:   The character to search for

**Description**

Returns pointer to first occurrence of ‘c’ in s. If c is not found, then
return a pointer to the null byte at the end of s.

char \*strrchr(const char \*s, int c)
:   Find the last occurrence of a character in a string

**Parameters**

`const char *s`
:   The string to be searched

`int c`
:   The character to search for

char \*strnchr(const char \*s, size\_t count, int c)
:   Find a character in a length limited string

**Parameters**

`const char *s`
:   The string to be searched

`size_t count`
:   The number of characters to be searched

`int c`
:   The character to search for

**Description**

Note that the `NUL-terminator` is considered part of the string, and can
be searched for.

size\_t strspn(const char \*s, const char \*accept)
:   Calculate the length of the initial substring of **s** which only contain letters in **accept**

**Parameters**

`const char *s`
:   The string to be searched

`const char *accept`
:   The string to search for

size\_t strcspn(const char \*s, const char \*reject)
:   Calculate the length of the initial substring of **s** which does not contain letters in **reject**

**Parameters**

`const char *s`
:   The string to be searched

`const char *reject`
:   The string to avoid

char \*strpbrk(const char \*cs, const char \*ct)
:   Find the first occurrence of a set of characters

**Parameters**

`const char *cs`
:   The string to be searched

`const char *ct`
:   The characters to search for

char \*strsep(char \*\*s, const char \*ct)
:   Split a string into tokens

**Parameters**

`char **s`
:   The string to be searched

`const char *ct`
:   The characters to search for

**Description**

[`strsep()`](#c.strsep "strsep") updates **s** to point after the token, ready for the next call.

It returns empty tokens, too, behaving exactly like the libc function
of that name. In fact, it was stolen from glibc2 and de-fancy-fied.
Same semantics, slimmer shape. ;)

void \*memset(void \*s, int c, size\_t count)
:   Fill a region of memory with the given value

**Parameters**

`void *s`
:   Pointer to the start of the area.

`int c`
:   The byte to fill the area with

`size_t count`
:   The size of the area.

**Description**

Do not use [`memset()`](#c.memset "memset") to access IO space, use `memset_io()` instead.

void \*memset16(uint16\_t \*s, uint16\_t v, size\_t count)
:   Fill a memory area with a uint16\_t

**Parameters**

`uint16_t *s`
:   Pointer to the start of the area.

`uint16_t v`
:   The value to fill the area with

`size_t count`
:   The number of values to store

**Description**

Differs from [`memset()`](#c.memset "memset") in that it fills with a uint16\_t instead
of a byte. Remember that **count** is the number of uint16\_ts to
store, not the number of bytes.

void \*memset32(uint32\_t \*s, uint32\_t v, size\_t count)
:   Fill a memory area with a uint32\_t

**Parameters**

`uint32_t *s`
:   Pointer to the start of the area.

`uint32_t v`
:   The value to fill the area with

`size_t count`
:   The number of values to store

**Description**

Differs from [`memset()`](#c.memset "memset") in that it fills with a uint32\_t instead
of a byte. Remember that **count** is the number of uint32\_ts to
store, not the number of bytes.

void \*memset64(uint64\_t \*s, uint64\_t v, size\_t count)
:   Fill a memory area with a uint64\_t

**Parameters**

`uint64_t *s`
:   Pointer to the start of the area.

`uint64_t v`
:   The value to fill the area with

`size_t count`
:   The number of values to store

**Description**

Differs from [`memset()`](#c.memset "memset") in that it fills with a uint64\_t instead
of a byte. Remember that **count** is the number of uint64\_ts to
store, not the number of bytes.

void \*memcpy(void \*dest, const void \*src, size\_t count)
:   Copy one area of memory to another

**Parameters**

`void *dest`
:   Where to copy to

`const void *src`
:   Where to copy from

`size_t count`
:   The size of the area.

**Description**

You should not use this function to access IO space, use `memcpy_toio()`
or `memcpy_fromio()` instead.

void \*memmove(void \*dest, const void \*src, size\_t count)
:   Copy one area of memory to another

**Parameters**

`void *dest`
:   Where to copy to

`const void *src`
:   Where to copy from

`size_t count`
:   The size of the area.

**Description**

Unlike [`memcpy()`](#c.memcpy "memcpy"), [`memmove()`](#c.memmove "memmove") copes with overlapping areas.

\_\_visible int memcmp(const void \*cs, const void \*ct, size\_t count)
:   Compare two areas of memory

**Parameters**

`const void *cs`
:   One area of memory

`const void *ct`
:   Another area of memory

`size_t count`
:   The size of the area.

int bcmp(const void \*a, const void \*b, size\_t len)
:   returns 0 if and only if the buffers have identical contents.

**Parameters**

`const void *a`
:   pointer to first buffer.

`const void *b`
:   pointer to second buffer.

`size_t len`
:   size of buffers.

**Description**

The sign or magnitude of a non-zero return value has no particular
meaning, and architectures may implement their own more efficient [`bcmp()`](#c.bcmp "bcmp"). So
while this particular implementation is a simple (tail) call to memcmp, do
not rely on anything but whether the return value is zero or non-zero.

void \*memscan(void \*addr, int c, size\_t size)
:   Find a character in an area of memory.

**Parameters**

`void *addr`
:   The memory area

`int c`
:   The byte to search for

`size_t size`
:   The size of the area.

**Description**

returns the address of the first occurrence of **c**, or 1 byte past
the area if **c** is not found

char \*strstr(const char \*s1, const char \*s2)
:   Find the first substring in a `NUL` terminated string

**Parameters**

`const char *s1`
:   The string to be searched

`const char *s2`
:   The string to search for

char \*strnstr(const char \*s1, const char \*s2, size\_t len)
:   Find the first substring in a length-limited string

**Parameters**

`const char *s1`
:   The string to be searched

`const char *s2`
:   The string to search for

`size_t len`
:   the maximum number of characters to search

void \*memchr(const void \*s, int c, size\_t n)
:   Find a character in an area of memory.

**Parameters**

`const void *s`
:   The memory area

`int c`
:   The byte to search for

`size_t n`
:   The size of the area.

**Description**

returns the address of the first occurrence of **c**, or `NULL`
if **c** is not found

void \*memchr\_inv(const void \*start, int c, size\_t bytes)
:   Find an unmatching character in an area of memory.

**Parameters**

`const void *start`
:   The memory area

`int c`
:   Find a character other than c

`size_t bytes`
:   The size of the area.

**Description**

returns the address of the first character other than **c**, or `NULL`
if the whole buffer contains just **c**.

void \*memdup\_array\_user(const void \_\_user \*src, size\_t n, size\_t size)
:   duplicate array from user space

**Parameters**

`const void __user *src`
:   source address in user space

`size_t n`
:   number of array members to copy

`size_t size`
:   size of one array member

**Return**

an [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR") on failure. Result is physically
contiguous, to be freed by [`kfree()`](mm-api.html#c.kfree "kfree").

void \*vmemdup\_array\_user(const void \_\_user \*src, size\_t n, size\_t size)
:   duplicate array from user space

**Parameters**

`const void __user *src`
:   source address in user space

`size_t n`
:   number of array members to copy

`size_t size`
:   size of one array member

**Return**

an [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR") on failure. Result may be not
physically contiguous. Use [`kvfree()`](mm-api.html#c.kvfree "kvfree") to free.

strscpy

`strscpy (dst, src, ...)`

> Copy a C-string into a sized buffer

**Parameters**

`dst`
:   Where to copy the string to

`src`
:   Where to copy the string from

`...`
:   Size of destination buffer (optional)

**Description**

Copy the source string **src**, or as much of it as fits, into the
destination **dst** buffer. The behavior is undefined if the string
buffers overlap. The destination **dst** buffer is always NUL terminated,
unless it’s zero-sized.

The size argument **...** is only required when **dst** is not an array, or
when the copy needs to be smaller than sizeof(**dst**).

Preferred to [`strncpy()`](#c.strncpy "strncpy") since it always returns a valid string, and
doesn’t unnecessarily force the tail of the destination buffer to be
zero padded. If padding is desired please use [`strscpy_pad()`](#c.strscpy_pad "strscpy_pad").

Returns the number of characters copied in **dst** (not including the
trailing `NUL`) or -E2BIG if **size** is 0 or the copy from **src** was
truncated.

strscpy\_pad

`strscpy_pad (dst, src, ...)`

> Copy a C-string into a sized buffer

**Parameters**

`dst`
:   Where to copy the string to

`src`
:   Where to copy the string from

`...`
:   Size of destination buffer

**Description**

Copy the string, or as much of it as fits, into the dest buffer. The
behavior is undefined if the string buffers overlap. The destination
buffer is always `NUL` terminated, unless it’s zero-sized.

If the source string is shorter than the destination buffer, the
remaining bytes in the buffer will be filled with `NUL` bytes.

For full explanation of why you may want to consider using the
‘strscpy’ functions please see the function docstring for [`strscpy()`](#c.strscpy "strscpy").

**Return**

* The number of characters copied (not including the trailing `NULs`)
* -E2BIG if count is 0 or **src** was truncated.

bool mem\_is\_zero(const void \*s, size\_t n)
:   Check if an area of memory is all 0’s.

**Parameters**

`const void *s`
:   The memory area

`size_t n`
:   The size of the area

**Return**

True if the area of memory is all 0’s.

sysfs\_match\_string

`sysfs_match_string (_a, _s)`

> matches given string in an array

**Parameters**

`_a`
:   array of strings

`_s`
:   string to match with

**Description**

Helper for [`__sysfs_match_string()`](#c.__sysfs_match_string "__sysfs_match_string"). Calculates the size of **a** automatically.

void memzero\_explicit(void \*s, size\_t count)
:   Fill a region of memory (e.g. sensitive keying data) with 0s.

**Parameters**

`void *s`
:   Pointer to the start of the area.

`size_t count`
:   The size of the area.

**Note**

usually using [`memset()`](#c.memset "memset") is just fine (!), but in cases
where clearing out \_local\_ data at the end of a scope is
necessary, [`memzero_explicit()`](#c.memzero_explicit "memzero_explicit") should be used instead in
order to prevent the compiler from optimising away zeroing.

[`memzero_explicit()`](#c.memzero_explicit "memzero_explicit") doesn’t need an arch-specific version as
it just invokes the one of [`memset()`](#c.memset "memset") implicitly.

const char \*kbasename(const char \*path)
:   return the last part of a pathname.

**Parameters**

`const char *path`
:   path to extract the filename from.

**Return**

Pointer to the filename portion inside **path**. If no ‘/’ exists,
returns **path** unchanged.

strtomem\_pad

`strtomem_pad (dest, src, pad)`

> Copy NUL-terminated string to non-NUL-terminated buffer

**Parameters**

`dest`
:   Pointer of destination character array (marked as \_\_nonstring)

`src`
:   Pointer to NUL-terminated string

`pad`
:   Padding character to fill any remaining bytes of **dest** after copy

**Description**

This is a replacement for [`strncpy()`](#c.strncpy "strncpy") uses where the destination is not
a NUL-terminated string, but with bounds checking on the source size, and
an explicit padding character. If padding is not required, use [`strtomem()`](#c.strtomem "strtomem").

Note that the size of **dest** is not an argument, as the length of **dest**
must be discoverable by the compiler.

strtomem

`strtomem (dest, src)`

> Copy NUL-terminated string to non-NUL-terminated buffer

**Parameters**

`dest`
:   Pointer of destination character array (marked as \_\_nonstring)

`src`
:   Pointer to NUL-terminated string

**Description**

This is a replacement for [`strncpy()`](#c.strncpy "strncpy") uses where the destination is not
a NUL-terminated string, but with bounds checking on the source size, and
without trailing padding. If padding is required, use [`strtomem_pad()`](#c.strtomem_pad "strtomem_pad").

Note that the size of **dest** is not an argument, as the length of **dest**
must be discoverable by the compiler.

memtostr

`memtostr (dest, src)`

> Copy a possibly non-NUL-term string to a NUL-term string

**Parameters**

`dest`
:   Pointer to destination NUL-terminates string

`src`
:   Pointer to character array (likely marked as \_\_nonstring)

**Description**

This is a replacement for [`strncpy()`](#c.strncpy "strncpy") uses where the source is not
a NUL-terminated string.

Note that sizes of **dest** and **src** must be known at compile-time.

memtostr\_pad

`memtostr_pad (dest, src)`

> Copy a possibly non-NUL-term string to a NUL-term string with NUL padding in the destination

**Parameters**

`dest`
:   Pointer to destination NUL-terminates string

`src`
:   Pointer to character array (likely marked as \_\_nonstring)

**Description**

This is a replacement for [`strncpy()`](#c.strncpy "strncpy") uses where the source is not
a NUL-terminated string.

Note that sizes of **dest** and **src** must be known at compile-time.

memset\_after

`memset_after (obj, v, member)`

> Set a value after a `struct member` to the end of a struct

**Parameters**

`obj`
:   Address of target `struct instance`

`v`
:   Byte value to repeatedly write

`member`
:   after which `struct member` to start writing bytes

**Description**

This is good for clearing padding following the given member.

memset\_startat

`memset_startat (obj, v, member)`

> Set a value starting at a member to the end of a struct

**Parameters**

`obj`
:   Address of target `struct instance`

`v`
:   Byte value to repeatedly write

`member`
:   `struct member` to start writing at

**Description**

Note that if there is padding between the prior member and the target
member, [`memset_after()`](#c.memset_after "memset_after") should be used to clear the prior padding.

size\_t str\_has\_prefix(const char \*str, const char \*prefix)
:   Test if a string has a given prefix

**Parameters**

`const char *str`
:   The string to test

`const char *prefix`
:   The string to see if **str** starts with

**Description**

A common way to test a prefix of a string is to do:
:   strncmp(str, prefix, sizeof(prefix) - 1)

But this can lead to bugs due to typos, or if prefix is a pointer
and not a constant. Instead use [`str_has_prefix()`](#c.str_has_prefix "str_has_prefix").

**Return**

* strlen(**prefix**) if **str** starts with **prefix**
* 0 if **str** does not start with **prefix**

bool strstarts(const char \*str, const char \*prefix)
:   does **str** start with **prefix**?

**Parameters**

`const char *str`
:   string to examine

`const char *prefix`
:   prefix to look for.

**Return**

True if **str** begins with **prefix**. False in all other cases.

bool strends(const char \*str, const char \*suffix)
:   Check if a string ends with another string.

**Parameters**

`const char *str`
:   NULL-terminated string to check against **suffix**

`const char *suffix`
:   NULL-terminated string defining the suffix to look for in **str**

**Return**

True if **str** ends with **suffix**. False in all other cases.

char \*kstrdup(const char \*s, gfp\_t gfp)
:   allocate space for and copy an existing string

**Parameters**

`const char *s`
:   the string to duplicate

`gfp_t gfp`
:   the GFP mask used in the [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc") call when allocating memory

**Return**

newly allocated copy of **s** or `NULL` in case of error

const char \*kstrdup\_const(const char \*s, gfp\_t gfp)
:   conditionally duplicate an existing const string

**Parameters**

`const char *s`
:   the string to duplicate

`gfp_t gfp`
:   the GFP mask used in the [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc") call when allocating memory

**Note**

Strings allocated by kstrdup\_const should be freed by kfree\_const and
must not be passed to `krealloc()`.

**Return**

source string if it is in .rodata section otherwise
fallback to kstrdup.

char \*kstrndup(const char \*s, size\_t max, gfp\_t gfp)
:   allocate space for and copy an existing string

**Parameters**

`const char *s`
:   the string to duplicate

`size_t max`
:   read at most **max** chars from **s**

`gfp_t gfp`
:   the GFP mask used in the [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc") call when allocating memory

**Note**

Use [`kmemdup_nul()`](#c.kmemdup_nul "kmemdup_nul") instead if the size is known exactly.

**Return**

newly allocated copy of **s** or `NULL` in case of error

void \*kmemdup(const void \*src, size\_t len, gfp\_t gfp)
:   duplicate region of memory

**Parameters**

`const void *src`
:   memory region to duplicate

`size_t len`
:   memory region length

`gfp_t gfp`
:   GFP mask to use

**Return**

newly allocated copy of **src** or `NULL` in case of error,
result is physically contiguous. Use [`kfree()`](mm-api.html#c.kfree "kfree") to free.

char \*kmemdup\_nul(const char \*s, size\_t len, gfp\_t gfp)
:   Create a NUL-terminated string from unterminated data

**Parameters**

`const char *s`
:   The data to stringify

`size_t len`
:   The size of the data

`gfp_t gfp`
:   the GFP mask used in the [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc") call when allocating memory

**Return**

newly allocated copy of **s** with NUL-termination or `NULL` in
case of error

void \*memdup\_user(const void \_\_user \*src, size\_t len)
:   duplicate memory region from user space

**Parameters**

`const void __user *src`
:   source address in user space

`size_t len`
:   number of bytes to copy

**Return**

an [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR") on failure. Result is physically
contiguous, to be freed by [`kfree()`](mm-api.html#c.kfree "kfree").

void \*vmemdup\_user(const void \_\_user \*src, size\_t len)
:   duplicate memory region from user space

**Parameters**

`const void __user *src`
:   source address in user space

`size_t len`
:   number of bytes to copy

**Return**

an [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR") on failure. Result may be not
physically contiguous. Use [`kvfree()`](mm-api.html#c.kvfree "kvfree") to free.

char \*strndup\_user(const char \_\_user \*s, long n)
:   duplicate an existing string from user space

**Parameters**

`const char __user *s`
:   The string to duplicate

`long n`
:   Maximum number of bytes to copy, including the trailing NUL.

**Return**

newly allocated copy of **s** or an [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR") in case of error

void \*memdup\_user\_nul(const void \_\_user \*src, size\_t len)
:   duplicate memory region from user space and NUL-terminate

**Parameters**

`const void __user *src`
:   source address in user space

`size_t len`
:   number of bytes to copy

**Return**

an [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR") on failure.

## Basic Kernel Library Functions

The Linux kernel provides more basic utility functions.

### Bit Operations

void set\_bit(long nr, volatile unsigned long \*addr)
:   Atomically set a bit in memory

**Parameters**

`long nr`
:   the bit to set

`volatile unsigned long *addr`
:   the address to start counting from

**Description**

This is a relaxed atomic operation (no implied memory barriers).

Note that **nr** may be almost arbitrarily large; this function is not
restricted to acting on a single-word quantity.

void clear\_bit(long nr, volatile unsigned long \*addr)
:   Clears a bit in memory

**Parameters**

`long nr`
:   Bit to clear

`volatile unsigned long *addr`
:   Address to start counting from

**Description**

This is a relaxed atomic operation (no implied memory barriers).

void change\_bit(long nr, volatile unsigned long \*addr)
:   Toggle a bit in memory

**Parameters**

`long nr`
:   Bit to change

`volatile unsigned long *addr`
:   Address to start counting from

**Description**

This is a relaxed atomic operation (no implied memory barriers).

Note that **nr** may be almost arbitrarily large; this function is not
restricted to acting on a single-word quantity.

bool test\_and\_set\_bit(long nr, volatile unsigned long \*addr)
:   Set a bit and return its old value

**Parameters**

`long nr`
:   Bit to set

`volatile unsigned long *addr`
:   Address to count from

**Description**

This is an atomic fully-ordered operation (implied full memory barrier).

bool test\_and\_clear\_bit(long nr, volatile unsigned long \*addr)
:   Clear a bit and return its old value

**Parameters**

`long nr`
:   Bit to clear

`volatile unsigned long *addr`
:   Address to count from

**Description**

This is an atomic fully-ordered operation (implied full memory barrier).

bool test\_and\_change\_bit(long nr, volatile unsigned long \*addr)
:   Change a bit and return its old value

**Parameters**

`long nr`
:   Bit to change

`volatile unsigned long *addr`
:   Address to count from

**Description**

This is an atomic fully-ordered operation (implied full memory barrier).

void \_\_\_set\_bit(unsigned long nr, volatile unsigned long \*addr)
:   Set a bit in memory

**Parameters**

`unsigned long nr`
:   the bit to set

`volatile unsigned long *addr`
:   the address to start counting from

**Description**

Unlike [`set_bit()`](#c.set_bit "set_bit"), this function is non-atomic. If it is called on the same
region of memory concurrently, the effect may be that only one operation
succeeds.

void \_\_\_clear\_bit(unsigned long nr, volatile unsigned long \*addr)
:   Clears a bit in memory

**Parameters**

`unsigned long nr`
:   the bit to clear

`volatile unsigned long *addr`
:   the address to start counting from

**Description**

Unlike [`clear_bit()`](#c.clear_bit "clear_bit"), this function is non-atomic. If it is called on the same
region of memory concurrently, the effect may be that only one operation
succeeds.

void \_\_\_change\_bit(unsigned long nr, volatile unsigned long \*addr)
:   Toggle a bit in memory

**Parameters**

`unsigned long nr`
:   the bit to change

`volatile unsigned long *addr`
:   the address to start counting from

**Description**

Unlike [`change_bit()`](#c.change_bit "change_bit"), this function is non-atomic. If it is called on the same
region of memory concurrently, the effect may be that only one operation
succeeds.

bool \_\_\_test\_and\_set\_bit(unsigned long nr, volatile unsigned long \*addr)
:   Set a bit and return its old value

**Parameters**

`unsigned long nr`
:   Bit to set

`volatile unsigned long *addr`
:   Address to count from

**Description**

This operation is non-atomic. If two instances of this operation race, one
can appear to succeed but actually fail.

bool \_\_\_test\_and\_clear\_bit(unsigned long nr, volatile unsigned long \*addr)
:   Clear a bit and return its old value

**Parameters**

`unsigned long nr`
:   Bit to clear

`volatile unsigned long *addr`
:   Address to count from

**Description**

This operation is non-atomic. If two instances of this operation race, one
can appear to succeed but actually fail.

bool \_\_\_test\_and\_change\_bit(unsigned long nr, volatile unsigned long \*addr)
:   Change a bit and return its old value

**Parameters**

`unsigned long nr`
:   Bit to change

`volatile unsigned long *addr`
:   Address to count from

**Description**

This operation is non-atomic. If two instances of this operation race, one
can appear to succeed but actually fail.

bool \_test\_bit(unsigned long nr, volatile const unsigned long \*addr)
:   Determine whether a bit is set

**Parameters**

`unsigned long nr`
:   bit number to test

`const volatile unsigned long *addr`
:   Address to start counting from

bool \_test\_bit\_acquire(unsigned long nr, volatile const unsigned long \*addr)
:   Determine, with acquire semantics, whether a bit is set

**Parameters**

`unsigned long nr`
:   bit number to test

`const volatile unsigned long *addr`
:   Address to start counting from

void clear\_bit\_unlock(long nr, volatile unsigned long \*addr)
:   Clear a bit in memory, for unlock

**Parameters**

`long nr`
:   the bit to set

`volatile unsigned long *addr`
:   the address to start counting from

**Description**

This operation is atomic and provides release barrier semantics.

void \_\_clear\_bit\_unlock(long nr, volatile unsigned long \*addr)
:   Clears a bit in memory

**Parameters**

`long nr`
:   Bit to clear

`volatile unsigned long *addr`
:   Address to start counting from

**Description**

This is a non-atomic operation but implies a release barrier before the
memory operation. It can be used for an unlock if no other CPUs can
concurrently modify other bits in the word.

bool test\_and\_set\_bit\_lock(long nr, volatile unsigned long \*addr)
:   Set a bit and return its old value, for lock

**Parameters**

`long nr`
:   Bit to set

`volatile unsigned long *addr`
:   Address to count from

**Description**

This operation is atomic and provides acquire barrier semantics if
the returned value is 0.
It can be used to implement bit locks.

bool xor\_unlock\_is\_negative\_byte(unsigned long mask, volatile unsigned long \*addr)
:   XOR a single byte in memory and test if it is negative, for unlock.

**Parameters**

`unsigned long mask`
:   Change the bits which are set in this mask.

`volatile unsigned long *addr`
:   The address of the word containing the byte to change.

**Description**

Changes some of bits 0-6 in the word pointed to by **addr**.
This operation is atomic and provides release barrier semantics.
Used to optimise some folio operations which are commonly paired
with an unlock or end of writeback. Bit 7 is used as PG\_waiters to
indicate whether anybody is waiting for the unlock.

**Return**

Whether the top bit of the byte is set.

### Bitmap Operations

bitmaps provide an array of bits, implemented using an
array of unsigned longs. The number of valid bits in a
given bitmap does \_not\_ need to be an exact multiple of
BITS\_PER\_LONG.

The possible unused bits in the last, partially used word
of a bitmap are ‘don’t care’. The implementation makes
no particular effort to keep them zero. It ensures that
their value will not affect the results of any operation.
The bitmap operations that return Boolean (bitmap\_empty,
for example) or scalar (bitmap\_weight, for example) results
carefully filter out these unused bits from impacting their
results.

The byte ordering of bitmaps is more natural on little
endian architectures. See the big-endian headers
include/asm-ppc64/bitops.h and include/asm-s390/bitops.h
for the best explanations of this ordering.

The DECLARE\_BITMAP(name,bits) macro, in linux/types.h, can be used
to declare an array named ‘name’ of just enough unsigned longs to
contain all bit positions from 0 to ‘bits’ - 1.

The available bitmap operations and their rough meaning in the
case that the bitmap is a single unsigned long are thus:

The generated code is more efficient when nbits is known at
compile-time and at most BITS\_PER\_LONG.

```
bitmap_zero(dst, nbits)                     *dst = 0UL
bitmap_fill(dst, nbits)                     *dst = ~0UL
bitmap_copy(dst, src, nbits)                *dst = *src
bitmap_and(dst, src1, src2, nbits)          *dst = *src1 & *src2
bitmap_or(dst, src1, src2, nbits)           *dst = *src1 | *src2
bitmap_weighted_or(dst, src1, src2, nbits)  *dst = *src1 | *src2. Returns Hamming Weight of dst
bitmap_weighted_xor(dst, src1, src2, nbits) *dst = *src1 ^ *src2. Returns Hamming Weight of dst
bitmap_xor(dst, src1, src2, nbits)          *dst = *src1 ^ *src2
bitmap_andnot(dst, src1, src2, nbits)       *dst = *src1 & ~(*src2)
bitmap_complement(dst, src, nbits)          *dst = ~(*src)
bitmap_equal(src1, src2, nbits)             Are *src1 and *src2 equal?
bitmap_intersects(src1, src2, nbits)        Do *src1 and *src2 overlap?
bitmap_subset(src1, src2, nbits)            Is *src1 a subset of *src2?
bitmap_empty(src, nbits)                    Are all bits zero in *src?
bitmap_full(src, nbits)                     Are all bits set in *src?
bitmap_weight(src, nbits)                   Hamming Weight: number set bits
bitmap_weight_and(src1, src2, nbits)        Hamming Weight of and'ed bitmap
bitmap_weight_andnot(src1, src2, nbits)     Hamming Weight of andnot'ed bitmap
bitmap_weight_from(src, start, end)         Hamming Weight starting from **start**
bitmap_set(dst, pos, nbits)                 Set specified bit area
bitmap_clear(dst, pos, nbits)               Clear specified bit area
bitmap_find_next_zero_area(buf, len, pos, n, mask)  Find bit free area
bitmap_find_next_zero_area_off(buf, len, pos, n, mask, mask_off)  as above
bitmap_shift_right(dst, src, n, nbits)      *dst = *src >> n
bitmap_shift_left(dst, src, n, nbits)       *dst = *src << n
bitmap_cut(dst, src, first, n, nbits)       Cut n bits from first, copy rest
bitmap_replace(dst, old, new, mask, nbits)  *dst = (*old & ~(*mask)) | (*new & *mask)
bitmap_scatter(dst, src, mask, nbits)       *dst = map(dense, sparse)(src)
bitmap_gather(dst, src, mask, nbits)        *dst = map(sparse, dense)(src)
bitmap_remap(dst, src, old, new, nbits)     *dst = map(old, new)(src)
bitmap_bitremap(oldbit, old, new, nbits)    newbit = map(old, new)(oldbit)
bitmap_onto(dst, orig, relmap, nbits)       *dst = orig relative to relmap
bitmap_fold(dst, orig, sz, nbits)           dst bits = orig bits mod sz
bitmap_parse(buf, buflen, dst, nbits)       Parse bitmap dst from kernel buf
bitmap_parse_user(ubuf, ulen, dst, nbits)   Parse bitmap dst from user buf
bitmap_parselist(buf, dst, nbits)           Parse bitmap dst from kernel buf
bitmap_parselist_user(buf, dst, nbits)      Parse bitmap dst from user buf
bitmap_find_free_region(bitmap, bits, order)  Find and allocate bit region
bitmap_release_region(bitmap, pos, order)   Free specified bit region
bitmap_allocate_region(bitmap, pos, order)  Allocate specified bit region
bitmap_from_arr32(dst, buf, nbits)          Copy nbits from u32[] buf to dst
bitmap_from_arr64(dst, buf, nbits)          Copy nbits from u64[] buf to dst
bitmap_to_arr32(buf, src, nbits)            Copy nbits from buf to u32[] dst
bitmap_to_arr64(buf, src, nbits)            Copy nbits from buf to u64[] dst
bitmap_get_value8(map, start)               Get 8bit value from map at start
bitmap_set_value8(map, value, start)        Set 8bit value to map at start
bitmap_read(map, start, nbits)              Read an nbits-sized value from
                                            map at start
bitmap_write(map, value, start, nbits)      Write an nbits-sized value to
                                            map at start
```

Note, `bitmap_zero()` and `bitmap_fill()` operate over the region of
unsigned longs, that is, bits behind bitmap till the unsigned long
boundary will be zeroed or filled as well. Consider to use
`bitmap_clear()` or `bitmap_set()` to make explicit zeroing or filling
respectively.

Also the following operations in asm/bitops.h apply to bitmaps.:

```
set_bit(bit, addr)                  *addr |= bit
clear_bit(bit, addr)                *addr &= ~bit
change_bit(bit, addr)               *addr ^= bit
test_bit(bit, addr)                 Is bit set in *addr?
test_and_set_bit(bit, addr)         Set bit and return old value
test_and_clear_bit(bit, addr)       Clear bit and return old value
test_and_change_bit(bit, addr)      Change bit and return old value
find_first_zero_bit(addr, nbits)    Position first zero bit in *addr
find_first_bit(addr, nbits)         Position first set bit in *addr
find_next_zero_bit(addr, nbits, bit)
                                    Position next zero bit in *addr >= bit
find_next_bit(addr, nbits, bit)     Position next set bit in *addr >= bit
find_next_and_bit(addr1, addr2, nbits, bit)
                                    Same as find_next_bit, but in
                                    (*addr1 & *addr2)
```

void \_\_bitmap\_shift\_right(unsigned long \*dst, const unsigned long \*src, unsigned shift, unsigned nbits)
:   logical right shift of the bits in a bitmap

**Parameters**

`unsigned long *dst`
:   destination bitmap

`const unsigned long *src`
:   source bitmap

`unsigned shift`
:   shift by this many bits

`unsigned nbits`
:   bitmap size, in bits

**Description**

Shifting right (dividing) means moving bits in the MS -> LS bit
direction. Zeros are fed into the vacated MS positions and the
LS bits shifted off the bottom are lost.

void \_\_bitmap\_shift\_left(unsigned long \*dst, const unsigned long \*src, unsigned int shift, unsigned int nbits)
:   logical left shift of the bits in a bitmap

**Parameters**

`unsigned long *dst`
:   destination bitmap

`const unsigned long *src`
:   source bitmap

`unsigned int shift`
:   shift by this many bits

`unsigned int nbits`
:   bitmap size, in bits

**Description**

Shifting left (multiplying) means moving bits in the LS -> MS
direction. Zeros are fed into the vacated LS bit positions
and those MS bits shifted off the top are lost.

void bitmap\_cut(unsigned long \*dst, const unsigned long \*src, unsigned int first, unsigned int cut, unsigned int nbits)
:   remove bit region from bitmap and right shift remaining bits

**Parameters**

`unsigned long *dst`
:   destination bitmap, might overlap with src

`const unsigned long *src`
:   source bitmap

`unsigned int first`
:   start bit of region to be removed

`unsigned int cut`
:   number of bits to remove

`unsigned int nbits`
:   bitmap size, in bits

**Description**

Set the n-th bit of **dst** iff the n-th bit of **src** is set and
n is less than **first**, or the m-th bit of **src** is set for any
m such that **first** <= n < nbits, and m = n + **cut**.

In pictures, example for a big-endian 32-bit architecture:

The **src** bitmap is:

```
31                                   63
|                                    |
10000000 11000001 11110010 00010101  10000000 11000001 01110010 00010101
                |  |              |                                    |
               16  14             0                                   32
```

if **cut** is 3, and **first** is 14, bits 14-16 in **src** are cut and **dst** is:

```
31                                   63
|                                    |
10110000 00011000 00110010 00010101  00010000 00011000 00101110 01000010
                   |              |                                    |
                   14 (bit 17     0                                   32
                       from @src)
```

Note that **dst** and **src** might overlap partially or entirely.

This is implemented in the obvious way, with a shift and carry
step for each moved bit. Optimisation is left as an exercise
for the compiler.

unsigned long bitmap\_find\_next\_zero\_area\_off(unsigned long \*map, unsigned long size, unsigned long start, unsigned int nr, unsigned long align\_mask, unsigned long align\_offset)
:   find a contiguous aligned zero area

**Parameters**

`unsigned long *map`
:   The address to base the search on

`unsigned long size`
:   The bitmap size in bits

`unsigned long start`
:   The bitnumber to start searching at

`unsigned int nr`
:   The number of zeroed bits we’re looking for

`unsigned long align_mask`
:   Alignment mask for zero area

`unsigned long align_offset`
:   Alignment offset for zero area.

**Description**

The **align\_mask** should be one less than a power of 2; the effect is that
the bit offset of all zero areas this function finds plus **align\_offset**
is multiple of that power of 2.

void bitmap\_remap(unsigned long \*dst, const unsigned long \*src, const unsigned long \*old, const unsigned long \*new, unsigned int nbits)
:   Apply map defined by a pair of bitmaps to another bitmap

**Parameters**

`unsigned long *dst`
:   remapped result

`const unsigned long *src`
:   subset to be remapped

`const unsigned long *old`
:   defines domain of map

`const unsigned long *new`
:   defines range of map

`unsigned int nbits`
:   number of bits in each of these bitmaps

**Description**

Let **old** and **new** define a mapping of bit positions, such that
whatever position is held by the n-th set bit in **old** is mapped
to the n-th set bit in **new**. In the more general case, allowing
for the possibility that the weight ‘w’ of **new** is less than the
weight of **old**, map the position of the n-th set bit in **old** to
the position of the m-th set bit in **new**, where m == n % w.

If either of the **old** and **new** bitmaps are empty, or if **src** and
**dst** point to the same location, then this routine copies **src**
to **dst**.

The positions of unset bits in **old** are mapped to themselves
(the identity map).

Apply the above specified mapping to **src**, placing the result in
**dst**, clearing any bits previously set in **dst**.

For example, lets say that **old** has bits 4 through 7 set, and
**new** has bits 12 through 15 set. This defines the mapping of bit
position 4 to 12, 5 to 13, 6 to 14 and 7 to 15, and of all other
bit positions unchanged. So if say **src** comes into this routine
with bits 1, 5 and 7 set, then **dst** should leave with bits 1,
13 and 15 set.

int bitmap\_bitremap(int oldbit, const unsigned long \*old, const unsigned long \*new, int bits)
:   Apply map defined by a pair of bitmaps to a single bit

**Parameters**

`int oldbit`
:   bit position to be mapped

`const unsigned long *old`
:   defines domain of map

`const unsigned long *new`
:   defines range of map

`int bits`
:   number of bits in each of these bitmaps

**Description**

Let **old** and **new** define a mapping of bit positions, such that
whatever position is held by the n-th set bit in **old** is mapped
to the n-th set bit in **new**. In the more general case, allowing
for the possibility that the weight ‘w’ of **new** is less than the
weight of **old**, map the position of the n-th set bit in **old** to
the position of the m-th set bit in **new**, where m == n % w.

The positions of unset bits in **old** are mapped to themselves
(the identity map).

Apply the above specified mapping to bit position **oldbit**, returning
the new bit position.

For example, lets say that **old** has bits 4 through 7 set, and
**new** has bits 12 through 15 set. This defines the mapping of bit
position 4 to 12, 5 to 13, 6 to 14 and 7 to 15, and of all other
bit positions unchanged. So if say **oldbit** is 5, then this routine
returns 13.

void bitmap\_from\_arr32(unsigned long \*bitmap, const u32 \*buf, unsigned int nbits)
:   copy the contents of u32 array of bits to bitmap

**Parameters**

`unsigned long *bitmap`
:   array of unsigned longs, the destination bitmap

`const u32 *buf`
:   array of u32 (in host byte order), the source bitmap

`unsigned int nbits`
:   number of bits in **bitmap**

void bitmap\_to\_arr32(u32 \*buf, const unsigned long \*bitmap, unsigned int nbits)
:   copy the contents of bitmap to a u32 array of bits

**Parameters**

`u32 *buf`
:   array of u32 (in host byte order), the dest bitmap

`const unsigned long *bitmap`
:   array of unsigned longs, the source bitmap

`unsigned int nbits`
:   number of bits in **bitmap**

void bitmap\_from\_arr64(unsigned long \*bitmap, const u64 \*buf, unsigned int nbits)
:   copy the contents of u64 array of bits to bitmap

**Parameters**

`unsigned long *bitmap`
:   array of unsigned longs, the destination bitmap

`const u64 *buf`
:   array of u64 (in host byte order), the source bitmap

`unsigned int nbits`
:   number of bits in **bitmap**

void bitmap\_to\_arr64(u64 \*buf, const unsigned long \*bitmap, unsigned int nbits)
:   copy the contents of bitmap to a u64 array of bits

**Parameters**

`u64 *buf`
:   array of u64 (in host byte order), the dest bitmap

`const unsigned long *bitmap`
:   array of unsigned longs, the source bitmap

`unsigned int nbits`
:   number of bits in **bitmap**

int bitmap\_pos\_to\_ord(const unsigned long \*buf, unsigned int pos, unsigned int nbits)
:   find ordinal of set bit at given position in bitmap

**Parameters**

`const unsigned long *buf`
:   pointer to a bitmap

`unsigned int pos`
:   a bit position in **buf** (0 <= **pos** < **nbits**)

`unsigned int nbits`
:   number of valid bit positions in **buf**

**Description**

Map the bit at position **pos** in **buf** (of length **nbits**) to the
ordinal of which set bit it is. If it is not set or if **pos**
is not a valid bit position, map to -1.

If for example, just bits 4 through 7 are set in **buf**, then **pos**
values 4 through 7 will get mapped to 0 through 3, respectively,
and other **pos** values will get mapped to -1. When **pos** value 7
gets mapped to (returns) **ord** value 3 in this example, that means
that bit 7 is the 3rd (starting with 0th) set bit in **buf**.

The bit positions 0 through **bits** are valid positions in **buf**.

void bitmap\_onto(unsigned long \*dst, const unsigned long \*orig, const unsigned long \*relmap, unsigned int bits)
:   translate one bitmap relative to another

**Parameters**

`unsigned long *dst`
:   resulting translated bitmap

`const unsigned long *orig`
:   original untranslated bitmap

`const unsigned long *relmap`
:   bitmap relative to which translated

`unsigned int bits`
:   number of bits in each of these bitmaps

**Description**

Set the n-th bit of **dst** iff there exists some m such that the
n-th bit of **relmap** is set, the m-th bit of **orig** is set, and
the n-th bit of **relmap** is also the m-th \_set\_ bit of **relmap**.
(If you understood the previous sentence the first time your
read it, you’re overqualified for your current job.)

In other words, **orig** is mapped onto (surjectively) **dst**,
using the map { <n, m> | the n-th bit of **relmap** is the
m-th set bit of **relmap** }.

Any set bits in **orig** above bit number W, where W is the
weight of (number of set bits in) **relmap** are mapped nowhere.
In particular, if for all bits m set in **orig**, m >= W, then
**dst** will end up empty. In situations where the possibility
of such an empty result is not desired, one way to avoid it is
to use the [`bitmap_fold()`](#c.bitmap_fold "bitmap_fold") operator, below, to first fold the
**orig** bitmap over itself so that all its set bits x are in the
range 0 <= x < W. The [`bitmap_fold()`](#c.bitmap_fold "bitmap_fold") operator does this by
setting the bit (m % W) in **dst**, for each bit (m) set in **orig**.

Example [1] for bitmap\_onto():
:   Let’s say **relmap** has bits 30-39 set, and **orig** has bits
    1, 3, 5, 7, 9 and 11 set. Then on return from this routine,
    **dst** will have bits 31, 33, 35, 37 and 39 set.

    When bit 0 is set in **orig**, it means turn on the bit in
    **dst** corresponding to whatever is the first bit (if any)
    that is turned on in **relmap**. Since bit 0 was off in the
    above example, we leave off that bit (bit 30) in **dst**.

    When bit 1 is set in **orig** (as in the above example), it
    means turn on the bit in **dst** corresponding to whatever
    is the second bit that is turned on in **relmap**. The second
    bit in **relmap** that was turned on in the above example was
    bit 31, so we turned on bit 31 in **dst**.

    Similarly, we turned on bits 33, 35, 37 and 39 in **dst**,
    because they were the 4th, 6th, 8th and 10th set bits
    set in **relmap**, and the 4th, 6th, 8th and 10th bits of
    **orig** (i.e. bits 3, 5, 7 and 9) were also set.

    When bit 11 is set in **orig**, it means turn on the bit in
    **dst** corresponding to whatever is the twelfth bit that is
    turned on in **relmap**. In the above example, there were
    only ten bits turned on in **relmap** (30..39), so that bit
    11 was set in **orig** had no affect on **dst**.

Example [2] for bitmap\_fold() + bitmap\_onto():
:   Let’s say **relmap** has these ten bits set:

    ```
    40 41 42 43 45 48 53 61 74 95
    ```

    (for the curious, that’s 40 plus the first ten terms of the
    Fibonacci sequence.)

    Further lets say we use the following code, invoking
    [`bitmap_fold()`](#c.bitmap_fold "bitmap_fold") then bitmap\_onto, as suggested above to
    avoid the possibility of an empty **dst** result:

    ```
    unsigned long *tmp;     // a temporary bitmap's bits

    bitmap_fold(tmp, orig, bitmap_weight(relmap, bits), bits);
    bitmap_onto(dst, tmp, relmap, bits);
    ```

    Then this table shows what various values of **dst** would be, for
    various **orig**’s. I list the zero-based positions of each set bit.
    The tmp column shows the intermediate result, as computed by
    using [`bitmap_fold()`](#c.bitmap_fold "bitmap_fold") to fold the **orig** bitmap modulo ten
    (the weight of **relmap**):

    > |  |  |  |
    > | --- | --- | --- |
    > | **orig** | tmp | **dst** |
    > | 0 | 0 | 40 |
    > | 1 | 1 | 41 |
    > | 9 | 9 | 95 |
    > | 10 | 0 | 40 [[1]](#f1) |
    > | 1 3 5 7 | 1 3 5 7 | 41 43 48 61 |
    > | 0 1 2 3 4 | 0 1 2 3 4 | 40 41 42 43 45 |
    > | 0 9 18 27 | 0 9 8 7 | 40 61 74 95 |
    > | 0 10 20 30 | 0 | 40 |
    > | 0 11 22 33 | 0 1 2 3 | 40 41 42 43 |
    > | 0 12 24 36 | 0 2 4 6 | 40 42 45 53 |
    > | 78 102 211 | 1 2 8 | 41 42 74 [[1]](#f1) |

[1]
([1](#id1),[2](#id2))

For these marked lines, if we hadn’t first done [`bitmap_fold()`](#c.bitmap_fold "bitmap_fold")
into tmp, then the **dst** result would have been empty.

If either of **orig** or **relmap** is empty (no set bits), then **dst**
will be returned empty.

If (as explained above) the only set bits in **orig** are in positions
m where m >= W, (where W is the weight of **relmap**) then **dst** will
once again be returned empty.

All bits in **dst** not set by the above rule are cleared.

void bitmap\_fold(unsigned long \*dst, const unsigned long \*orig, unsigned int sz, unsigned int nbits)
:   fold larger bitmap into smaller, modulo specified size

**Parameters**

`unsigned long *dst`
:   resulting smaller bitmap

`const unsigned long *orig`
:   original larger bitmap

`unsigned int sz`
:   specified size

`unsigned int nbits`
:   number of bits in each of these bitmaps

**Description**

For each bit oldbit in **orig**, set bit oldbit mod **sz** in **dst**.
Clear all other bits in **dst**. See further the comment and
Example [2] for [`bitmap_onto()`](#c.bitmap_onto "bitmap_onto") for why and how to use this.

unsigned long bitmap\_find\_next\_zero\_area(unsigned long \*map, unsigned long size, unsigned long start, unsigned int nr, unsigned long align\_mask)
:   find a contiguous aligned zero area

**Parameters**

`unsigned long *map`
:   The address to base the search on

`unsigned long size`
:   The bitmap size in bits

`unsigned long start`
:   The bitnumber to start searching at

`unsigned int nr`
:   The number of zeroed bits we’re looking for

`unsigned long align_mask`
:   Alignment mask for zero area

**Description**

The **align\_mask** should be one less than a power of 2; the effect is that
the bit offset of all zero areas this function finds is multiples of that
power of 2. A **align\_mask** of 0 means no alignment is required.

bool bitmap\_or\_equal(const unsigned long \*src1, const unsigned long \*src2, const unsigned long \*src3, unsigned int nbits)
:   Check whether the or of two bitmaps is equal to a third

**Parameters**

`const unsigned long *src1`
:   Pointer to bitmap 1

`const unsigned long *src2`
:   Pointer to bitmap 2 will be or’ed with bitmap 1

`const unsigned long *src3`
:   Pointer to bitmap 3. Compare to the result of **\*src1** | **\*src2**

`unsigned int nbits`
:   number of bits in each of these bitmaps

**Return**

True if (**\*src1** | **\*src2**) == **\*src3**, false otherwise

unsigned long bitmap\_weight\_from(const unsigned long \*bitmap, unsigned int start, unsigned int end)
:   Hamming weight for a memory region

**Parameters**

`const unsigned long *bitmap`
:   The base address

`unsigned int start`
:   The bitnumber to starts weighting

`unsigned int end`
:   the bitmap size in bits

**Description**

Returns the number of set bits in the region. If **start** >= **end**,
return >= end.

void bitmap\_scatter(unsigned long \*dst, const unsigned long \*src, const unsigned long \*mask, unsigned int nbits)
:   Scatter a bitmap according to the given mask

**Parameters**

`unsigned long *dst`
:   scattered bitmap

`const unsigned long *src`
:   gathered bitmap

`const unsigned long *mask`
:   mask representing bits to assign to in the scattered bitmap

`unsigned int nbits`
:   number of bits in each of these bitmaps

**Description**

Scatters bitmap with sequential bits according to the given **mask**.

**Example**

If **src** bitmap = 0x005a, with **mask** = 0x1313, **dst** will be 0x0302.

Or in binary form
**src** **mask** **dst**
0000000001011010 0001001100010011 0000001100000010

(Bits 0, 1, 2, 3, 4, 5 are copied to the bits 0, 1, 4, 8, 9, 12)

A more ‘visual’ description of the operation:

```
src:  0000000001011010
                ||||||
         +------+|||||
         |  +----+||||
         |  |+----+|||
         |  ||   +-+||
         |  ||   |  ||
mask: ...v..vv...v..vv
      ...0..11...0..10
dst:  0000001100000010
```

A relationship exists between [`bitmap_scatter()`](#c.bitmap_scatter "bitmap_scatter") and [`bitmap_gather()`](#c.bitmap_gather "bitmap_gather"). See
[`bitmap_gather()`](#c.bitmap_gather "bitmap_gather") for the bitmap gather detailed operations. TL;DR:
[`bitmap_gather()`](#c.bitmap_gather "bitmap_gather") can be seen as the ‘reverse’ [`bitmap_scatter()`](#c.bitmap_scatter "bitmap_scatter") operation.

void bitmap\_gather(unsigned long \*dst, const unsigned long \*src, const unsigned long \*mask, unsigned int nbits)
:   Gather a bitmap according to given mask

**Parameters**

`unsigned long *dst`
:   gathered bitmap

`const unsigned long *src`
:   scattered bitmap

`const unsigned long *mask`
:   mask representing bits to extract from in the scattered bitmap

`unsigned int nbits`
:   number of bits in each of these bitmaps

**Description**

Gathers bitmap with sparse bits according to the given **mask**.

**Example**

If **src** bitmap = 0x0302, with **mask** = 0x1313, **dst** will be 0x001a.

Or in binary form
**src** **mask** **dst**
0000001100000010 0001001100010011 0000000000011010

(Bits 0, 1, 4, 8, 9, 12 are copied to the bits 0, 1, 2, 3, 4, 5)

A more ‘visual’ description of the operation:

```
mask: ...v..vv...v..vv
src:  0000001100000010
         ^  ^^   ^   0
         |  ||   |  10
         |  ||   > 010
         |  |+--> 1010
         |  +--> 11010
         +----> 011010
dst:  0000000000011010
```

A relationship exists between [`bitmap_gather()`](#c.bitmap_gather "bitmap_gather") and [`bitmap_scatter()`](#c.bitmap_scatter "bitmap_scatter"). See
[`bitmap_scatter()`](#c.bitmap_scatter "bitmap_scatter") for the bitmap scatter detailed operations. TL;DR:
[`bitmap_scatter()`](#c.bitmap_scatter "bitmap_scatter") can be seen as the ‘reverse’ [`bitmap_gather()`](#c.bitmap_gather "bitmap_gather") operation.

Suppose scattered computed using bitmap\_scatter(scattered, src, mask, n).
The operation bitmap\_gather(result, scattered, mask, n) leads to a result
equal or equivalent to src.

The result can be ‘equivalent’ because [`bitmap_scatter()`](#c.bitmap_scatter "bitmap_scatter") and [`bitmap_gather()`](#c.bitmap_gather "bitmap_gather")
are not bijective.
The result and src values are equivalent in that sense that a call to
bitmap\_scatter(res, src, mask, n) and a call to
bitmap\_scatter(res, result, mask, n) will lead to the same res value.

void bitmap\_release\_region(unsigned long \*bitmap, unsigned int pos, int order)
:   release allocated bitmap region

**Parameters**

`unsigned long *bitmap`
:   array of unsigned longs corresponding to the bitmap

`unsigned int pos`
:   beginning of bit region to release

`int order`
:   region size (log base 2 of number of bits) to release

**Description**

This is the complement to `__bitmap_find_free_region()` and releases
the found region (by clearing it in the bitmap).

int bitmap\_allocate\_region(unsigned long \*bitmap, unsigned int pos, int order)
:   allocate bitmap region

**Parameters**

`unsigned long *bitmap`
:   array of unsigned longs corresponding to the bitmap

`unsigned int pos`
:   beginning of bit region to allocate

`int order`
:   region size (log base 2 of number of bits) to allocate

**Description**

Allocate (set bits in) a specified region of a bitmap.

**Return**

0 on success, or `-EBUSY` if specified region wasn’t
free (not all bits were zero).

int bitmap\_find\_free\_region(unsigned long \*bitmap, unsigned int bits, int order)
:   find a contiguous aligned mem region

**Parameters**

`unsigned long *bitmap`
:   array of unsigned longs corresponding to the bitmap

`unsigned int bits`
:   number of bits in the bitmap

`int order`
:   region size (log base 2 of number of bits) to find

**Description**

Find a region of free (zero) bits in a **bitmap** of **bits** bits and
allocate them (set them to one). Only consider regions of length
a power (**order**) of two, aligned to that power of two, which
makes the search algorithm much faster.

**Return**

the bit offset in bitmap of the allocated region,
or -errno on failure.

BITMAP\_FROM\_U64

`BITMAP_FROM_U64 (n)`

> Represent u64 value in the format suitable for bitmap.

**Parameters**

`n`
:   u64 value

**Description**

Linux bitmaps are internally arrays of unsigned longs, i.e. 32-bit
integers in 32-bit environment, and 64-bit integers in 64-bit one.

There are four combinations of endianness and length of the word in linux
ABIs: LE64, BE64, LE32 and BE32.

On 64-bit kernels 64-bit LE and BE numbers are naturally ordered in
bitmaps and therefore don’t require any special handling.

On 32-bit kernels 32-bit LE ABI orders lo word of 64-bit number in memory
prior to hi, and 32-bit BE orders hi word prior to lo. The bitmap on the
other hand is represented as an array of 32-bit words and the position of
bit N may therefore be calculated as: word #(N/32) and bit #(N``32``) in that
word. For example, bit #42 is located at 10th position of 2nd word.
It matches 32-bit LE ABI, and we can simply let the compiler store 64-bit
values in memory as it usually does. But for BE we need to swap hi and lo
words manually.

With all that, the macro [`BITMAP_FROM_U64()`](#c.BITMAP_FROM_U64 "BITMAP_FROM_U64") does explicit reordering of hi and
lo parts of u64. For LE32 it does nothing, and for BE environment it swaps
hi and lo words, as is expected by bitmap.

void bitmap\_from\_u64(unsigned long \*dst, u64 mask)
:   Check and swap words within u64.

**Parameters**

`unsigned long *dst`
:   destination bitmap

`u64 mask`
:   source bitmap

**Description**

In 32-bit Big Endian kernel, when using `` (u32 *)(:c:type:`val`)[*] ``
to read u64 mask, we will get the wrong word.
That is `` (u32 *)(:c:type:`val`)[0] `` gets the upper 32 bits,
but we expect the lower 32-bits of u64.

unsigned long bitmap\_read(const unsigned long \*map, unsigned long start, unsigned long nbits)
:   read a value of n-bits from the memory region

**Parameters**

`const unsigned long *map`
:   address to the bitmap memory region

`unsigned long start`
:   bit offset of the n-bit value

`unsigned long nbits`
:   size of value in bits, nonzero, up to BITS\_PER\_LONG

**Return**

value of **nbits** bits located at the **start** bit offset within the
**map** memory region. For **nbits** = 0 and **nbits** > BITS\_PER\_LONG the return
value is undefined.

void bitmap\_write(unsigned long \*map, unsigned long value, unsigned long start, unsigned long nbits)
:   write n-bit value within a memory region

**Parameters**

`unsigned long *map`
:   address to the bitmap memory region

`unsigned long value`
:   value to write, clamped to nbits

`unsigned long start`
:   bit offset of the n-bit value

`unsigned long nbits`
:   size of value in bits, nonzero, up to BITS\_PER\_LONG.

**Description**

[`bitmap_write()`](#c.bitmap_write "bitmap_write") behaves as-if implemented as **nbits** calls of `__assign_bit()`,
i.e. bits beyond **nbits** are ignored:

> for (bit = 0; bit < nbits; bit++)
> :   \_\_assign\_bit(start + bit, bitmap, val & BIT(bit));

For **nbits** == 0 and **nbits** > BITS\_PER\_LONG no writes are performed.

### Command-line Parsing

int get\_option(char \*\*str, int \*pint)
:   Parse integer from an option string

**Parameters**

`char **str`
:   option string

`int *pint`
:   (optional output) integer value parsed from **str**

**Description**

> Read an int from an option string; if available accept a subsequent
> comma as well.
>
> When **pint** is NULL the function can be used as a validator of
> the current option in the string.
>
> Return values:
> 0 - no int in string
> 1 - int found, no subsequent comma
> 2 - int found including a subsequent comma
> 3 - hyphen found to denote a range
>
> Leading hyphen without integer is no integer case, but we consume it
> for the sake of simplification.

char \*get\_options(const char \*str, int nints, int \*ints)
:   Parse a string into a list of integers

**Parameters**

`const char *str`
:   String to be parsed

`int nints`
:   size of integer array

`int *ints`
:   integer array (must have room for at least one element)

**Description**

> This function parses a string containing a comma-separated
> list of integers, a hyphen-separated range of \_positive\_ integers,
> or a combination of both. The parse halts when the array is
> full, or when no more numbers can be retrieved from the
> string.
>
> When **nints** is 0, the function just validates the given **str** and
> returns the amount of parseable integers as described below.
>
> The first element is filled by the number of collected integers
> in the range. The rest is what was parsed from the **str**.
>
> Return value is the character in the string which caused
> the parse to end (typically a null terminator, if **str** is
> completely parseable).

unsigned long long memparse(const char \*ptr, char \*\*retptr)
:   parse a string with mem suffixes into a number

**Parameters**

`const char *ptr`
:   Where parse begins

`char **retptr`
:   (output) Optional pointer to next char after parse completes

**Description**

> Parses a string into a number. The number stored at **ptr** is
> potentially suffixed with K, M, G, T, P, E.

### Error Pointers

IS\_ERR\_VALUE

`IS_ERR_VALUE (x)`

> Detect an error pointer.

**Parameters**

`x`
:   The pointer to check.

**Description**

Like [`IS_ERR()`](#c.IS_ERR "IS_ERR"), but does not generate a compiler warning if result is unused.

void \*ERR\_PTR(long error)
:   Create an error pointer.

**Parameters**

`long error`
:   A negative error code.

**Description**

Encodes **error** into a pointer value. Users should consider the result
opaque and not assume anything about how the error is encoded.

**Return**

A pointer with **error** encoded within its value.

INIT\_ERR\_PTR

`INIT_ERR_PTR (error)`

> Init a const error pointer.

**Parameters**

`error`
:   A negative error code.

**Description**

Like [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR"), but usable to initialize static variables.

long PTR\_ERR(\_\_force const void \*ptr)
:   Extract the error code from an error pointer.

**Parameters**

`__force const void *ptr`
:   An error pointer.

**Return**

The error code within **ptr**.

bool IS\_ERR(\_\_force const void \*ptr)
:   Detect an error pointer.

**Parameters**

`__force const void *ptr`
:   The pointer to check.

**Return**

true if **ptr** is an error pointer, false otherwise.

bool IS\_ERR\_OR\_NULL(\_\_force const void \*ptr)
:   Detect an error pointer or a null pointer.

**Parameters**

`__force const void *ptr`
:   The pointer to check.

**Description**

Like [`IS_ERR()`](#c.IS_ERR "IS_ERR"), but also returns true for a null pointer.

void \*ERR\_CAST(\_\_force const void \*ptr)
:   Explicitly cast an error-valued pointer to another pointer type

**Parameters**

`__force const void *ptr`
:   The pointer to cast.

**Description**

Explicitly cast an error-valued pointer to another pointer type in such a
way as to make it clear that’s what’s going on.

int PTR\_ERR\_OR\_ZERO(\_\_force const void \*ptr)
:   Extract the error code from a pointer if it has one.

**Parameters**

`__force const void *ptr`
:   A potential error pointer.

**Description**

Convenience function that can be used inside a function that returns
an error code to propagate errors received as error pointers.
For example, `return PTR_ERR_OR_ZERO(ptr);` replaces:

```
if (IS_ERR(ptr))
        return PTR_ERR(ptr);
else
        return 0;
```

**Return**

The error code within **ptr** if it is an error pointer; 0 otherwise.

### Sorting

void sort\_r(void \*base, size\_t num, size\_t size, cmp\_r\_func\_t cmp\_func, swap\_r\_func\_t swap\_func, const void \*priv)
:   sort an array of elements

**Parameters**

`void *base`
:   pointer to data to sort

`size_t num`
:   number of elements

`size_t size`
:   size of each element

`cmp_r_func_t cmp_func`
:   pointer to comparison function

`swap_r_func_t swap_func`
:   pointer to swap function or NULL

`const void *priv`
:   third argument passed to comparison function

**Description**

This function does a heapsort on the given array. You may provide
a swap\_func function if you need to do something more than a memory
copy (e.g. fix up pointers or auxiliary data), but the built-in swap
avoids a slow retpoline and so is significantly faster.

The comparison function must adhere to specific mathematical
properties to ensure correct and stable sorting:
- Antisymmetry: cmp\_func(a, b) must return the opposite sign of
cmp\_func(b, a).
- Transitivity: if cmp\_func(a, b) <= 0 and cmp\_func(b, c) <= 0, then
cmp\_func(a, c) <= 0.

Sorting time is O(n log n) both on average and worst-case. While
quicksort is slightly faster on average, it suffers from exploitable
O(n\*n) worst-case behavior and extra memory requirements that make
it less suitable for kernel use.

void sort\_r\_nonatomic(void \*base, size\_t num, size\_t size, cmp\_r\_func\_t cmp\_func, swap\_r\_func\_t swap\_func, const void \*priv)
:   sort an array of elements, with cond\_resched

**Parameters**

`void *base`
:   pointer to data to sort

`size_t num`
:   number of elements

`size_t size`
:   size of each element

`cmp_r_func_t cmp_func`
:   pointer to comparison function

`swap_r_func_t swap_func`
:   pointer to swap function or NULL

`const void *priv`
:   third argument passed to comparison function

**Description**

Same as sort\_r, but preferred for larger arrays as it does a periodic
`cond_resched()`.

void list\_sort(void \*priv, struct list\_head \*head, list\_cmp\_func\_t cmp)
:   sort a list

**Parameters**

`void *priv`
:   private data, opaque to [`list_sort()`](#c.list_sort "list_sort"), passed to **cmp**

`struct list_head *head`
:   the list to sort

`list_cmp_func_t cmp`
:   the elements comparison function

**Description**

The comparison function **cmp** must return > 0 if **a** should sort after
**b** (”**a** > **b**” if you want an ascending sort), and <= 0 if **a** should
sort before **b** *or* their original order should be preserved. It is
always called with the element that came first in the input in **a**,
and list\_sort is a stable sort, so it is not necessary to distinguish
the **a** < **b** and **a** == **b** cases.

The comparison function must adhere to specific mathematical properties
to ensure correct and stable sorting:
- Antisymmetry: cmp(**a**, **b**) must return the opposite sign of
cmp(**b**, **a**).
- Transitivity: if cmp(**a**, **b**) <= 0 and cmp(**b**, **c**) <= 0, then
cmp(**a**, **c**) <= 0.

This is compatible with two styles of **cmp** function:
- The traditional style which returns <0 / =0 / >0, or
- Returning a boolean 0/1.
The latter offers a chance to save a few cycles in the comparison
(which is used by e.g. `plug_ctx_cmp()` in block/blk-mq.c).

A good way to write a multi-word comparison is:

```
if (a->high != b->high)
        return a->high > b->high;
if (a->middle != b->middle)
        return a->middle > b->middle;
return a->low > b->low;
```

This mergesort is as eager as possible while always performing at least
2:1 balanced merges. Given two pending sublists of size 2^k, they are
merged to a size-2^(k+1) list as soon as we have 2^k following elements.

Thus, it will avoid cache thrashing as long as 3\*2^k elements can
fit into the cache. Not quite as good as a fully-eager bottom-up
mergesort, but it does use 0.2\*n fewer comparisons, so is faster in
the common case that everything fits into L1.

The merging is controlled by “count”, the number of elements in the
pending lists. This is beautifully simple code, but rather subtle.

Each time we increment “count”, we set one bit (bit k) and clear
bits k-1 .. 0. Each time this happens (except the very first time
for each bit, when count increments to 2^k), we merge two lists of
size 2^k into one list of size 2^(k+1).

This merge happens exactly when the count reaches an odd multiple of
2^k, which is when we have 2^k elements pending in smaller lists,
so it’s safe to merge away two lists of size 2^k.

After this happens twice, we have created two lists of size 2^(k+1),
which will be merged into a list of size 2^(k+2) before we create
a third list of size 2^(k+1), so there are never more than two pending.

The number of pending lists of size 2^k is determined by the
state of bit k of “count” plus two extra pieces of information:

* The state of bit k-1 (when k == 0, consider bit -1 always set), and
* Whether the higher-order bits are zero or non-zero (i.e.
  is count >= 2^(k+1)).

There are six states we distinguish. “x” represents some arbitrary
bits, and “y” represents some arbitrary non-zero bits:
0: 00x: 0 pending of size 2^k; x pending of sizes < 2^k
1: 01x: 0 pending of size 2^k; 2^(k-1) + x pending of sizes < 2^k
2: x10x: 0 pending of size 2^k; 2^k + x pending of sizes < 2^k
3: x11x: 1 pending of size 2^k; 2^(k-1) + x pending of sizes < 2^k
4: y00x: 1 pending of size 2^k; 2^k + x pending of sizes < 2^k
5: y01x: 2 pending of size 2^k; 2^(k-1) + x pending of sizes < 2^k
(merge and loop back to state 2)

We gain lists of size 2^k in the 2->3 and 4->5 transitions (because
bit k-1 is set while the more significant bits are non-zero) and
merge them away in the 5->2 transition. Note in particular that just
before the 5->2 transition, all lower-order bits are 11 (state 3),
so there is one list of each smaller size.

When we reach the end of the input, we merge all the pending
lists, from smallest to largest. If you work through cases 2 to
5 above, you can see that the number of elements we merge with a list
of size 2^k varies from 2^(k-1) (cases 3 and 5 when x == 0) to
2^(k+1) - 1 (second merge of case 5 when x == 2^(k-1) - 1).

### Text Searching

INTRODUCTION

> The textsearch infrastructure provides text searching facilities for
> both linear and non-linear data. Individual search algorithms are
> implemented in modules and chosen by the user.

ARCHITECTURE

```
  User
  +----------------+
  |        finish()|<--------------(6)-----------------+
  |get_next_block()|<--------------(5)---------------+ |
  |                |                     Algorithm   | |
  |                |                    +------------------------------+
  |                |                    |  init()   find()   destroy() |
  |                |                    +------------------------------+
  |                |       Core API           ^       ^          ^
  |                |      +---------------+  (2)     (4)        (8)
  |             (1)|----->| prepare()     |---+       |          |
  |             (3)|----->| find()/next() |-----------+          |
  |             (7)|----->| destroy()     |----------------------+
  +----------------+      +---------------+

(1) User configures a search by calling textsearch_prepare() specifying
    the search parameters such as the pattern and algorithm name.
(2) Core requests the algorithm to allocate and initialize a search
    configuration according to the specified parameters.
(3) User starts the search(es) by calling textsearch_find() or
    textsearch_next() to fetch subsequent occurrences. A state variable
    is provided to the algorithm to store persistent variables.
(4) Core eventually resets the search offset and forwards the find()
    request to the algorithm.
(5) Algorithm calls get_next_block() provided by the user continuously
    to fetch the data to be searched in block by block.
(6) Algorithm invokes finish() after the last call to get_next_block
    to clean up any leftovers from get_next_block. (Optional)
(7) User destroys the configuration by calling textsearch_destroy().
(8) Core notifies the algorithm to destroy algorithm specific
    allocations. (Optional)
```

USAGE

> Before a search can be performed, a configuration must be created
> by calling [`textsearch_prepare()`](#c.textsearch_prepare "textsearch_prepare") specifying the searching algorithm,
> the pattern to look for and flags. As a flag, you can set TS\_IGNORECASE
> to perform case insensitive matching. But it might slow down
> performance of algorithm, so you should use it at own your risk.
> The returned configuration may then be used for an arbitrary
> amount of times and even in parallel as long as a separate `struct
> ts_state` variable is provided to every instance.
>
> The actual search is performed by either calling
> [`textsearch_find_continuous()`](#c.textsearch_find_continuous "textsearch_find_continuous") for linear data or by providing
> an own `get_next_block()` implementation and
> calling [`textsearch_find()`](#c.textsearch_find "textsearch_find"). Both functions return
> the position of the first occurrence of the pattern or UINT\_MAX if
> no match was found. Subsequent occurrences can be found by calling
> [`textsearch_next()`](#c.textsearch_next "textsearch_next") regardless of the linearity of the data.
>
> Once you’re done using a configuration it must be given back via
> textsearch\_destroy.

EXAMPLE:

```
int pos;
struct ts_config *conf;
struct ts_state state;
const char *pattern = "chicken";
const char *example = "We dance the funky chicken";

conf = textsearch_prepare("kmp", pattern, strlen(pattern),
                          GFP_KERNEL, TS_AUTOLOAD);
if (IS_ERR(conf)) {
    err = PTR_ERR(conf);
    goto errout;
}

pos = textsearch_find_continuous(conf, &state, example, strlen(example));
if (pos != UINT_MAX)
    panic("Oh my god, dancing chickens at %d\n", pos);

textsearch_destroy(conf);
```

int textsearch\_register(struct ts\_ops \*ops)
:   register a textsearch module

**Parameters**

`struct ts_ops *ops`
:   operations lookup table

**Description**

This function must be called by textsearch modules to announce
their presence. The specified &\*\*ops\*\* must have `name` set to a
unique identifier and the callbacks `find()`, `init()`, `get_pattern()`,
and `get_pattern_len()` must be implemented.

Returns 0 or -EEXISTS if another module has already registered
with same name.

int textsearch\_unregister(struct ts\_ops \*ops)
:   unregister a textsearch module

**Parameters**

`struct ts_ops *ops`
:   operations lookup table

**Description**

This function must be called by textsearch modules to announce
their disappearance for examples when the module gets unloaded.
The `ops` parameter must be the same as the one during the
registration.

Returns 0 on success or -ENOENT if no matching textsearch
registration was found.

unsigned int textsearch\_find\_continuous(struct ts\_config \*conf, struct ts\_state \*state, const void \*data, unsigned int len)
:   search a pattern in continuous/linear data

**Parameters**

`struct ts_config *conf`
:   search configuration

`struct ts_state *state`
:   search state

`const void *data`
:   data to search in

`unsigned int len`
:   length of data

**Description**

A simplified version of [`textsearch_find()`](#c.textsearch_find "textsearch_find") for continuous/linear data.
Call [`textsearch_next()`](#c.textsearch_next "textsearch_next") to retrieve subsequent matches.

Returns the position of first occurrence of the pattern or
`UINT_MAX` if no occurrence was found.

struct ts\_config \*textsearch\_prepare(const char \*algo, const void \*pattern, unsigned int len, gfp\_t gfp\_mask, int flags)
:   Prepare a search

**Parameters**

`const char *algo`
:   name of search algorithm

`const void *pattern`
:   pattern data

`unsigned int len`
:   length of pattern

`gfp_t gfp_mask`
:   allocation mask

`int flags`
:   search flags

**Description**

Looks up the search algorithm module and creates a new textsearch
configuration for the specified pattern.

**Note**

The format of the pattern may not be compatible between
:   the various search algorithms.

Returns a new textsearch configuration according to the specified
parameters or a [`ERR_PTR()`](#c.ERR_PTR "ERR_PTR"). If a zero length pattern is passed, this
function returns EINVAL.

void textsearch\_destroy(struct ts\_config \*conf)
:   destroy a search configuration

**Parameters**

`struct ts_config *conf`
:   search configuration

**Description**

Releases all references of the configuration and frees
up the memory.

unsigned int textsearch\_next(struct ts\_config \*conf, struct ts\_state \*state)
:   continue searching for a pattern

**Parameters**

`struct ts_config *conf`
:   search configuration

`struct ts_state *state`
:   search state

**Description**

Continues a search looking for more occurrences of the pattern.
[`textsearch_find()`](#c.textsearch_find "textsearch_find") must be called to find the first occurrence
in order to reset the state.

Returns the position of the next occurrence of the pattern or
UINT\_MAX if not match was found.

unsigned int textsearch\_find(struct ts\_config \*conf, struct ts\_state \*state)
:   start searching for a pattern

**Parameters**

`struct ts_config *conf`
:   search configuration

`struct ts_state *state`
:   search state

**Description**

Returns the position of first occurrence of the pattern or
UINT\_MAX if no match was found.

void \*textsearch\_get\_pattern(struct ts\_config \*conf)
:   return head of the pattern

**Parameters**

`struct ts_config *conf`
:   search configuration

unsigned int textsearch\_get\_pattern\_len(struct ts\_config \*conf)
:   return length of the pattern

**Parameters**

`struct ts_config *conf`
:   search configuration

## CRC and Math Functions in Linux

### Arithmetic Overflow Checking

check\_add\_overflow

`check_add_overflow (a, b, d)`

> Calculate addition with overflow checking

**Parameters**

`a`
:   first addend

`b`
:   second addend

`d`
:   pointer to store sum

**Description**

Returns true on wrap-around, false otherwise.

**\*d** holds the results of the attempted addition, regardless of whether
wrap-around occurred.

wrapping\_add

`wrapping_add (type, a, b)`

> Intentionally perform a wrapping addition

**Parameters**

`type`
:   type for result of calculation

`a`
:   first addend

`b`
:   second addend

**Description**

Return the potentially wrapped-around addition without
tripping any wrap-around sanitizers that may be enabled.

wrapping\_assign\_add

`wrapping_assign_add (var, offset)`

> Intentionally perform a wrapping increment assignment

**Parameters**

`var`
:   variable to be incremented

`offset`
:   amount to add

**Description**

Increments **var** by **offset** with wrap-around. Returns the resulting
value of **var**. Will not trip any wrap-around sanitizers.

Returns the new value of **var**.

check\_sub\_overflow

`check_sub_overflow (a, b, d)`

> Calculate subtraction with overflow checking

**Parameters**

`a`
:   minuend; value to subtract from

`b`
:   subtrahend; value to subtract from **a**

`d`
:   pointer to store difference

**Description**

Returns true on wrap-around, false otherwise.

**\*d** holds the results of the attempted subtraction, regardless of whether
wrap-around occurred.

wrapping\_sub

`wrapping_sub (type, a, b)`

> Intentionally perform a wrapping subtraction

**Parameters**

`type`
:   type for result of calculation

`a`
:   minuend; value to subtract from

`b`
:   subtrahend; value to subtract from **a**

**Description**

Return the potentially wrapped-around subtraction without
tripping any wrap-around sanitizers that may be enabled.

wrapping\_assign\_sub

`wrapping_assign_sub (var, offset)`

> Intentionally perform a wrapping decrement assign

**Parameters**

`var`
:   variable to be decremented

`offset`
:   amount to subtract

**Description**

Decrements **var** by **offset** with wrap-around. Returns the resulting
value of **var**. Will not trip any wrap-around sanitizers.

Returns the new value of **var**.

check\_mul\_overflow

`check_mul_overflow (a, b, d)`

> Calculate multiplication with overflow checking

**Parameters**

`a`
:   first factor

`b`
:   second factor

`d`
:   pointer to store product

**Description**

Returns true on wrap-around, false otherwise.

**\*d** holds the results of the attempted multiplication, regardless of whether
wrap-around occurred.

wrapping\_mul

`wrapping_mul (type, a, b)`

> Intentionally perform a wrapping multiplication

**Parameters**

`type`
:   type for result of calculation

`a`
:   first factor

`b`
:   second factor

**Description**

Return the potentially wrapped-around multiplication without
tripping any wrap-around sanitizers that may be enabled.

check\_shl\_overflow

`check_shl_overflow (a, s, d)`

> Calculate a left-shifted value and check overflow

**Parameters**

`a`
:   Value to be shifted

`s`
:   How many bits left to shift

`d`
:   Pointer to where to store the result

**Description**

Computes **\*d** = (**a** << **s**)

Returns true if ‘**\*d**’ cannot hold the result or when ‘**a** << **s**’ doesn’t
make sense. Example conditions:

* ‘**a** << **s**’ causes bits to be lost when stored in **\*d**.
* ‘**s**’ is garbage (e.g. negative) or so large that the result of
  ‘**a** << **s**’ is guaranteed to be 0.
* ‘**a**’ is negative.
* ‘**a** << **s**’ sets the sign bit, if any, in ‘**\*d**’.

‘**\*d**’ will hold the results of the attempted shift, but is not
considered “safe for use” if true is returned.

overflows\_type

`overflows_type (n, T)`

> helper for checking the overflows between value, variables, or data type

**Parameters**

`n`
:   source constant value or variable to be checked

`T`
:   destination variable or data type proposed to store **x**

**Description**

Compares the **x** expression for whether or not it can safely fit in
the storage of the type in **T**. **x** and **T** can have different types.
If **x** is a constant expression, this will also resolve to a constant
expression.

**Return**

true if overflow can occur, false otherwise.

range\_overflows

`range_overflows (start, size, max)`

> Check if a range is out of bounds

**Parameters**

`start`
:   Start of the range.

`size`
:   Size of the range.

`max`
:   Exclusive upper boundary.

**Description**

A strict check to determine if the range [**start**, **start** + **size**) is
invalid with respect to the allowable range [0, **max**). Any range
starting at or beyond **max** is considered an overflow, even if **size** is 0.

**Return**

true if the range is out of bounds.

range\_overflows\_t

`range_overflows_t (type, start, size, max)`

> Check if a range is out of bounds

**Parameters**

`type`
:   Data type to use.

`start`
:   Start of the range.

`size`
:   Size of the range.

`max`
:   Exclusive upper boundary.

**Description**

Same as [`range_overflows()`](#c.range_overflows "range_overflows") but forcing the parameters to **type**.

**Return**

true if the range is out of bounds.

range\_end\_overflows

`range_end_overflows (start, size, max)`

> Check if a range’s endpoint is out of bounds

**Parameters**

`start`
:   Start of the range.

`size`
:   Size of the range.

`max`
:   Exclusive upper boundary.

**Description**

Checks only if the endpoint of a range (**start** + **size**) exceeds **max**.
Unlike [`range_overflows()`](#c.range_overflows "range_overflows"), a zero-sized range at the boundary (**start** == **max**)
is not considered an overflow. Useful for iterator-style checks.

**Return**

true if the endpoint exceeds the boundary.

range\_end\_overflows\_t

`range_end_overflows_t (type, start, size, max)`

> Check if a range’s endpoint is out of bounds

**Parameters**

`type`
:   Data type to use.

`start`
:   Start of the range.

`size`
:   Size of the range.

`max`
:   Exclusive upper boundary.

**Description**

Same as [`range_end_overflows()`](#c.range_end_overflows "range_end_overflows") but forcing the parameters to **type**.

**Return**

true if the endpoint exceeds the boundary.

castable\_to\_type

`castable_to_type (n, T)`

> like `__same_type()`, but also allows for casted literals

**Parameters**

`n`
:   variable or constant value

`T`
:   variable or data type

**Description**

Unlike the `__same_type()` macro, this allows a constant value as the
first argument. If this value would not overflow into an assignment
of the second argument’s type, it returns true. Otherwise, this falls
back to `__same_type()`.

size\_t size\_mul(size\_t factor1, size\_t factor2)
:   Calculate size\_t multiplication with saturation at SIZE\_MAX

**Parameters**

`size_t factor1`
:   first factor

`size_t factor2`
:   second factor

**Return**

calculate **factor1** \* **factor2**, both promoted to size\_t,
with any overflow causing the return value to be SIZE\_MAX. The
lvalue must be size\_t to avoid implicit type conversion.

size\_t size\_add(size\_t addend1, size\_t addend2)
:   Calculate size\_t addition with saturation at SIZE\_MAX

**Parameters**

`size_t addend1`
:   first addend

`size_t addend2`
:   second addend

**Return**

calculate **addend1** + **addend2**, both promoted to size\_t,
with any overflow causing the return value to be SIZE\_MAX. The
lvalue must be size\_t to avoid implicit type conversion.

size\_t size\_sub(size\_t minuend, size\_t subtrahend)
:   Calculate size\_t subtraction with saturation at SIZE\_MAX

**Parameters**

`size_t minuend`
:   value to subtract from

`size_t subtrahend`
:   value to subtract from **minuend**

**Return**

calculate **minuend** - **subtrahend**, both promoted to size\_t,
with any overflow causing the return value to be SIZE\_MAX. For
composition with the [`size_add()`](#c.size_add "size_add") and [`size_mul()`](#c.size_mul "size_mul") helpers, neither
argument may be SIZE\_MAX (or the result with be forced to SIZE\_MAX).
The lvalue must be size\_t to avoid implicit type conversion.

array\_size

`array_size (a, b)`

> Calculate size of 2-dimensional array.

**Parameters**

`a`
:   dimension one

`b`
:   dimension two

**Description**

Calculates size of 2-dimensional array: **a** \* **b**.

**Return**

number of bytes needed to represent the array or SIZE\_MAX on
overflow.

array3\_size

`array3_size (a, b, c)`

> Calculate size of 3-dimensional array.

**Parameters**

`a`
:   dimension one

`b`
:   dimension two

`c`
:   dimension three

**Description**

Calculates size of 3-dimensional array: **a** \* **b** \* **c**.

**Return**

number of bytes needed to represent the array or SIZE\_MAX on
overflow.

flex\_array\_size

`flex_array_size (p, member, count)`

> Calculate size of a flexible array member within an enclosing structure.

**Parameters**

`p`
:   Pointer to the structure.

`member`
:   Name of the flexible array member.

`count`
:   Number of elements in the array.

**Description**

Calculates size of a flexible array of **count** number of **member**
elements, at the end of structure **p**.

**Return**

number of bytes needed or SIZE\_MAX on overflow.

struct\_size

`struct_size (p, member, count)`

> Calculate size of structure with trailing flexible array.

**Parameters**

`p`
:   Pointer to the structure.

`member`
:   Name of the array member.

`count`
:   Number of elements in the array.

**Description**

Calculates size of memory needed for structure of **p** followed by an
array of **count** number of **member** elements.

**Return**

number of bytes needed or SIZE\_MAX on overflow.

struct\_size\_t

`struct_size_t (type, member, count)`

> Calculate size of structure with trailing flexible array

**Parameters**

`type`
:   structure type name.

`member`
:   Name of the array member.

`count`
:   Number of elements in the array.

**Description**

Calculates size of memory needed for structure **type** followed by an
array of **count** number of **member** elements. Prefer using [`struct_size()`](#c.struct_size "struct_size")
when possible instead, to keep calculations associated with a specific
instance variable of type **type**.

**Return**

number of bytes needed or SIZE\_MAX on overflow.

struct\_offset

`struct_offset (p, member)`

> Calculate the offset of a member within a struct

**Parameters**

`p`
:   Pointer to the struct

`member`
:   Name of the member to get the offset of

**Description**

Calculates the offset of a particular **member** of the structure pointed
to by **p**.

**Return**

number of bytes to the location of **member**.

\_\_DEFINE\_FLEX

`__DEFINE_FLEX (type, name, member, count, trailer...)`

> helper macro for [`DEFINE_FLEX()`](#c.DEFINE_FLEX "DEFINE_FLEX") family. Enables caller macro to pass arbitrary trailing expressions

**Parameters**

`type`
:   structure type name, including “struct” keyword.

`name`
:   Name for a variable to define.

`member`
:   Name of the array member.

`count`
:   Number of elements in the array; must be compile-time const.

`trailer...`
:   Trailing expressions for attributes and/or initializers.

\_DEFINE\_FLEX

`_DEFINE_FLEX (type, name, member, count, initializer...)`

> helper macro for [`DEFINE_FLEX()`](#c.DEFINE_FLEX "DEFINE_FLEX") family. Enables caller macro to pass (different) initializer.

**Parameters**

`type`
:   structure type name, including “struct” keyword.

`name`
:   Name for a variable to define.

`member`
:   Name of the array member.

`count`
:   Number of elements in the array; must be compile-time const.

`initializer...`
:   Initializer expression (e.g., pass = { } at minimum).

DEFINE\_RAW\_FLEX

`DEFINE_RAW_FLEX (type, name, member, count)`

> Define an on-stack instance of structure with a trailing flexible array member, when it does not have a \_\_counted\_by annotation.

**Parameters**

`type`
:   structure type name, including “struct” keyword.

`name`
:   Name for a variable to define.

`member`
:   Name of the array member.

`count`
:   Number of elements in the array; must be compile-time const.

**Description**

Define a zeroed, on-stack, instance of **type** structure with a trailing
flexible array member.
Use \_\_struct\_size(**name**) to get compile-time size of it afterwards.
Use \_\_member\_size(**name->member**) to get compile-time size of **name** members.
Use STACK\_FLEX\_ARRAY\_SIZE(**name**, **member**) to get compile-time number of
elements in array **member**.

DEFINE\_FLEX

`DEFINE_FLEX (TYPE, NAME, MEMBER, COUNTER, COUNT)`

> Define an on-stack instance of structure with a trailing flexible array member.

**Parameters**

`TYPE`
:   structure type name, including “struct” keyword.

`NAME`
:   Name for a variable to define.

`MEMBER`
:   Name of the array member.

`COUNTER`
:   Name of the \_\_counted\_by member.

`COUNT`
:   Number of elements in the array; must be compile-time const.

**Description**

Define a zeroed, on-stack, instance of **TYPE** structure with a trailing
flexible array member.
Use \_\_struct\_size(**NAME**) to get compile-time size of it afterwards.
Use \_\_member\_size(**NAME->member**) to get compile-time size of **NAME** members.
Use STACK\_FLEX\_ARRAY\_SIZE(**name**, **member**) to get compile-time number of
elements in array **member**.

STACK\_FLEX\_ARRAY\_SIZE

`STACK_FLEX_ARRAY_SIZE (name, array)`

> helper macro for [`DEFINE_FLEX()`](#c.DEFINE_FLEX "DEFINE_FLEX") family. Returns the number of elements in **array**.

**Parameters**

`name`
:   Name for a variable defined in [`DEFINE_RAW_FLEX()`](#c.DEFINE_RAW_FLEX "DEFINE_RAW_FLEX")/[`DEFINE_FLEX()`](#c.DEFINE_FLEX "DEFINE_FLEX").

`array`
:   Name of the array member.

typeof\_flex\_counter

`typeof_flex_counter (FAM)`

> Return the type of the counter variable of a given flexible array member annotated by `__counted_by()`.

**Parameters**

`FAM`
:   Instance of flexible array member within a given struct.

**Return**

“size\_t” if no annotation exists.

overflows\_flex\_counter\_type

`overflows_flex_counter_type (TYPE, FAM, COUNT)`

> Check if the counter associated with the given flexible array member can represent a value.

**Parameters**

`TYPE`
:   Type of the struct that contains the **FAM**.

`FAM`
:   Member name of the FAM within **TYPE**.

`COUNT`
:   Value to check against the \_\_counted\_by annotated **FAM**’s counter.

**Return**

true if **COUNT** can be represented in the **FAM**’s counter. When
**FAM** is not annotated with `__counted_by()`, always returns true.

\_\_set\_flex\_counter

`__set_flex_counter (FAM, COUNT)`

> Set the counter associated with the given flexible array member that has been annoated by `__counted_by()`.

**Parameters**

`FAM`
:   Instance of flexible array member within a given struct.

`COUNT`
:   Value to store to the \_\_counted\_by annotated **FAM\_PTR**’s counter.

**Description**

This is a no-op if no annotation exists. Count needs to be checked with
[`overflows_flex_counter_type()`](#c.overflows_flex_counter_type "overflows_flex_counter_type") before using this function.

### CRC Functions

uint8\_t crc4(uint8\_t c, uint64\_t x, int bits)
:   calculate the 4-bit crc of a value.

**Parameters**

`uint8_t c`
:   starting crc4

`uint64_t x`
:   value to checksum

`int bits`
:   number of bits in **x** to checksum

**Description**

Returns the crc4 value of **x**, using polynomial 0b10111.

The **x** value is treated as left-aligned, and bits above **bits** are ignored
in the crc calculations.

u8 crc7\_be(u8 crc, const u8 \*buffer, size\_t len)
:   update the CRC7 for the data buffer

**Parameters**

`u8 crc`
:   previous CRC7 value

`const u8 *buffer`
:   data pointer

`size_t len`
:   number of bytes in the buffer

**Context**

any

**Description**

Returns the updated CRC7 value.
The CRC7 is left-aligned in the byte (the lsbit is always 0), as that
makes the computation easier, and all callers want it in that form.

void crc8\_populate\_msb(u8 table[CRC8\_TABLE\_SIZE], u8 polynomial)
:   fill crc table for given polynomial in reverse bit order.

**Parameters**

`u8 table[CRC8_TABLE_SIZE]`
:   table to be filled.

`u8 polynomial`
:   polynomial for which table is to be filled.

void crc8\_populate\_lsb(u8 table[CRC8\_TABLE\_SIZE], u8 polynomial)
:   fill crc table for given polynomial in regular bit order.

**Parameters**

`u8 table[CRC8_TABLE_SIZE]`
:   table to be filled.

`u8 polynomial`
:   polynomial for which table is to be filled.

u8 crc8(const u8 table[CRC8\_TABLE\_SIZE], const u8 \*pdata, size\_t nbytes, u8 crc)
:   calculate a crc8 over the given input data.

**Parameters**

`const u8 table[CRC8_TABLE_SIZE]`
:   crc table used for calculation.

`const u8 *pdata`
:   pointer to data buffer.

`size_t nbytes`
:   number of bytes in data buffer.

`u8 crc`
:   previous returned crc8 value.

u16 crc16(u16 crc, const u8 \*p, size\_t len)
:   compute the CRC-16 for the data buffer

**Parameters**

`u16 crc`
:   previous CRC value

`const u8 *p`
:   data pointer

`size_t len`
:   number of bytes in the buffer

**Description**

Returns the updated CRC value.

u16 crc\_ccitt(u16 crc, u8 const \*buffer, size\_t len)
:   recompute the CRC (CRC-CCITT variant) for the data buffer

**Parameters**

`u16 crc`
:   previous CRC value

`u8 const *buffer`
:   data pointer

`size_t len`
:   number of bytes in the buffer

u16 crc\_itu\_t(u16 crc, const u8 \*buffer, size\_t len)
:   Compute the CRC-ITU-T for the data buffer

**Parameters**

`u16 crc`
:   previous CRC value

`const u8 *buffer`
:   data pointer

`size_t len`
:   number of bytes in the buffer

**Description**

Returns the updated CRC value

u32 crc32\_le(u32 crc, const void \*p, size\_t len)
:   Compute least-significant-bit-first IEEE CRC-32

**Parameters**

`u32 crc`
:   Initial CRC value. ~0 (recommended) or 0 for a new CRC computation, or
    the previous CRC value if computing incrementally.

`const void *p`
:   Pointer to the data buffer

`size_t len`
:   Length of data in bytes

**Description**

This implements the CRC variant that is often known as the IEEE CRC-32, or
simply CRC-32, and is widely used in Ethernet and other applications:

* Polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 +
  :   x^7 + x^5 + x^4 + x^2 + x^1 + x^0
* Bit order: Least-significant-bit-first
* Polynomial in integer form: 0xedb88320

This does *not* invert the CRC at the beginning or end. The caller is
expected to do that if it needs to. Inverting at both ends is recommended.

For new applications, prefer to use CRC-32C instead. See [`crc32c()`](#c.crc32c "crc32c").

**Context**

Any context

**Return**

The new CRC value

u32 crc32\_be(u32 crc, const void \*p, size\_t len)
:   Compute most-significant-bit-first IEEE CRC-32

**Parameters**

`u32 crc`
:   Initial CRC value. ~0 (recommended) or 0 for a new CRC computation, or
    the previous CRC value if computing incrementally.

`const void *p`
:   Pointer to the data buffer

`size_t len`
:   Length of data in bytes

**Description**

[`crc32_be()`](#c.crc32_be "crc32_be") is the same as [`crc32_le()`](#c.crc32_le "crc32_le") except that [`crc32_be()`](#c.crc32_be "crc32_be") computes the
*most-significant-bit-first* variant of the CRC. I.e., within each byte, the
most significant bit is processed first (treated as highest order polynomial
coefficient). The same bit order is also used for the CRC value itself:

* Polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 +
  :   x^7 + x^5 + x^4 + x^2 + x^1 + x^0
* Bit order: Most-significant-bit-first
* Polynomial in integer form: 0x04c11db7

**Context**

Any context

**Return**

The new CRC value

u32 crc32c(u32 crc, const void \*p, size\_t len)
:   Compute CRC-32C

**Parameters**

`u32 crc`
:   Initial CRC value. ~0 (recommended) or 0 for a new CRC computation, or
    the previous CRC value if computing incrementally.

`const void *p`
:   Pointer to the data buffer

`size_t len`
:   Length of data in bytes

**Description**

This implements CRC-32C, i.e. the Castagnoli CRC. This is the recommended
CRC variant to use in new applications that want a 32-bit CRC.

* Polynomial: x^32 + x^28 + x^27 + x^26 + x^25 + x^23 + x^22 + x^20 + x^19 +
  :   x^18 + x^14 + x^13 + x^11 + x^10 + x^9 + x^8 + x^6 + x^0
* Bit order: Least-significant-bit-first
* Polynomial in integer form: 0x82f63b78

This does *not* invert the CRC at the beginning or end. The caller is
expected to do that if it needs to. Inverting at both ends is recommended.

**Context**

Any context

**Return**

The new CRC value

u64 crc64\_be(u64 crc, const void \*p, size\_t len)
:   Calculate bitwise big-endian ECMA-182 CRC64

**Parameters**

`u64 crc`
:   seed value for computation. 0 or (u64)~0 for a new CRC calculation,
    or the previous crc64 value if computing incrementally.

`const void *p`
:   pointer to buffer over which CRC64 is run

`size_t len`
:   length of buffer **p**

u64 crc64\_nvme(u64 crc, const void \*p, size\_t len)
:   Calculate CRC64-NVME

**Parameters**

`u64 crc`
:   seed value for computation. 0 for a new CRC calculation, or the
    previous crc64 value if computing incrementally.

`const void *p`
:   pointer to buffer over which CRC64 is run

`size_t len`
:   length of buffer **p**

**Description**

This computes the CRC64 defined in the NVME NVM Command Set Specification,
*including the bitwise inversion at the beginning and end*.

### Base 2 log and power Functions

bool is\_power\_of\_2(unsigned long n)
:   check if a value is a power of two

**Parameters**

`unsigned long n`
:   the value to check

**Description**

Determine whether some value is a power of two, where zero is
*not* considered a power of two.

**Return**

true if **n** is a power of 2, otherwise false.

unsigned long \_\_roundup\_pow\_of\_two(unsigned long n)
:   round up to nearest power of two

**Parameters**

`unsigned long n`
:   value to round up

unsigned long \_\_rounddown\_pow\_of\_two(unsigned long n)
:   round down to nearest power of two

**Parameters**

`unsigned long n`
:   value to round down

const\_ilog2

`const_ilog2 (n)`

> log base 2 of 32-bit or a 64-bit constant unsigned value

**Parameters**

`n`
:   parameter

**Description**

Use this where sparse expects a true constant expression, e.g. for array
indices.

ilog2

`ilog2 (n)`

> log base 2 of 32-bit or a 64-bit unsigned value

**Parameters**

`n`
:   parameter

**Description**

constant-capable log of base 2 calculation
- this can be used to initialise global variables from constant data, hence
the massive ternary operator construction

selects the appropriately-sized optimised version depending on sizeof(n)

roundup\_pow\_of\_two

`roundup_pow_of_two (n)`

> round the given value up to nearest power of two

**Parameters**

`n`
:   parameter

**Description**

round the given value up to the nearest power of two
- the result is undefined when n == 0
- this can be used to initialise global variables from constant data

rounddown\_pow\_of\_two

`rounddown_pow_of_two (n)`

> round the given value down to nearest power of two

**Parameters**

`n`
:   parameter

**Description**

round the given value down to the nearest power of two
- the result is undefined when n == 0
- this can be used to initialise global variables from constant data

order\_base\_2

`order_base_2 (n)`

> calculate the (rounded up) base 2 order of the argument

**Parameters**

`n`
:   parameter

**Description**

The first few values calculated by this routine:
:   ob2(0) = 0
    ob2(1) = 0
    ob2(2) = 1
    ob2(3) = 2
    ob2(4) = 2
    ob2(5) = 3
    ... and so on.

bits\_per

`bits_per (n)`

> calculate the number of bits required for the argument

**Parameters**

`n`
:   parameter

**Description**

This is constant-capable and can be used for compile time
initializations, e.g bitfields.

The first few values calculated by this routine:
bf(0) = 1
bf(1) = 1
bf(2) = 2
bf(3) = 2
bf(4) = 3
... and so on.

unsigned int max\_pow\_of\_two\_factor(unsigned int n)
:   return highest power-of-2 factor

**Parameters**

`unsigned int n`
:   parameter

**Description**

find highest power-of-2 which is evenly divisible into n.
0 is returned for n == 0 or 1.

### Integer log and power Functions

unsigned int intlog2(u32 value)
:   computes log2 of a value; the result is shifted left by 24 bits

**Parameters**

`u32 value`
:   The value (must be != 0)

**Description**

to use rational values you can use the following method:

> intlog2(value) = intlog2(value \* 2^x) - x \* 2^24

Some usecase examples:

> intlog2(8) will give 3 << 24 = 3 \* 2^24
>
> intlog2(9) will give 3 << 24 + ... = 3.16... \* 2^24
>
> intlog2(1.5) = intlog2(3) - 2^24 = 0.584... \* 2^24

**Return**

log2(value) \* 2^24

unsigned int intlog10(u32 value)
:   computes log10 of a value; the result is shifted left by 24 bits

**Parameters**

`u32 value`
:   The value (must be != 0)

**Description**

to use rational values you can use the following method:

> intlog10(value) = intlog10(value \* 10^x) - x \* 2^24

An usecase example:

> > intlog10(1000) will give 3 << 24 = 3 \* 2^24
>
> due to the implementation intlog10(1000) might be not exactly 3 \* 2^24

look at intlog2 for similar examples

**Return**

log10(value) \* 2^24

u64 int\_pow(u64 base, unsigned int exp)
:   computes the exponentiation of the given base and exponent

**Parameters**

`u64 base`
:   base which will be raised to the given power

`unsigned int exp`
:   power to be raised to

**Description**

Computes: pow(base, exp), i.e. **base** raised to the **exp** power

unsigned long int\_sqrt(unsigned long x)
:   computes the integer square root

**Parameters**

`unsigned long x`
:   integer of which to calculate the sqrt

**Description**

Computes: floor(sqrt(x))

u32 int\_sqrt64(u64 x)
:   strongly typed int\_sqrt function when minimum 64 bit input is expected.

**Parameters**

`u64 x`
:   64bit integer of which to calculate the sqrt

### Division Functions

do\_div

`do_div (n, base)`

> returns 2 values: calculate remainder and update new dividend

**Parameters**

`n`
:   uint64\_t dividend (will be updated)

`base`
:   uint32\_t divisor

**Description**

Summary:
`uint32_t remainder = n % base;`
`n = n / base;`

**Return**

(uint32\_t)remainder

**NOTE**

macro parameter **n** is evaluated multiple times,
beware of side effects!

u64 div\_u64\_rem(u64 dividend, u32 divisor, u32 \*remainder)
:   unsigned 64bit divide with 32bit divisor with remainder

**Parameters**

`u64 dividend`
:   unsigned 64bit dividend

`u32 divisor`
:   unsigned 32bit divisor

`u32 *remainder`
:   pointer to unsigned 32bit remainder

**Return**

sets `*remainder`, then returns dividend / divisor

**Description**

This is commonly provided by 32bit archs to provide an optimized 64bit
divide.

s64 div\_s64\_rem(s64 dividend, s32 divisor, s32 \*remainder)
:   signed 64bit divide with 32bit divisor with remainder

**Parameters**

`s64 dividend`
:   signed 64bit dividend

`s32 divisor`
:   signed 32bit divisor

`s32 *remainder`
:   pointer to signed 32bit remainder

**Return**

sets `*remainder`, then returns dividend / divisor

u64 div64\_u64\_rem(u64 dividend, u64 divisor, u64 \*remainder)
:   unsigned 64bit divide with 64bit divisor and remainder

**Parameters**

`u64 dividend`
:   unsigned 64bit dividend

`u64 divisor`
:   unsigned 64bit divisor

`u64 *remainder`
:   pointer to unsigned 64bit remainder

**Return**

sets `*remainder`, then returns dividend / divisor

u64 div64\_u64(u64 dividend, u64 divisor)
:   unsigned 64bit divide with 64bit divisor

**Parameters**

`u64 dividend`
:   unsigned 64bit dividend

`u64 divisor`
:   unsigned 64bit divisor

**Return**

dividend / divisor

s64 div64\_s64(s64 dividend, s64 divisor)
:   signed 64bit divide with 64bit divisor

**Parameters**

`s64 dividend`
:   signed 64bit dividend

`s64 divisor`
:   signed 64bit divisor

**Return**

dividend / divisor

u64 div\_u64(u64 dividend, u32 divisor)
:   unsigned 64bit divide with 32bit divisor

**Parameters**

`u64 dividend`
:   unsigned 64bit dividend

`u32 divisor`
:   unsigned 32bit divisor

**Description**

This is the most common 64bit divide and should be used if possible,
as many 32bit archs can optimize this variant better than a full 64bit
divide.

**Return**

dividend / divisor

s64 div\_s64(s64 dividend, s32 divisor)
:   signed 64bit divide with 32bit divisor

**Parameters**

`s64 dividend`
:   signed 64bit dividend

`s32 divisor`
:   signed 32bit divisor

**Return**

dividend / divisor

u64 mul\_u64\_add\_u64\_div\_u64(u64 a, u64 b, u64 c, u64 d)
:   unsigned 64bit multiply, add, and divide

**Parameters**

`u64 a`
:   first unsigned 64bit multiplicand

`u64 b`
:   second unsigned 64bit multiplicand

`u64 c`
:   unsigned 64bit addend

`u64 d`
:   unsigned 64bit divisor

**Description**

Multiply two 64bit values together to generate a 128bit product
add a third value and then divide by a fourth.
The Generic code divides by 0 if **d** is zero and returns ~0 on overflow.
Architecture specific code may trap on zero or overflow.

**Return**

(**a** \* **b** + **c**) / **d**

mul\_u64\_u64\_div\_u64

`mul_u64_u64_div_u64 (a, b, d)`

> unsigned 64bit multiply and divide

**Parameters**

`a`
:   first unsigned 64bit multiplicand

`b`
:   second unsigned 64bit multiplicand

`d`
:   unsigned 64bit divisor

**Description**

Multiply two 64bit values together to generate a 128bit product
and then divide by a third value.
The Generic code divides by 0 if **d** is zero and returns ~0 on overflow.
Architecture specific code may trap on zero or overflow.

**Return**

**a** \* **b** / **d**

mul\_u64\_u64\_div\_u64\_roundup

`mul_u64_u64_div_u64_roundup (a, b, d)`

> unsigned 64bit multiply and divide rounded up

**Parameters**

`a`
:   first unsigned 64bit multiplicand

`b`
:   second unsigned 64bit multiplicand

`d`
:   unsigned 64bit divisor

**Description**

Multiply two 64bit values together to generate a 128bit product
and then divide and round up.
The Generic code divides by 0 if **d** is zero and returns ~0 on overflow.
Architecture specific code may trap on zero or overflow.

**Return**

(**a** \* **b** + **d** - 1) / **d**

DIV64\_U64\_ROUND\_UP

`DIV64_U64_ROUND_UP (ll, d)`

> unsigned 64bit divide with 64bit divisor rounded up

**Parameters**

`ll`
:   unsigned 64bit dividend

`d`
:   unsigned 64bit divisor

**Description**

Divide unsigned 64bit dividend by unsigned 64bit divisor
and round up.

**Return**

dividend / divisor rounded up

DIV\_U64\_ROUND\_UP

`DIV_U64_ROUND_UP (ll, d)`

> unsigned 64bit divide with 32bit divisor rounded up

**Parameters**

`ll`
:   unsigned 64bit dividend

`d`
:   unsigned 32bit divisor

**Description**

Divide unsigned 64bit dividend by unsigned 32bit divisor
and round up.

**Return**

dividend / divisor rounded up

DIV64\_U64\_ROUND\_CLOSEST

`DIV64_U64_ROUND_CLOSEST (dividend, divisor)`

> unsigned 64bit divide with 64bit divisor rounded to nearest integer

**Parameters**

`dividend`
:   unsigned 64bit dividend

`divisor`
:   unsigned 64bit divisor

**Description**

Divide unsigned 64bit dividend by unsigned 64bit divisor
and round to closest integer.

**Return**

dividend / divisor rounded to nearest integer

DIV\_U64\_ROUND\_CLOSEST

`DIV_U64_ROUND_CLOSEST (dividend, divisor)`

> unsigned 64bit divide with 32bit divisor rounded to nearest integer

**Parameters**

`dividend`
:   unsigned 64bit dividend

`divisor`
:   unsigned 32bit divisor

**Description**

Divide unsigned 64bit dividend by unsigned 32bit divisor
and round to closest integer.

**Return**

dividend / divisor rounded to nearest integer

DIV\_S64\_ROUND\_CLOSEST

`DIV_S64_ROUND_CLOSEST (dividend, divisor)`

> signed 64bit divide with 32bit divisor rounded to nearest integer

**Parameters**

`dividend`
:   signed 64bit dividend

`divisor`
:   signed 32bit divisor

**Description**

Divide signed 64bit dividend by signed 32bit divisor
and round to closest integer.

**Return**

dividend / divisor rounded to nearest integer

u64 roundup\_u64(u64 x, u32 y)
:   Round up a 64bit value to the next specified 32bit multiple

**Parameters**

`u64 x`
:   the value to up

`u32 y`
:   32bit multiple to round up to

**Description**

Rounds **x** to the next multiple of **y**. For 32bit **x** values, see roundup and
the faster `round_up()` for powers of 2.

**Return**

rounded up value.

unsigned long gcd(unsigned long a, unsigned long b)
:   calculate and return the greatest common divisor of 2 unsigned longs

**Parameters**

`unsigned long a`
:   first value

`unsigned long b`
:   second value

### UUID/GUID

void generate\_random\_uuid(unsigned char uuid[16])
:   generate a random UUID

**Parameters**

`unsigned char uuid[16]`
:   where to put the generated UUID

**Description**

Random UUID interface

Used to create a Boot ID or a filesystem UUID/GUID, but can be
useful for other kernel drivers.

bool uuid\_is\_valid(const char \*uuid)
:   checks if a UUID string is valid

**Parameters**

`const char *uuid`
:   UUID string to check

**Description**

It checks if the UUID string is following the format:
:   xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

where x is a hex digit.

**Return**

true if input is valid UUID string.

## Kernel IPC facilities

### IPC utilities

int ipc\_init(void)
:   initialise ipc subsystem

**Parameters**

`void`
:   no arguments

**Description**

The various sysv ipc resources (semaphores, messages and shared
memory) are initialised.

A callback routine is registered into the memory hotplug notifier
chain: since msgmni scales to lowmem this callback routine will be
called upon successful memory add / remove to recompute msmgni.

void ipc\_init\_ids(struct ipc\_ids \*ids)
:   initialise ipc identifiers

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

**Description**

Set up the sequence range to use for the ipc identifier range (limited
below ipc\_mni) then initialise the keys hashtable and ids idr.

void ipc\_init\_proc\_interface(const char \*path, const char \*header, int ids, int (\*show)(struct seq\_file\*, void\*))
:   create a proc interface for sysipc types using a seq\_file interface.

**Parameters**

`const char *path`
:   Path in procfs

`const char *header`
:   Banner to be printed at the beginning of the file.

`int ids`
:   ipc id table to iterate.

`int (*show)(struct seq_file *, void *)`
:   show routine.

struct kern\_ipc\_perm \*ipc\_findkey(struct ipc\_ids \*ids, key\_t key)
:   find a key in an ipc identifier set

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`key_t key`
:   key to find

**Description**

Returns the locked pointer to the ipc structure if found or NULL
otherwise. If key is found ipc points to the owning ipc structure

Called with writer ipc\_ids.rwsem held.

int ipc\_addid(struct ipc\_ids \*ids, struct kern\_ipc\_perm \*new, int limit)
:   add an ipc identifier

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`struct kern_ipc_perm *new`
:   new ipc permission set

`int limit`
:   limit for the number of used ids

**Description**

Add an entry ‘new’ to the ipc ids idr. The permissions object is
initialised and the first free entry is set up and the index assigned
is returned. The ‘new’ entry is returned in a locked state on success.

On failure the entry is not locked and a negative err-code is returned.
The caller must use `ipc_rcu_putref()` to free the identifier.

Called with writer ipc\_ids.rwsem held.

int ipcget\_new(struct ipc\_namespace \*ns, struct ipc\_ids \*ids, const struct ipc\_ops \*ops, struct ipc\_params \*params)
:   create a new ipc object

**Parameters**

`struct ipc_namespace *ns`
:   ipc namespace

`struct ipc_ids *ids`
:   ipc identifier set

`const struct ipc_ops *ops`
:   the actual creation routine to call

`struct ipc_params *params`
:   its parameters

**Description**

This routine is called by sys\_msgget, `sys_semget()` and `sys_shmget()`
when the key is IPC\_PRIVATE.

int ipc\_check\_perms(struct ipc\_namespace \*ns, struct kern\_ipc\_perm \*ipcp, const struct ipc\_ops \*ops, struct ipc\_params \*params)
:   check security and permissions for an ipc object

**Parameters**

`struct ipc_namespace *ns`
:   ipc namespace

`struct kern_ipc_perm *ipcp`
:   ipc permission set

`const struct ipc_ops *ops`
:   the actual security routine to call

`struct ipc_params *params`
:   its parameters

**Description**

This routine is called by `sys_msgget()`, `sys_semget()` and `sys_shmget()`
when the key is not IPC\_PRIVATE and that key already exists in the
ds IDR.

On success, the ipc id is returned.

It is called with ipc\_ids.rwsem and ipcp->lock held.

int ipcget\_public(struct ipc\_namespace \*ns, struct ipc\_ids \*ids, const struct ipc\_ops \*ops, struct ipc\_params \*params)
:   get an ipc object or create a new one

**Parameters**

`struct ipc_namespace *ns`
:   ipc namespace

`struct ipc_ids *ids`
:   ipc identifier set

`const struct ipc_ops *ops`
:   the actual creation routine to call

`struct ipc_params *params`
:   its parameters

**Description**

This routine is called by sys\_msgget, `sys_semget()` and `sys_shmget()`
when the key is not IPC\_PRIVATE.
It adds a new entry if the key is not found and does some permission
/ security checkings if the key is found.

On success, the ipc id is returned.

void ipc\_kht\_remove(struct ipc\_ids \*ids, struct kern\_ipc\_perm \*ipcp)
:   remove an ipc from the key hashtable

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`struct kern_ipc_perm *ipcp`
:   ipc perm structure containing the key to remove

**Description**

ipc\_ids.rwsem (as a writer) and the spinlock for this ID are held
before this function is called, and remain locked on the exit.

int ipc\_search\_maxidx(struct ipc\_ids \*ids, int limit)
:   search for the highest assigned index

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`int limit`
:   known upper limit for highest assigned index

**Description**

The function determines the highest assigned index in **ids**. It is intended
to be called when ids->max\_idx needs to be updated.
Updating ids->max\_idx is necessary when the current highest index ipc
object is deleted.
If no ipc object is allocated, then -1 is returned.

ipc\_ids.rwsem needs to be held by the caller.

void ipc\_rmid(struct ipc\_ids \*ids, struct kern\_ipc\_perm \*ipcp)
:   remove an ipc identifier

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`struct kern_ipc_perm *ipcp`
:   ipc perm structure containing the identifier to remove

**Description**

ipc\_ids.rwsem (as a writer) and the spinlock for this ID are held
before this function is called, and remain locked on the exit.

void ipc\_set\_key\_private(struct ipc\_ids \*ids, struct kern\_ipc\_perm \*ipcp)
:   switch the key of an existing ipc to IPC\_PRIVATE

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`struct kern_ipc_perm *ipcp`
:   ipc perm structure containing the key to modify

**Description**

ipc\_ids.rwsem (as a writer) and the spinlock for this ID are held
before this function is called, and remain locked on the exit.

int ipcperms(struct ipc\_namespace \*ns, struct kern\_ipc\_perm \*ipcp, short flag)
:   check ipc permissions

**Parameters**

`struct ipc_namespace *ns`
:   ipc namespace

`struct kern_ipc_perm *ipcp`
:   ipc permission set

`short flag`
:   desired permission set

**Description**

Check user, group, other permissions for access
to ipc resources. return 0 if allowed

**flag** will most probably be 0 or `S_...UGO` from <linux/stat.h>

void kernel\_to\_ipc64\_perm(struct kern\_ipc\_perm \*in, struct ipc64\_perm \*out)
:   convert kernel ipc permissions to user

**Parameters**

`struct kern_ipc_perm *in`
:   kernel permissions

`struct ipc64_perm *out`
:   new style ipc permissions

**Description**

Turn the kernel object **in** into a set of permissions descriptions
for returning to userspace (**out**).

void ipc64\_perm\_to\_ipc\_perm(struct ipc64\_perm \*in, struct ipc\_perm \*out)
:   convert new ipc permissions to old

**Parameters**

`struct ipc64_perm *in`
:   new style ipc permissions

`struct ipc_perm *out`
:   old style ipc permissions

**Description**

Turn the new style permissions object **in** into a compatibility
object and store it into the **out** pointer.

struct kern\_ipc\_perm \*ipc\_obtain\_object\_idr(struct ipc\_ids \*ids, int id)
:   Look for an id in the ipc ids idr and return associated ipc object.

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`int id`
:   ipc id to look for

**Description**

Call inside the RCU critical section.
The ipc object is *not* locked on exit.

struct kern\_ipc\_perm \*ipc\_obtain\_object\_check(struct ipc\_ids \*ids, int id)
:   Similar to [`ipc_obtain_object_idr()`](#c.ipc_obtain_object_idr "ipc_obtain_object_idr") but also checks the ipc object sequence number.

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`int id`
:   ipc id to look for

**Description**

Call inside the RCU critical section.
The ipc object is *not* locked on exit.

int ipcget(struct ipc\_namespace \*ns, struct ipc\_ids \*ids, const struct ipc\_ops \*ops, struct ipc\_params \*params)
:   Common sys\_\*`get()` code

**Parameters**

`struct ipc_namespace *ns`
:   namespace

`struct ipc_ids *ids`
:   ipc identifier set

`const struct ipc_ops *ops`
:   operations to be called on ipc object creation, permission checks
    and further checks

`struct ipc_params *params`
:   the parameters needed by the previous operations.

**Description**

Common routine called by `sys_msgget()`, `sys_semget()` and `sys_shmget()`.

int ipc\_update\_perm(struct ipc64\_perm \*in, struct kern\_ipc\_perm \*out)
:   update the permissions of an ipc object

**Parameters**

`struct ipc64_perm *in`
:   the permission given as input.

`struct kern_ipc_perm *out`
:   the permission of the ipc to set.

struct kern\_ipc\_perm \*ipcctl\_obtain\_check(struct ipc\_namespace \*ns, struct ipc\_ids \*ids, int id, int cmd, struct ipc64\_perm \*perm, int extra\_perm)
:   retrieve an ipc object and check permissions

**Parameters**

`struct ipc_namespace *ns`
:   ipc namespace

`struct ipc_ids *ids`
:   the table of ids where to look for the ipc

`int id`
:   the id of the ipc to retrieve

`int cmd`
:   the cmd to check

`struct ipc64_perm *perm`
:   the permission to set

`int extra_perm`
:   one extra permission parameter used by msq

**Description**

This function does some common audit and permissions check for some IPC\_XXX
cmd and is called from semctl\_down, shmctl\_down and msgctl\_down.

It:
:   * retrieves the ipc object with the given id in the given table.
    * performs some audit and permission check, depending on the given cmd
    * returns a pointer to the ipc object or otherwise, the corresponding
      error.

Call holding the both the rwsem and the rcu read lock.

int ipc\_parse\_version(int \*cmd)
:   ipc call version

**Parameters**

`int *cmd`
:   pointer to command

**Description**

Return IPC\_64 for new style IPC and IPC\_OLD for old style IPC.
The **cmd** value is turned from an encoding command and version into
just the command code.

struct kern\_ipc\_perm \*sysvipc\_find\_ipc(struct ipc\_ids \*ids, loff\_t \*pos)
:   Find and lock the ipc structure based on seq pos

**Parameters**

`struct ipc_ids *ids`
:   ipc identifier set

`loff_t *pos`
:   expected position

**Description**

The function finds an ipc structure, based on the sequence file
position **pos**. If there is no ipc structure at position **pos**, then
the successor is selected.
If a structure is found, then it is locked (both [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and
`ipc_lock_object()`) and **pos** is set to the position needed to locate
the found ipc structure.
If nothing is found (i.e. EOF), **pos** is not modified.

The function returns the found ipc structure, or NULL at EOF.

## FIFO Buffer

### kfifo interface

DECLARE\_KFIFO\_PTR

`DECLARE_KFIFO_PTR (fifo, type)`

> macro to declare a fifo pointer object

**Parameters**

`fifo`
:   name of the declared fifo

`type`
:   type of the fifo elements

DECLARE\_KFIFO

`DECLARE_KFIFO (fifo, type, size)`

> macro to declare a fifo object

**Parameters**

`fifo`
:   name of the declared fifo

`type`
:   type of the fifo elements

`size`
:   the number of elements in the fifo, this must be a power of 2

INIT\_KFIFO

`INIT_KFIFO (fifo)`

> Initialize a fifo declared by DECLARE\_KFIFO

**Parameters**

`fifo`
:   name of the declared fifo datatype

DEFINE\_KFIFO

`DEFINE_KFIFO (fifo, type, size)`

> macro to define and initialize a fifo

**Parameters**

`fifo`
:   name of the declared fifo datatype

`type`
:   type of the fifo elements

`size`
:   the number of elements in the fifo, this must be a power of 2

**Note**

the macro can be used for global and local fifo data type variables.

kfifo\_initialized

`kfifo_initialized (fifo)`

> Check if the fifo is initialized

**Parameters**

`fifo`
:   address of the fifo to check

**Description**

Return `true` if fifo is initialized, otherwise `false`.
Assumes the fifo was 0 before.

kfifo\_esize

`kfifo_esize (fifo)`

> returns the size of the element managed by the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_recsize

`kfifo_recsize (fifo)`

> returns the size of the record length field

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_size

`kfifo_size (fifo)`

> returns the size of the fifo in elements

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_reset

`kfifo_reset (fifo)`

> removes the entire fifo content

**Parameters**

`fifo`
:   address of the fifo to be used

**Note**

usage of [`kfifo_reset()`](#c.kfifo_reset "kfifo_reset") is dangerous. It should be only called when the
fifo is exclusived locked or when it is secured that no other thread is
accessing the fifo.

kfifo\_reset\_out

`kfifo_reset_out (fifo)`

> skip fifo content

**Parameters**

`fifo`
:   address of the fifo to be used

**Note**

The usage of [`kfifo_reset_out()`](#c.kfifo_reset_out "kfifo_reset_out") is safe until it will be only called
from the reader thread and there is only one concurrent reader. Otherwise
it is dangerous and must be handled in the same way as [`kfifo_reset()`](#c.kfifo_reset "kfifo_reset").

kfifo\_len

`kfifo_len (fifo)`

> returns the number of used elements in the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_is\_empty

`kfifo_is_empty (fifo)`

> returns true if the fifo is empty

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_is\_empty\_spinlocked

`kfifo_is_empty_spinlocked (fifo, lock)`

> returns true if the fifo is empty using a spinlock for locking

**Parameters**

`fifo`
:   address of the fifo to be used

`lock`
:   spinlock to be used for locking

kfifo\_is\_empty\_spinlocked\_noirqsave

`kfifo_is_empty_spinlocked_noirqsave (fifo, lock)`

> returns true if the fifo is empty using a spinlock for locking, doesn’t disable interrupts

**Parameters**

`fifo`
:   address of the fifo to be used

`lock`
:   spinlock to be used for locking

kfifo\_is\_full

`kfifo_is_full (fifo)`

> returns true if the fifo is full

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_avail

`kfifo_avail (fifo)`

> returns the number of unused elements in the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_skip\_count

`kfifo_skip_count (fifo, count)`

> skip output data

**Parameters**

`fifo`
:   address of the fifo to be used

`count`
:   count of data to skip

kfifo\_skip

`kfifo_skip (fifo)`

> skip output data

**Parameters**

`fifo`
:   address of the fifo to be used

kfifo\_peek\_len

`kfifo_peek_len (fifo)`

> gets the size of the next fifo record

**Parameters**

`fifo`
:   address of the fifo to be used

**Description**

This function returns the size of the next fifo record in number of bytes.

kfifo\_alloc

`kfifo_alloc (fifo, size, gfp_mask)`

> dynamically allocates a new fifo buffer

**Parameters**

`fifo`
:   pointer to the fifo

`size`
:   the number of elements in the fifo, this must be a power of 2

`gfp_mask`
:   get\_free\_pages mask, passed to [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc")

**Description**

This macro dynamically allocates a new fifo buffer.

The number of elements will be rounded-up to a power of 2.
The fifo will be release with [`kfifo_free()`](#c.kfifo_free "kfifo_free").
Return 0 if no error, otherwise an error code.

kfifo\_alloc\_node

`kfifo_alloc_node (fifo, size, gfp_mask, node)`

> dynamically allocates a new fifo buffer on a NUMA node

**Parameters**

`fifo`
:   pointer to the fifo

`size`
:   the number of elements in the fifo, this must be a power of 2

`gfp_mask`
:   get\_free\_pages mask, passed to [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc")

`node`
:   NUMA node to allocate memory on

**Description**

This macro dynamically allocates a new fifo buffer with NUMA node awareness.

The number of elements will be rounded-up to a power of 2.
The fifo will be release with [`kfifo_free()`](#c.kfifo_free "kfifo_free").
Return 0 if no error, otherwise an error code.

kfifo\_free

`kfifo_free (fifo)`

> frees the fifo

**Parameters**

`fifo`
:   the fifo to be freed

kfifo\_init

`kfifo_init (fifo, buffer, size)`

> initialize a fifo using a preallocated buffer

**Parameters**

`fifo`
:   the fifo to assign the buffer

`buffer`
:   the preallocated buffer to be used

`size`
:   the size of the internal buffer, this have to be a power of 2

**Description**

This macro initializes a fifo using a preallocated buffer.

The number of elements will be rounded-up to a power of 2.
Return 0 if no error, otherwise an error code.

kfifo\_put

`kfifo_put (fifo, val)`

> put data into the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

`val`
:   the data to be added

**Description**

This macro copies the given value into the fifo.
It returns 0 if the fifo was full. Otherwise it returns the number
processed elements.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_get

`kfifo_get (fifo, val)`

> get data from the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

`val`
:   address where to store the data

**Description**

This macro reads the data from the fifo.
It returns 0 if the fifo was empty. Otherwise it returns the number
processed elements.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_peek

`kfifo_peek (fifo, val)`

> get data from the fifo without removing

**Parameters**

`fifo`
:   address of the fifo to be used

`val`
:   address where to store the data

**Description**

This reads the data from the fifo without removing it from the fifo.
It returns 0 if the fifo was empty. Otherwise it returns the number
processed elements.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_in

`kfifo_in (fifo, buf, n)`

> put data into the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   the data to be added

`n`
:   number of elements to be added

**Description**

This macro copies the given buffer into the fifo and returns the
number of copied elements.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_in\_spinlocked

`kfifo_in_spinlocked (fifo, buf, n, lock)`

> put data into the fifo using a spinlock for locking

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   the data to be added

`n`
:   number of elements to be added

`lock`
:   pointer to the spinlock to use for locking

**Description**

This macro copies the given values buffer into the fifo and returns the
number of copied elements.

kfifo\_in\_spinlocked\_noirqsave

`kfifo_in_spinlocked_noirqsave (fifo, buf, n, lock)`

> put data into fifo using a spinlock for locking, don’t disable interrupts

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   the data to be added

`n`
:   number of elements to be added

`lock`
:   pointer to the spinlock to use for locking

**Description**

This is a variant of [`kfifo_in_spinlocked()`](#c.kfifo_in_spinlocked "kfifo_in_spinlocked") but uses spin\_lock/`unlock()`
for locking and doesn’t disable interrupts.

kfifo\_out

`kfifo_out (fifo, buf, n)`

> get data from the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   pointer to the storage buffer

`n`
:   max. number of elements to get

**Description**

This macro gets some data from the fifo and returns the numbers of elements
copied.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_out\_spinlocked

`kfifo_out_spinlocked (fifo, buf, n, lock)`

> get data from the fifo using a spinlock for locking

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   pointer to the storage buffer

`n`
:   max. number of elements to get

`lock`
:   pointer to the spinlock to use for locking

**Description**

This macro gets the data from the fifo and returns the numbers of elements
copied.

kfifo\_out\_spinlocked\_noirqsave

`kfifo_out_spinlocked_noirqsave (fifo, buf, n, lock)`

> get data from the fifo using a spinlock for locking, don’t disable interrupts

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   pointer to the storage buffer

`n`
:   max. number of elements to get

`lock`
:   pointer to the spinlock to use for locking

**Description**

This is a variant of [`kfifo_out_spinlocked()`](#c.kfifo_out_spinlocked "kfifo_out_spinlocked") which uses spin\_lock/`unlock()`
for locking and doesn’t disable interrupts.

kfifo\_from\_user

`kfifo_from_user (fifo, from, len, copied)`

> puts some data from user space into the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

`from`
:   pointer to the data to be added

`len`
:   the length of the data to be added

`copied`
:   pointer to output variable to store the number of copied bytes

**Description**

This macro copies at most **len** bytes from the **from** into the
fifo, depending of the available space and returns -EFAULT/0.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_to\_user

`kfifo_to_user (fifo, to, len, copied)`

> copies data from the fifo into user space

**Parameters**

`fifo`
:   address of the fifo to be used

`to`
:   where the data must be copied

`len`
:   the size of the destination buffer

`copied`
:   pointer to output variable to store the number of copied bytes

**Description**

This macro copies at most **len** bytes from the fifo into the
**to** buffer and returns -EFAULT/0.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_dma\_in\_prepare\_mapped

`kfifo_dma_in_prepare_mapped (fifo, sgl, nents, len, dma)`

> setup a scatterlist for DMA input

**Parameters**

`fifo`
:   address of the fifo to be used

`sgl`
:   pointer to the scatterlist array

`nents`
:   number of entries in the scatterlist array

`len`
:   number of elements to transfer

`dma`
:   mapped dma address to fill into **sgl**

**Description**

This macro fills a scatterlist for DMA input.
It returns the number entries in the scatterlist array.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macros.

kfifo\_dma\_in\_finish

`kfifo_dma_in_finish (fifo, len)`

> finish a DMA IN operation

**Parameters**

`fifo`
:   address of the fifo to be used

`len`
:   number of bytes to received

**Description**

This macro finishes a DMA IN operation. The in counter will be updated by
the len parameter. No error checking will be done.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macros.

kfifo\_dma\_out\_prepare\_mapped

`kfifo_dma_out_prepare_mapped (fifo, sgl, nents, len, dma)`

> setup a scatterlist for DMA output

**Parameters**

`fifo`
:   address of the fifo to be used

`sgl`
:   pointer to the scatterlist array

`nents`
:   number of entries in the scatterlist array

`len`
:   number of elements to transfer

`dma`
:   mapped dma address to fill into **sgl**

**Description**

This macro fills a scatterlist for DMA output which at most **len** bytes
to transfer.
It returns the number entries in the scatterlist array.
A zero means there is no space available and the scatterlist is not filled.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macros.

kfifo\_dma\_out\_finish

`kfifo_dma_out_finish (fifo, len)`

> finish a DMA OUT operation

**Parameters**

`fifo`
:   address of the fifo to be used

`len`
:   number of bytes transferred

**Description**

This macro finishes a DMA OUT operation. The out counter will be updated by
the len parameter. No error checking will be done.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macros.

kfifo\_out\_peek

`kfifo_out_peek (fifo, buf, n)`

> gets some data from the fifo

**Parameters**

`fifo`
:   address of the fifo to be used

`buf`
:   pointer to the storage buffer

`n`
:   max. number of elements to get

**Description**

This macro gets the data from the fifo and returns the numbers of elements
copied. The data is not removed from the fifo.

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_out\_linear

`kfifo_out_linear (fifo, tail, n)`

> gets a tail of/offset to available data

**Parameters**

`fifo`
:   address of the fifo to be used

`tail`
:   pointer to an unsigned int to store the value of tail

`n`
:   max. number of elements to point at

**Description**

This macro obtains the offset (tail) to the available data in the fifo
buffer and returns the
numbers of elements available. It returns the available count till the end
of data or till the end of the buffer. So that it can be used for linear
data processing (like [`memcpy()`](#c.memcpy "memcpy") of (**fifo->data** + **tail**) with count
returned).

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

kfifo\_out\_linear\_ptr

`kfifo_out_linear_ptr (fifo, ptr, n)`

> gets a pointer to the available data

**Parameters**

`fifo`
:   address of the fifo to be used

`ptr`
:   pointer to data to store the pointer to tail

`n`
:   max. number of elements to point at

**Description**

Similarly to [`kfifo_out_linear()`](#c.kfifo_out_linear "kfifo_out_linear"), this macro obtains the pointer to the
available data in the fifo buffer and returns the numbers of elements
available. It returns the available count till the end of available data or
till the end of the buffer. So that it can be used for linear data
processing (like [`memcpy()`](#c.memcpy "memcpy") of **ptr** with count returned).

Note that with only one concurrent reader and one concurrent
writer, you don’t need extra locking to use these macro.

## relay interface support

Relay interface support is designed to provide an efficient mechanism
for tools and facilities to relay large amounts of data from kernel
space to user space.

### relay interface

int relay\_buf\_full(struct rchan\_buf \*buf)
:   boolean, is the channel buffer full?

**Parameters**

`struct rchan_buf *buf`
:   channel buffer

**Description**

> Returns 1 if the buffer is full, 0 otherwise.

void relay\_reset(struct rchan \*chan)
:   reset the channel

**Parameters**

`struct rchan *chan`
:   the channel

**Description**

> This has the effect of erasing all data from all channel buffers
> and restarting the channel in its initial state. The buffers
> are not freed, so any mappings are still in effect.
>
> NOTE. Care should be taken that the channel isn’t actually
> being used by anything when this call is made.

struct rchan \*relay\_open(const char \*base\_filename, struct dentry \*parent, size\_t subbuf\_size, size\_t n\_subbufs, const struct rchan\_callbacks \*cb, void \*private\_data)
:   create a new relay channel

**Parameters**

`const char *base_filename`
:   base name of files to create

`struct dentry *parent`
:   dentry of parent directory, `NULL` for root directory or buffer

`size_t subbuf_size`
:   size of sub-buffers

`size_t n_subbufs`
:   number of sub-buffers

`const struct rchan_callbacks *cb`
:   client callback functions

`void *private_data`
:   user-defined data

**Description**

> Returns channel pointer if successful, `NULL` otherwise.
>
> Creates a channel buffer for each cpu using the sizes and
> attributes specified. The created channel buffer files
> will be named base\_filename0...base\_filenameN-1. File
> permissions will be `S_IRUSR`.

size\_t relay\_switch\_subbuf(struct rchan\_buf \*buf, size\_t length)
:   switch to a new sub-buffer

**Parameters**

`struct rchan_buf *buf`
:   channel buffer

`size_t length`
:   size of current event

**Description**

> Returns either the length passed in or 0 if full.
>
> Performs sub-buffer-switch tasks such as invoking callbacks,
> updating padding counts, waking up readers, etc.

void relay\_subbufs\_consumed(struct rchan \*chan, unsigned int cpu, size\_t subbufs\_consumed)
:   update the buffer’s sub-buffers-consumed count

**Parameters**

`struct rchan *chan`
:   the channel

`unsigned int cpu`
:   the cpu associated with the channel buffer to update

`size_t subbufs_consumed`
:   number of sub-buffers to add to current buf’s count

**Description**

> Adds to the channel buffer’s consumed sub-buffer count.
> subbufs\_consumed should be the number of sub-buffers newly consumed,
> not the total consumed.
>
> NOTE. Kernel clients don’t need to call this function if the channel
> mode is ‘overwrite’.

void relay\_close(struct rchan \*chan)
:   close the channel

**Parameters**

`struct rchan *chan`
:   the channel

**Description**

> Closes all channel buffers and frees the channel.

void relay\_flush(struct rchan \*chan)
:   close the channel

**Parameters**

`struct rchan *chan`
:   the channel

**Description**

> Flushes all channel buffers, i.e. forces buffer switch.

int relay\_mmap\_prepare\_buf(struct rchan\_buf \*buf, struct vm\_area\_desc \*desc)
:   * mmap channel buffer to process address space

**Parameters**

`struct rchan_buf *buf`
:   the relay channel buffer

`struct vm_area_desc *desc`
:   describing what to map

**Description**

> Returns 0 if ok, negative on error
>
> Caller should already have grabbed mmap\_lock.

void \*relay\_alloc\_buf(struct rchan\_buf \*buf, size\_t \*size)
:   allocate a channel buffer

**Parameters**

`struct rchan_buf *buf`
:   the buffer struct

`size_t *size`
:   total size of the buffer

**Description**

> Returns a pointer to the resulting buffer, `NULL` if unsuccessful. The
> passed in size will get page aligned, if it isn’t already.

struct rchan\_buf \*relay\_create\_buf(struct rchan \*chan)
:   allocate and initialize a channel buffer

**Parameters**

`struct rchan *chan`
:   the relay channel

**Description**

> Returns channel buffer if successful, `NULL` otherwise.

void relay\_destroy\_channel(struct [kref](#c.relay_destroy_channel "kref") \*kref)
:   free the channel struct

**Parameters**

`struct kref *kref`
:   target kernel reference that contains the relay channel

**Description**

> Should only be called from [`kref_put()`](kref.html#c.kref_put "kref_put").

void relay\_destroy\_buf(struct rchan\_buf \*buf)
:   destroy an rchan\_buf `struct and` associated buffer

**Parameters**

`struct rchan_buf *buf`
:   the buffer struct

void relay\_remove\_buf(struct [kref](#c.relay_remove_buf "kref") \*kref)
:   remove a channel buffer

**Parameters**

`struct kref *kref`
:   target kernel reference that contains the relay buffer

**Description**

> Removes the file from the filesystem, which also frees the
> rchan\_buf\_struct and the channel buffer. Should only be called from
> [`kref_put()`](kref.html#c.kref_put "kref_put").

int relay\_buf\_empty(struct rchan\_buf \*buf)
:   boolean, is the channel buffer empty?

**Parameters**

`struct rchan_buf *buf`
:   channel buffer

**Description**

> Returns 1 if the buffer is empty, 0 otherwise.

void wakeup\_readers(struct irq\_work \*work)
:   wake up readers waiting on a channel

**Parameters**

`struct irq_work *work`
:   contains the channel buffer

**Description**

> This is the function used to defer reader waking

void \_\_relay\_reset(struct rchan\_buf \*buf, unsigned int init)
:   reset a channel buffer

**Parameters**

`struct rchan_buf *buf`
:   the channel buffer

`unsigned int init`
:   1 if this is a first-time initialization

**Description**

> See [`relay_reset()`](#c.relay_reset "relay_reset") for description of effect.

void relay\_close\_buf(struct rchan\_buf \*buf)
:   close a channel buffer

**Parameters**

`struct rchan_buf *buf`
:   channel buffer

**Description**

> Marks the buffer finalized and restores the default callbacks.
> The channel buffer and channel buffer data structure are then freed
> automatically when the last reference is given up.

size\_t relay\_stats(struct rchan \*chan, int flags)
:   get channel buffer statistics

**Parameters**

`struct rchan *chan`
:   the channel

`int flags`
:   select particular information to get

**Description**

> Returns the count of certain field that caller specifies.

int relay\_file\_open(struct [inode](#c.relay_file_open "inode") \*inode, struct [file](../filesystems/api-summary.html#c.file "file") \*filp)
:   open file op for relay files

**Parameters**

`struct inode *inode`
:   the inode

`struct file *filp`
:   the file

**Description**

> Increments the channel buffer refcount.

int relay\_file\_mmap\_prepare(struct vm\_area\_desc \*desc)
:   mmap file op for relay files

**Parameters**

`struct vm_area_desc *desc`
:   describing what to map

**Description**

> Calls upon [`relay_mmap_prepare_buf()`](#c.relay_mmap_prepare_buf "relay_mmap_prepare_buf") to map the file into user space.

\_\_poll\_t relay\_file\_poll(struct [file](../filesystems/api-summary.html#c.file "file") \*filp, poll\_table \*wait)
:   poll file op for relay files

**Parameters**

`struct file *filp`
:   the file

`poll_table *wait`
:   poll table

**Description**

> Poll implemention.

int relay\_file\_release(struct [inode](#c.relay_file_release "inode") \*inode, struct [file](../filesystems/api-summary.html#c.file "file") \*filp)
:   release file op for relay files

**Parameters**

`struct inode *inode`
:   the inode

`struct file *filp`
:   the file

**Description**

> Decrements the channel refcount, as the filesystem is
> no longer using it.

size\_t relay\_file\_read\_subbuf\_avail(size\_t read\_pos, struct rchan\_buf \*buf)
:   return bytes available in sub-buffer

**Parameters**

`size_t read_pos`
:   file read position

`struct rchan_buf *buf`
:   relay channel buffer

size\_t relay\_file\_read\_start\_pos(struct rchan\_buf \*buf)
:   find the first available byte to read

**Parameters**

`struct rchan_buf *buf`
:   relay channel buffer

**Description**

> If the read\_pos is in the middle of padding, return the
> position of the first actually available byte, otherwise
> return the original value.

size\_t relay\_file\_read\_end\_pos(struct rchan\_buf \*buf, size\_t read\_pos, size\_t count)
:   return the new read position

**Parameters**

`struct rchan_buf *buf`
:   relay channel buffer

`size_t read_pos`
:   file read position

`size_t count`
:   number of bytes to be read

## Module Support

### Kernel module auto-loading

int \_\_request\_module(bool wait, const char \*fmt, ...)
:   try to load a kernel module

**Parameters**

`bool wait`
:   wait (or not) for the operation to complete

`const char *fmt`
:   printf style format string for the name of the module

`...`
:   arguments as specified in the format string

**Description**

Load a module using the user mode module loader. The function returns
zero on success or a negative errno code or positive exit code from
“modprobe” on failure. Note that a successful module load does not mean
the module did not then unload and exit on an error of its own. Callers
must check that the service they requested is now available not blindly
invoke it.

If module auto-loading support is disabled then this function
simply returns -ENOENT.

### Module debugging

Enabling CONFIG\_MODULE\_STATS enables module debugging statistics which
are useful to monitor and root cause memory pressure issues with module
loading. These statistics are useful to allow us to improve production
workloads.

The current module debugging statistics supported help keep track of module
loading failures to enable improvements either for kernel module auto-loading
usage (`request_module()`) or interactions with userspace. Statistics are
provided to track all possible failures in the `finit_module()` path and memory
wasted in this process space. Each of the failure counters are associated
to a type of module loading failure which is known to incur a certain amount
of memory allocation loss. In the worst case loading a module will fail after
a 3 step memory allocation process:

> 1. memory allocated with `kernel_read_file_from_fd()`
> 2. module decompression processes the file read from
>    `kernel_read_file_from_fd()`, and [`vmap()`](mm-api.html#c.vmap "vmap") is used to map
>    the decompressed module to a new local buffer which represents
>    a copy of the decompressed module passed from userspace. The buffer
>    from `kernel_read_file_from_fd()` is freed right away.
> 3. `layout_and_allocate()` allocates space for the final resting
>    place where we would keep the module if it were to be processed
>    successfully.

If a failure occurs after these three different allocations only one
counter will be incremented with the summation of the allocated bytes freed
incurred during this failure. Likewise, if module loading failed only after
step b) a separate counter is used and incremented for the bytes freed and
not used during both of those allocations.

Virtual memory space can be limited, for example on x86 virtual memory size
defaults to 128 MiB. We should strive to limit and avoid wasting virtual
memory allocations when possible. These module debugging statistics help
to evaluate how much memory is being wasted on bootup due to module loading
failures.

All counters are designed to be incremental. Atomic counters are used so to
remain simple and avoid delays and deadlocks.

#### dup\_failed\_modules - tracks duplicate failed modules

Linked list of modules which failed to be loaded because an already existing
module with the same name was already being processed or already loaded.
The `finit_module()` system call incurs heavy virtual memory allocations. In
the worst case an `finit_module()` system call can end up allocating virtual
memory 3 times:

> 1. `kernel_read_file_from_fd()` call uses [`vmalloc()`](mm-api.html#c.vmalloc "vmalloc")
> 2. optional module decompression uses [`vmap()`](mm-api.html#c.vmap "vmap")
> 3. layout\_and `allocate()` can use [`vzalloc()`](mm-api.html#c.vzalloc "vzalloc") or an arch specific variation of
>    vmalloc to deal with ELF sections requiring special permissions

In practice on a typical boot today most `finit_module()` calls fail due to
the module with the same name already being loaded or about to be processed.
All virtual memory allocated to these failed modules will be freed with
no functional use.

To help with this the dup\_failed\_modules allows us to track modules which
failed to load due to the fact that a module was already loaded or being
processed. There are only two points at which we can fail such calls,
we list them below along with the number of virtual memory allocation
calls:

> 1. FAIL\_DUP\_MOD\_BECOMING: at the end of `early_mod_check()` before
>    `layout_and_allocate()`.
>    - with module decompression: 2 virtual memory allocation calls
>    - without module decompression: 1 virtual memory allocation calls
> 2. FAIL\_DUP\_MOD\_LOAD: after `layout_and_allocate()` on `add_unformed_module()`
>    - with module decompression 3 virtual memory allocation calls
>    - without module decompression 2 virtual memory allocation calls

We should strive to get this list to be as small as possible. If this list
is not empty it is a reflection of possible work or optimizations possible
either in-kernel or in userspace.

#### module statistics debugfs counters

The total amount of wasted virtual memory allocation space during module
loading can be computed by adding the total from the summation:

> * **invalid\_kread\_bytes** +
>   **invalid\_decompress\_bytes** +
>   **invalid\_becoming\_bytes** +
>   **invalid\_mod\_bytes**

The following debugfs counters are available to inspect module loading
failures:

> > * total\_mod\_size: total bytes ever used by all modules we’ve dealt with on
> >   this system
> > * total\_text\_size: total bytes of the .text and .init.text ELF section
> >   sizes we’ve dealt with on this system
> > * invalid\_kread\_bytes: bytes allocated and then freed on failures which
> >   happen due to the initial `kernel_read_file_from_fd()`. `kernel_read_file_from_fd()`
> >   uses [`vmalloc()`](mm-api.html#c.vmalloc "vmalloc"). These should typically not happen unless your system is
> >   under memory pressure.
> > * invalid\_decompress\_bytes: number of bytes allocated and freed due to
> >   memory allocations in the module decompression path that use [`vmap()`](mm-api.html#c.vmap "vmap").
> >   These typically should not happen unless your system is under memory
> >   pressure.
> > * invalid\_becoming\_bytes: total number of bytes allocated and freed used
> >   to read the kernel module userspace wants us to read before we
> >   promote it to be processed to be added to our **modules** linked list. These
> >   failures can happen if we had a check in between a successful `kernel_read_file_from_fd()`
> >   call and right before we allocate the our private memory for the module
> >   which would be kept if the module is successfully loaded. The most common
> >   reason for this failure is when userspace is racing to load a module
> >   which it does not yet see loaded. The first module to succeed in
> >   `add_unformed_module()` will add a module to our `modules` list and
> >   subsequent loads of modules with the same name will error out at the
> >   end of `early_mod_check()`. The check for `module_patient_check_exists()`
> >   at the end of `early_mod_check()` prevents duplicate allocations
> >   on `layout_and_allocate()` for modules already being processed. These
> >   duplicate failed modules are non-fatal, however they typically are
> >   indicative of userspace not seeing a module in userspace loaded yet and
> >   unnecessarily trying to load a module before the kernel even has a chance
> >   to begin to process prior requests. Although duplicate failures can be
> >   non-fatal, we should try to reduce [`vmalloc()`](mm-api.html#c.vmalloc "vmalloc") pressure proactively, so
> >   ideally after boot this will be close to as 0 as possible. If module
> >   decompression was used we also add to this counter the cost of the
> >   initial `kernel_read_file_from_fd()` of the compressed module. If module
> >   decompression was not used the value represents the total allocated and
> >   freed bytes in `kernel_read_file_from_fd()` calls for these type of
> >   failures. These failures can occur because:
> >
> > > * `module_sig_check()` - module signature checks
> > > * `elf_validity_cache_copy()` - some ELF validation issue
> > > * `early_mod_check()`:
> > >
> > >   + blacklisting
> > >   + failed to rewrite section headers
> > >   + version magic
> > >   + live patch requirements didn’t check out
> > >   + the module was detected as being already present
> >
> > * invalid\_mod\_bytes: these are the total number of bytes allocated and
> >   freed due to failures after we did all the sanity checks of the module
> >   which userspace passed to us and after our first check that the module
> >   is unique. A module can still fail to load if we detect the module is
> >   loaded after we allocate space for it with `layout_and_allocate()`, we do
> >   this check right before processing the module as live and run its
> >   initialization routines. Note that you have a failure of this type it
> >   also means the respective `kernel_read_file_from_fd()` memory space was
> >   also freed and not used, and so we increment this counter with twice
> >   the size of the module. Additionally if you used module decompression
> >   the size of the compressed module is also added to this counter.
>
> * modcount: how many modules we’ve loaded in our kernel life time
> * failed\_kreads: how many modules failed due to failed `kernel_read_file_from_fd()`
> * failed\_decompress: how many failed module decompression attempts we’ve had.
>   These really should not happen unless your compression / decompression
>   might be broken.
> * failed\_becoming: how many modules failed after we `kernel_read_file_from_fd()`
>   it and before we allocate memory for it with `layout_and_allocate()`. This
>   counter is never incremented if you manage to validate the module and
>   call `layout_and_allocate()` for it.
> * failed\_load\_modules: how many modules failed once we’ve allocated our
>   private space for our module using `layout_and_allocate()`. These failures
>   should hopefully mostly be dealt with already. Races in theory could
>   still exist here, but it would just mean the kernel had started processing
>   two threads concurrently up to `early_mod_check()` and one thread won.
>   These failures are good signs the kernel or userspace is doing something
>   seriously stupid or that could be improved. We should strive to fix these,
>   but it is perhaps not easy to fix them. A recent example are the modules
>   requests incurred for frequency modules, a separate module request was
>   being issued for each CPU on a system.

### Inter Module support

Refer to the files in kernel/module/ for more information.

## Hardware Interfaces

### DMA Channels

int request\_dma(unsigned int dmanr, const char \*device\_id)
:   request and reserve a system DMA channel

**Parameters**

`unsigned int dmanr`
:   DMA channel number

`const char * device_id`
:   reserving device ID string, used in /proc/dma

void free\_dma(unsigned int dmanr)
:   free a reserved system DMA channel

**Parameters**

`unsigned int dmanr`
:   DMA channel number

### Resources Management

struct resource \*request\_resource\_conflict(struct resource \*root, struct resource \*new)
:   request and reserve an I/O or memory resource

**Parameters**

`struct resource *root`
:   root resource descriptor

`struct resource *new`
:   resource descriptor desired by caller

**Description**

Returns 0 for success, conflict resource on error.

int find\_next\_res(struct resource \*parent, resource\_size\_t start, resource\_size\_t end, unsigned long flags, unsigned long desc, struct resource \*res)
:   Finds the lowest resource that covers part of [**start**..\*\*end\*\*].

**Parameters**

`struct resource *parent`
:   resource tree root to search

`resource_size_t start`
:   start address of the resource searched for

`resource_size_t end`
:   end address of same resource

`unsigned long flags`
:   flags which the resource must have

`unsigned long desc`
:   descriptor the resource must have

`struct resource *res`
:   return ptr, if resource found

**Description**

If a resource is found, returns 0 and **\*\*\*res is overwritten with the part
of the resource that’s within [\*\*start**..\*\*end\*\*]; if none is found, returns
-ENODEV. Returns -EINVAL for invalid parameters.

The caller must specify **start**, **end**, **flags**, and **desc**
(which may be IORES\_DESC\_NONE).

int reallocate\_resource(struct resource \*root, struct resource \*old, resource\_size\_t newsize, struct resource\_constraint \*constraint)
:   allocate a slot in the resource tree given range & alignment. The resource will be relocated if the new size cannot be reallocated in the current location.

**Parameters**

`struct resource *root`
:   root resource descriptor

`struct resource *old`
:   resource descriptor desired by caller

`resource_size_t newsize`
:   new size of the resource descriptor

`struct resource_constraint *constraint`
:   the memory range and alignment constraints to be met.

struct resource \*lookup\_resource(struct resource \*root, resource\_size\_t start)
:   find an existing resource by a resource start address

**Parameters**

`struct resource *root`
:   root resource descriptor

`resource_size_t start`
:   resource start address

**Description**

Returns a pointer to the resource if found, NULL otherwise

struct resource \*insert\_resource\_conflict(struct resource \*parent, struct resource \*new)
:   Inserts resource in the resource tree

**Parameters**

`struct resource *parent`
:   parent of the new resource

`struct resource *new`
:   new resource to insert

**Description**

Returns 0 on success, conflict resource if the resource can’t be inserted.

This function is equivalent to request\_resource\_conflict when no conflict
happens. If a conflict happens, and the conflicting resources
entirely fit within the range of the new resource, then the new
resource is inserted and the conflicting resources become children of
the new resource.

This function is intended for producers of resources, such as FW modules
and bus drivers.

resource\_size\_t resource\_alignment(struct resource \*res)
:   calculate resource’s alignment

**Parameters**

`struct resource *res`
:   resource pointer

**Description**

Returns alignment on success, 0 (invalid alignment) on failure.

void release\_mem\_region\_adjustable(resource\_size\_t start, resource\_size\_t size)
:   release a previously reserved memory region

**Parameters**

`resource_size_t start`
:   resource start address

`resource_size_t size`
:   resource region size

**Description**

This interface is intended for memory hot-delete. The requested region
is released from a currently busy memory resource. The requested region
must either match exactly or fit into a single busy resource entry. In
the latter case, the remaining resource is adjusted accordingly.

**Note**

* Additional release conditions, such as overlapping region, can be
  supported after they are confirmed as valid cases.
* When a busy memory resource gets split into two entries, its children are
  reassigned to the correct parent based on their range. If a child memory
  resource overlaps with more than one parent, enhance the logic as needed.

void merge\_system\_ram\_resource(struct resource \*res)
:   mark the System RAM resource mergeable and try to merge it with adjacent, mergeable resources

**Parameters**

`struct resource *res`
:   resource descriptor

**Description**

This interface is intended for memory hotplug, whereby lots of contiguous
system ram resources are added (e.g., via add\_memory\*()) by a driver, and
the actual resource boundaries are not of interest (e.g., it might be
relevant for DIMMs). Only resources that are marked mergeable, that have the
same parent, and that don’t have any children are considered. All mergeable
resources must be immutable during the request.

**Note**

* The caller has to make sure that no pointers to resources that are
  marked mergeable are used anymore after this call - the resource might
  be freed and the pointer might be stale!
* [`release_mem_region_adjustable()`](#c.release_mem_region_adjustable "release_mem_region_adjustable") will split on demand on memory hotunplug

int request\_resource(struct resource \*root, struct resource \*new)
:   request and reserve an I/O or memory resource

**Parameters**

`struct resource *root`
:   root resource descriptor

`struct resource *new`
:   resource descriptor desired by caller

**Description**

Returns 0 for success, negative error code on error.

int release\_resource(struct resource \*old)
:   release a previously reserved resource

**Parameters**

`struct resource *old`
:   resource pointer

int walk\_iomem\_res\_desc(unsigned long desc, unsigned long flags, u64 start, u64 end, void \*arg, int (\*func)(struct resource\*, void\*))
:   Walks through iomem resources and calls `func()` with matching resource ranges. \*

**Parameters**

`unsigned long desc`
:   I/O resource descriptor. Use IORES\_DESC\_NONE to skip **desc** check.

`unsigned long flags`
:   I/O resource flags

`u64 start`
:   start addr

`u64 end`
:   end addr

`void *arg`
:   function argument for the callback **func**

`int (*func)(struct resource *, void *)`
:   callback function that is called for each qualifying resource area

**Description**

All the memory ranges which overlap start,end and also match flags and
desc are valid candidates.

**NOTE**

For a new descriptor search, define a new IORES\_DESC in
<linux/ioport.h> and set it in ‘desc’ of a target resource entry.

int region\_intersects(resource\_size\_t start, size\_t size, unsigned long flags, unsigned long desc)
:   determine intersection of region with known resources

**Parameters**

`resource_size_t start`
:   region start address

`size_t size`
:   size of region

`unsigned long flags`
:   flags of resource (in iomem\_resource)

`unsigned long desc`
:   descriptor of resource (in iomem\_resource) or IORES\_DESC\_NONE

**Description**

Check if the specified region partially overlaps or fully eclipses a
resource identified by **flags** and **desc** (optional with IORES\_DESC\_NONE).
Return REGION\_DISJOINT if the region does not overlap **flags**/**desc**,
return REGION\_MIXED if the region overlaps **flags**/**desc** and another
resource, and return REGION\_INTERSECTS if the region overlaps **flags**/**desc**
and no other defined resource. Note that REGION\_INTERSECTS is also
returned in the case when the specified region overlaps RAM and undefined
memory holes.

`region_intersect()` is used by memory remapping functions to ensure
the user is not remapping RAM and is a vast speed up over walking
through the resource table page by page.

int find\_resource\_space(struct resource \*root, struct resource \*new, resource\_size\_t size, struct resource\_constraint \*constraint)
:   Find empty space in the resource tree

**Parameters**

`struct resource *root`
:   Root resource descriptor

`struct resource *new`
:   Resource descriptor awaiting an empty resource space

`resource_size_t size`
:   The minimum size of the empty space

`struct resource_constraint *constraint`
:   The range and alignment constraints to be met

**Description**

Finds an empty space under **root** in the resource tree satisfying range and
alignment **constraints**.

**Return**

* `0` - if successful, **new** members start, end, and flags are altered.
* `-EBUSY` - if no empty space was found.

int allocate\_resource(struct resource \*root, struct resource \*new, resource\_size\_t size, resource\_size\_t min, resource\_size\_t max, resource\_size\_t align, resource\_alignf alignf, void \*alignf\_data)
:   allocate empty slot in the resource tree given range & alignment. The resource will be reallocated with a new size if it was already allocated

**Parameters**

`struct resource *root`
:   root resource descriptor

`struct resource *new`
:   resource descriptor desired by caller

`resource_size_t size`
:   requested resource region size

`resource_size_t min`
:   minimum boundary to allocate

`resource_size_t max`
:   maximum boundary to allocate

`resource_size_t align`
:   alignment requested, in bytes

`resource_alignf alignf`
:   alignment function, optional, called if not NULL

`void *alignf_data`
:   arbitrary data to pass to the **alignf** function

int insert\_resource(struct resource \*parent, struct resource \*new)
:   Inserts a resource in the resource tree

**Parameters**

`struct resource *parent`
:   parent of the new resource

`struct resource *new`
:   new resource to insert

**Description**

Returns 0 on success, -EBUSY if the resource can’t be inserted.

This function is intended for producers of resources, such as FW modules
and bus drivers.

void insert\_resource\_expand\_to\_fit(struct resource \*root, struct resource \*new)
:   Insert a resource into the resource tree

**Parameters**

`struct resource *root`
:   root resource descriptor

`struct resource *new`
:   new resource to insert

**Description**

Insert a resource into the resource tree, possibly expanding it in order
to make it encompass any conflicting resources.

int remove\_resource(struct resource \*old)
:   Remove a resource in the resource tree

**Parameters**

`struct resource *old`
:   resource to remove

**Description**

Returns 0 on success, -EINVAL if the resource is not valid.

This function removes a resource previously inserted by [`insert_resource()`](#c.insert_resource "insert_resource")
or [`insert_resource_conflict()`](#c.insert_resource_conflict "insert_resource_conflict"), and moves the children (if any) up to
where they were before. [`insert_resource()`](#c.insert_resource "insert_resource") and [`insert_resource_conflict()`](#c.insert_resource_conflict "insert_resource_conflict")
insert a new resource, and move any conflicting resources down to the
children of the new resource.

[`insert_resource()`](#c.insert_resource "insert_resource"), [`insert_resource_conflict()`](#c.insert_resource_conflict "insert_resource_conflict") and [`remove_resource()`](#c.remove_resource "remove_resource") are
intended for producers of resources, such as FW modules and bus drivers.

int adjust\_resource(struct resource \*res, resource\_size\_t start, resource\_size\_t size)
:   modify a resource’s start and size

**Parameters**

`struct resource *res`
:   resource to modify

`resource_size_t start`
:   new start value

`resource_size_t size`
:   new size

**Description**

Given an existing resource, change its start and size to match the
arguments. Returns 0 on success, -EBUSY if it can’t fit.
Existing children of the resource are assumed to be immutable.

struct resource \*\_\_request\_region(struct resource \*parent, resource\_size\_t start, resource\_size\_t n, const char \*name, int flags)
:   create a new busy resource region

**Parameters**

`struct resource *parent`
:   parent resource descriptor

`resource_size_t start`
:   resource start address

`resource_size_t n`
:   resource region size

`const char *name`
:   reserving caller’s ID string

`int flags`
:   IO resource flags

void \_\_release\_region(struct resource \*parent, resource\_size\_t start, resource\_size\_t n)
:   release a previously reserved resource region

**Parameters**

`struct resource *parent`
:   parent resource descriptor

`resource_size_t start`
:   resource start address

`resource_size_t n`
:   resource region size

**Description**

The described resource region must match a currently busy region.

int devm\_request\_resource(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct resource \*root, struct resource \*new)
:   request and reserve an I/O or memory resource

**Parameters**

`struct device *dev`
:   device for which to request the resource

`struct resource *root`
:   root of the resource tree from which to request the resource

`struct resource *new`
:   descriptor of the resource to request

**Description**

This is a device-managed version of [`request_resource()`](#c.request_resource "request_resource"). There is usually
no need to release resources requested by this function explicitly since
that will be taken care of when the device is unbound from its driver.
If for some reason the resource needs to be released explicitly, because
of ordering issues for example, drivers must call [`devm_release_resource()`](#c.devm_release_resource "devm_release_resource")
rather than the regular [`release_resource()`](#c.release_resource "release_resource").

When a conflict is detected between any existing resources and the newly
requested resource, an error message will be printed.

Returns 0 on success or a negative error code on failure.

void devm\_release\_resource(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct resource \*new)
:   release a previously requested resource

**Parameters**

`struct device *dev`
:   device for which to release the resource

`struct resource *new`
:   descriptor of the resource to release

**Description**

Releases a resource previously requested using [`devm_request_resource()`](#c.devm_request_resource "devm_request_resource").

struct resource \*devm\_request\_free\_mem\_region(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct resource \*base, unsigned long size)
:   find free region for device private memory

**Parameters**

`struct device *dev`
:   device `struct to` bind the resource to

`struct resource *base`
:   resource tree to look in

`unsigned long size`
:   size in bytes of the device memory to add

**Description**

This function tries to find an empty range of physical address big enough to
contain the new resource, so that it can later be hotplugged as ZONE\_DEVICE
memory, which in turn allocates `struct pages`.

struct resource \*alloc\_free\_mem\_region(struct resource \*base, unsigned long size, unsigned long align, const char \*name)
:   find a free region relative to **base**

**Parameters**

`struct resource *base`
:   resource that will parent the new resource

`unsigned long size`
:   size in bytes of memory to allocate from **base**

`unsigned long align`
:   alignment requirements for the allocation

`const char *name`
:   resource name

**Description**

Buses like CXL, that can dynamically instantiate new memory regions,
need a method to allocate physical address space for those regions.
Allocate and insert a new resource to cover a free, unclaimed by a
descendant of **base**, range in the span of **base**.

### MTRR Handling

int arch\_phys\_wc\_add(unsigned long base, unsigned long size)
:   add a WC MTRR and handle errors if PAT is unavailable

**Parameters**

`unsigned long base`
:   Physical base address

`unsigned long size`
:   Size of region

**Description**

If PAT is available, this does nothing. If PAT is unavailable, it
attempts to add a WC MTRR covering size bytes starting at base and
logs an error if this fails.

The called should provide a power of two size on an equivalent
power of two boundary.

Drivers must store the return value to pass to mtrr\_del\_wc\_if\_needed,
but drivers should not try to interpret that return value.

## Security Framework

int lsm\_file\_alloc(struct [file](#c.lsm_file_alloc "file") \*file)
:   allocate a composite file blob

**Parameters**

`struct file *file`
:   the file that needs a blob

**Description**

Allocate the file blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_backing\_file\_alloc(struct [file](../filesystems/api-summary.html#c.file "file") \*backing\_file)
:   allocate a composite backing file blob

**Parameters**

`struct file *backing_file`
:   the backing file

**Description**

Allocate the backing file blob for all the modules.

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_blob\_alloc(void \*\*dest, size\_t size, gfp\_t gfp)
:   allocate a composite blob

**Parameters**

`void **dest`
:   the destination for the blob

`size_t size`
:   the size of the blob

`gfp_t gfp`
:   allocation type

**Description**

Allocate a blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_cred\_alloc(struct [cred](#c.lsm_cred_alloc "cred") \*cred, gfp\_t gfp)
:   allocate a composite cred blob

**Parameters**

`struct cred *cred`
:   the cred that needs a blob

`gfp_t gfp`
:   allocation type

**Description**

Allocate the cred blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_inode\_alloc(struct [inode](#c.lsm_inode_alloc "inode") \*inode, gfp\_t gfp)
:   allocate a composite inode blob

**Parameters**

`struct inode *inode`
:   the inode that needs a blob

`gfp_t gfp`
:   allocation flags

**Description**

Allocate the inode blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_task\_alloc(struct task\_struct \*task)
:   allocate a composite task blob

**Parameters**

`struct task_struct *task`
:   the task that needs a blob

**Description**

Allocate the task blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_ipc\_alloc(struct kern\_ipc\_perm \*kip)
:   allocate a composite ipc blob

**Parameters**

`struct kern_ipc_perm *kip`
:   the ipc that needs a blob

**Description**

Allocate the ipc blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_key\_alloc(struct [key](#c.lsm_key_alloc "key") \*key)
:   allocate a composite key blob

**Parameters**

`struct key *key`
:   the key that needs a blob

**Description**

Allocate the key blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_msg\_msg\_alloc(struct msg\_msg \*mp)
:   allocate a composite msg\_msg blob

**Parameters**

`struct msg_msg *mp`
:   the msg\_msg that needs a blob

**Description**

Allocate the ipc blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_bdev\_alloc(struct block\_device \*bdev)
:   allocate a composite block\_device blob

**Parameters**

`struct block_device *bdev`
:   the block\_device that needs a blob

**Description**

Allocate the block\_device blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_bpf\_map\_alloc(struct bpf\_map \*map)
:   allocate a composite bpf\_map blob

**Parameters**

`struct bpf_map *map`
:   the bpf\_map that needs a blob

**Description**

Allocate the bpf\_map blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_bpf\_prog\_alloc(struct bpf\_prog \*prog)
:   allocate a composite bpf\_prog blob

**Parameters**

`struct bpf_prog *prog`
:   the bpf\_prog that needs a blob

**Description**

Allocate the bpf\_prog blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_bpf\_token\_alloc(struct bpf\_token \*token)
:   allocate a composite bpf\_token blob

**Parameters**

`struct bpf_token *token`
:   the bpf\_token that needs a blob

**Description**

Allocate the bpf\_token blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_superblock\_alloc(struct super\_block \*sb)
:   allocate a composite superblock blob

**Parameters**

`struct super_block *sb`
:   the superblock that needs a blob

**Description**

Allocate the superblock blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int lsm\_fill\_user\_ctx(struct lsm\_ctx \_\_user \*uctx, u32 \*uctx\_len, void \*val, size\_t val\_len, u64 id, u64 flags)
:   Fill a user space lsm\_ctx structure

**Parameters**

`struct lsm_ctx __user *uctx`
:   a userspace LSM context to be filled

`u32 *uctx_len`
:   available uctx size (input), used uctx size (output)

`void *val`
:   the new LSM context value

`size_t val_len`
:   the size of the new LSM context value

`u64 id`
:   LSM id

`u64 flags`
:   LSM defined flags

**Description**

Fill all of the fields in a userspace lsm\_ctx structure. If **uctx** is NULL
simply calculate the required size to output via **utc\_len** and return
success.

Returns 0 on success, -E2BIG if userspace buffer is not large enough,
-EFAULT on a copyout error, -ENOMEM if memory can’t be allocated.

int security\_binder\_set\_context\_mgr(const struct cred \*mgr)
:   Check if becoming binder ctx mgr is ok

**Parameters**

`const struct cred *mgr`
:   task credentials of current binder process

**Description**

Check whether **mgr** is allowed to be the binder context manager.

**Return**

Return 0 if permission is granted.

int security\_binder\_transaction(const struct cred \*from, const struct cred \*to)
:   Check if a binder transaction is allowed

**Parameters**

`const struct cred *from`
:   sending process

`const struct cred *to`
:   receiving process

**Description**

Check whether **from** is allowed to invoke a binder transaction call to **to**.

**Return**

Returns 0 if permission is granted.

int security\_binder\_transfer\_binder(const struct cred \*from, const struct cred \*to)
:   Check if a binder transfer is allowed

**Parameters**

`const struct cred *from`
:   sending process

`const struct cred *to`
:   receiving process

**Description**

Check whether **from** is allowed to transfer a binder reference to **to**.

**Return**

Returns 0 if permission is granted.

int security\_binder\_transfer\_file(const struct cred \*from, const struct cred \*to, const struct [file](#c.security_binder_transfer_file "file") \*file)
:   Check if a binder file xfer is allowed

**Parameters**

`const struct cred *from`
:   sending process

`const struct cred *to`
:   receiving process

`const struct file *file`
:   file being transferred

**Description**

Check whether **from** is allowed to transfer **file** to **to**.

**Return**

Returns 0 if permission is granted.

int security\_ptrace\_access\_check(struct task\_struct \*child, unsigned int mode)
:   Check if tracing is allowed

**Parameters**

`struct task_struct *child`
:   target process

`unsigned int mode`
:   PTRACE\_MODE flags

**Description**

Check permission before allowing the current process to trace the **child**
process. Security modules may also want to perform a process tracing check
during an execve in the set\_security or apply\_creds hooks of tracing check
during an execve in the bprm\_set\_creds hook of binprm\_security\_ops if the
process is being traced and its security attributes would be changed by the
execve.

**Return**

Returns 0 if permission is granted.

int security\_ptrace\_traceme(struct task\_struct \*parent)
:   Check if tracing is allowed

**Parameters**

`struct task_struct *parent`
:   tracing process

**Description**

Check that the **parent** process has sufficient permission to trace the
current process before allowing the current process to present itself to the
**parent** process for tracing.

**Return**

Returns 0 if permission is granted.

int security\_capget(const struct task\_struct \*target, kernel\_cap\_t \*effective, kernel\_cap\_t \*inheritable, kernel\_cap\_t \*permitted)
:   Get the capability sets for a process

**Parameters**

`const struct task_struct *target`
:   target process

`kernel_cap_t *effective`
:   effective capability set

`kernel_cap_t *inheritable`
:   inheritable capability set

`kernel_cap_t *permitted`
:   permitted capability set

**Description**

Get the **effective**, **inheritable**, and **permitted** capability sets for the
**target** process. The hook may also perform permission checking to determine
if the current process is allowed to see the capability sets of the **target**
process.

**Return**

Returns 0 if the capability sets were successfully obtained.

int security\_capset(struct cred \*new, const struct cred \*old, const kernel\_cap\_t \*effective, const kernel\_cap\_t \*inheritable, const kernel\_cap\_t \*permitted)
:   Set the capability sets for a process

**Parameters**

`struct cred *new`
:   new credentials for the target process

`const struct cred *old`
:   current credentials of the target process

`const kernel_cap_t *effective`
:   effective capability set

`const kernel_cap_t *inheritable`
:   inheritable capability set

`const kernel_cap_t *permitted`
:   permitted capability set

**Description**

Set the **effective**, **inheritable**, and **permitted** capability sets for the
current process.

**Return**

Returns 0 and update **new** if permission is granted.

int security\_capable(const struct [cred](#c.security_capable "cred") \*cred, struct user\_namespace \*ns, int cap, unsigned int opts)
:   Check if a process has the necessary capability

**Parameters**

`const struct cred *cred`
:   credentials to examine

`struct user_namespace *ns`
:   user namespace

`int cap`
:   capability requested

`unsigned int opts`
:   capability check options

**Description**

Check whether the **tsk** process has the **cap** capability in the indicated
credentials. **cap** contains the capability <include/linux/capability.h>.
**opts** contains options for the capable check <include/linux/security.h>.

**Return**

Returns 0 if the capability is granted.

int security\_quotactl(int cmds, int type, int id, const struct super\_block \*sb)
:   Check if a `quotactl()` syscall is allowed for this fs

**Parameters**

`int cmds`
:   commands

`int type`
:   type

`int id`
:   id

`const struct super_block *sb`
:   filesystem

**Description**

Check whether the quotactl syscall is allowed for this **sb**.

**Return**

Returns 0 if permission is granted.

int security\_quota\_on(struct [dentry](#c.security_quota_on "dentry") \*dentry)
:   Check if QUOTAON is allowed for a dentry

**Parameters**

`struct dentry *dentry`
:   dentry

**Description**

Check whether QUOTAON is allowed for **dentry**.

**Return**

Returns 0 if permission is granted.

int security\_syslog(int type)
:   Check if accessing the kernel message ring is allowed

**Parameters**

`int type`
:   SYSLOG\_ACTION\_\* type

**Description**

Check permission before accessing the kernel message ring or changing
logging to the console. See the syslog(2) manual page for an explanation of
the **type** values.

**Return**

Return 0 if permission is granted.

int security\_settime64(const struct timespec64 \*ts, const struct timezone \*tz)
:   Check if changing the system time is allowed

**Parameters**

`const struct timespec64 *ts`
:   new time

`const struct timezone *tz`
:   timezone

**Description**

Check permission to change the system time, `struct timespec64` is defined in
<include/linux/time64.h> and timezone is defined in <include/linux/time.h>.

**Return**

Returns 0 if permission is granted.

int security\_vm\_enough\_memory\_mm(struct mm\_struct \*mm, long pages)
:   Check if allocating a new mem map is allowed

**Parameters**

`struct mm_struct *mm`
:   mm struct

`long pages`
:   number of pages

**Description**

Check permissions for allocating a new virtual mapping. If all LSMs return
a positive value, `__vm_enough_memory()` will be called with cap\_sys\_admin
set. If at least one LSM returns 0 or negative, `__vm_enough_memory()` will be
called with cap\_sys\_admin cleared.

**Return**

Returns 0 if permission is granted by the LSM infrastructure to the
caller.

int security\_bprm\_creds\_for\_exec(struct linux\_binprm \*bprm)
:   Prepare the credentials for `exec()`

**Parameters**

`struct linux_binprm *bprm`
:   binary program information

**Description**

If the setup in prepare\_exec\_creds did not setup **bprm->cred->security**
properly for executing **bprm->file**, update the LSM’s portion of
**bprm->cred->security** to be what commit\_creds needs to install for the new
program. This hook may also optionally check permissions (e.g. for
transitions between security domains). The hook must set **bprm->secureexec**
to 1 if AT\_SECURE should be set to request libc enable secure mode. **bprm**
contains the linux\_binprm structure.

If execveat(2) is called with the AT\_EXECVE\_CHECK flag, bprm->is\_check is
set. The result must be the same as without this flag even if the execution
will never really happen and **bprm** will always be dropped.

This hook must not change current->cred, only **bprm->cred**.

**Return**

Returns 0 if the hook is successful and permission is granted.

int security\_bprm\_creds\_from\_file(struct linux\_binprm \*bprm, const struct [file](#c.security_bprm_creds_from_file "file") \*file)
:   Update linux\_binprm creds based on file

**Parameters**

`struct linux_binprm *bprm`
:   binary program information

`const struct file *file`
:   associated file

**Description**

If **file** is setpcap, suid, sgid or otherwise marked to change privilege upon
exec, update **bprm->cred** to reflect that change. This is called after
finding the binary that will be executed without an interpreter. This
ensures that the credentials will not be derived from a script that the
binary will need to reopen, which when reopend may end up being a completely
different file. This hook may also optionally check permissions (e.g. for
transitions between security domains). The hook must set **bprm->secureexec**
to 1 if AT\_SECURE should be set to request libc enable secure mode. The
hook must add to **bprm->per\_clear** any personality flags that should be
cleared from current->personality. **bprm** contains the linux\_binprm
structure.

**Return**

Returns 0 if the hook is successful and permission is granted.

int security\_bprm\_check(struct linux\_binprm \*bprm)
:   Mediate binary handler search

**Parameters**

`struct linux_binprm *bprm`
:   binary program information

**Description**

This hook mediates the point when a search for a binary handler will begin.
It allows a check against the **bprm->cred->security** value which was set in
the preceding creds\_for\_exec call. The argv list and envp list are reliably
available in **bprm**. This hook may be called multiple times during a single
execve. **bprm** contains the linux\_binprm structure.

**Return**

Returns 0 if the hook is successful and permission is granted.

void security\_bprm\_committing\_creds(const struct linux\_binprm \*bprm)
:   Install creds for a process during `exec()`

**Parameters**

`const struct linux_binprm *bprm`
:   binary program information

**Description**

Prepare to install the new security attributes of a process being
transformed by an execve operation, based on the old credentials pointed to
by **current->cred** and the information set in **bprm->cred** by the
bprm\_creds\_for\_exec hook. **bprm** points to the linux\_binprm structure. This
hook is a good place to perform state changes on the process such as closing
open file descriptors to which access will no longer be granted when the
attributes are changed. This is called immediately before `commit_creds()`.

void security\_bprm\_committed\_creds(const struct linux\_binprm \*bprm)
:   Tidy up after cred install during `exec()`

**Parameters**

`const struct linux_binprm *bprm`
:   binary program information

**Description**

Tidy up after the installation of the new security attributes of a process
being transformed by an execve operation. The new credentials have, by this
point, been set to **current->cred**. **bprm** points to the linux\_binprm
structure. This hook is a good place to perform state changes on the
process such as clearing out non-inheritable signal state. This is called
immediately after `commit_creds()`.

int security\_fs\_context\_submount(struct fs\_context \*fc, struct super\_block \*reference)
:   Initialise fc->security

**Parameters**

`struct fs_context *fc`
:   new filesystem context

`struct super_block *reference`
:   dentry reference for submount/remount

**Description**

Fill out the ->security field for a new fs\_context.

**Return**

Returns 0 on success or negative error code on failure.

int security\_fs\_context\_dup(struct fs\_context \*fc, struct fs\_context \*src\_fc)
:   Duplicate a fs\_context LSM blob

**Parameters**

`struct fs_context *fc`
:   destination filesystem context

`struct fs_context *src_fc`
:   source filesystem context

**Description**

Allocate and attach a security structure to sc->security. This pointer is
initialised to NULL by the caller. **fc** indicates the new filesystem context.
**src\_fc** indicates the original filesystem context.

**Return**

Returns 0 on success or a negative error code on failure.

int security\_fs\_context\_parse\_param(struct fs\_context \*fc, struct fs\_parameter \*param)
:   Configure a filesystem context

**Parameters**

`struct fs_context *fc`
:   filesystem context

`struct fs_parameter *param`
:   filesystem parameter

**Description**

Userspace provided a parameter to configure a superblock. The LSM can
consume the parameter or return it to the caller for use elsewhere.

**Return**

If the parameter is used by the LSM it should return 0, if it is
returned to the caller -ENOPARAM is returned, otherwise a negative
error code is returned.

int security\_sb\_alloc(struct super\_block \*sb)
:   Allocate a super\_block LSM blob

**Parameters**

`struct super_block *sb`
:   filesystem superblock

**Description**

Allocate and attach a security structure to the sb->s\_security field. The
s\_security field is initialized to NULL when the structure is allocated.
**sb** contains the super\_block structure to be modified.

**Return**

Returns 0 if operation was successful.

void security\_sb\_delete(struct super\_block \*sb)
:   Release super\_block LSM associated objects

**Parameters**

`struct super_block *sb`
:   filesystem superblock

**Description**

Release objects tied to a superblock (e.g. inodes). **sb** contains the
super\_block structure being released.

void security\_sb\_free(struct super\_block \*sb)
:   Free a super\_block LSM blob

**Parameters**

`struct super_block *sb`
:   filesystem superblock

**Description**

Deallocate and clear the sb->s\_security field. **sb** contains the super\_block
structure to be modified.

int security\_sb\_kern\_mount(const struct super\_block \*sb)
:   Check if a kernel mount is allowed

**Parameters**

`const struct super_block *sb`
:   filesystem superblock

**Description**

Mount this **sb** if allowed by permissions.

**Return**

Returns 0 if permission is granted.

int security\_sb\_show\_options(struct seq\_file \*m, struct super\_block \*sb)
:   Output the mount options for a superblock

**Parameters**

`struct seq_file *m`
:   output file

`struct super_block *sb`
:   filesystem superblock

**Description**

Show (print on **m**) mount options for this **sb**.

**Return**

Returns 0 on success, negative values on failure.

int security\_sb\_statfs(struct [dentry](#c.security_sb_statfs "dentry") \*dentry)
:   Check if accessing fs stats is allowed

**Parameters**

`struct dentry *dentry`
:   superblock handle

**Description**

Check permission before obtaining filesystem statistics for the **mnt**
mountpoint. **dentry** is a handle on the superblock for the filesystem.

**Return**

Returns 0 if permission is granted.

int security\_sb\_mount(const char \*dev\_name, const struct [path](#c.security_sb_mount "path") \*path, const char \*type, unsigned long flags, void \*data)
:   Check permission for mounting a filesystem

**Parameters**

`const char *dev_name`
:   filesystem backing device

`const struct path *path`
:   mount point

`const char *type`
:   filesystem type

`unsigned long flags`
:   mount flags

`void *data`
:   filesystem specific data

**Description**

Check permission before an object specified by **dev\_name** is mounted on the
mount point named by **nd**. For an ordinary mount, **dev\_name** identifies a
device if the file system type requires a device. For a remount
(**flags** & MS\_REMOUNT), **dev\_name** is irrelevant. For a loopback/bind mount
(**flags** & MS\_BIND), **dev\_name** identifies the pathname of the object being
mounted.

**Return**

Returns 0 if permission is granted.

int security\_sb\_umount(struct vfsmount \*mnt, int flags)
:   Check permission for unmounting a filesystem

**Parameters**

`struct vfsmount *mnt`
:   mounted filesystem

`int flags`
:   unmount flags

**Description**

Check permission before the **mnt** file system is unmounted.

**Return**

Returns 0 if permission is granted.

int security\_sb\_pivotroot(const struct path \*old\_path, const struct path \*new\_path)
:   Check permissions for pivoting the rootfs

**Parameters**

`const struct path *old_path`
:   new location for current rootfs

`const struct path *new_path`
:   location of the new rootfs

**Description**

Check permission before pivoting the root filesystem.

**Return**

Returns 0 if permission is granted.

int security\_move\_mount(const struct path \*from\_path, const struct path \*to\_path)
:   Check permissions for moving a mount

**Parameters**

`const struct path *from_path`
:   source mount point

`const struct path *to_path`
:   destination mount point

**Description**

Check permission before a mount is moved.

**Return**

Returns 0 if permission is granted.

int security\_path\_notify(const struct [path](#c.security_path_notify "path") \*path, u64 mask, unsigned int obj\_type)
:   Check if setting a watch is allowed

**Parameters**

`const struct path *path`
:   file path

`u64 mask`
:   event mask

`unsigned int obj_type`
:   file path type

**Description**

Check permissions before setting a watch on events as defined by **mask**, on
an object at **path**, whose type is defined by **obj\_type**.

**Return**

Returns 0 if permission is granted.

int security\_inode\_alloc(struct [inode](#c.security_inode_alloc "inode") \*inode, gfp\_t gfp)
:   Allocate an inode LSM blob

**Parameters**

`struct inode *inode`
:   the inode

`gfp_t gfp`
:   allocation flags

**Description**

Allocate and attach a security structure to **inode->i\_security**. The
i\_security field is initialized to NULL when the inode structure is
allocated.

**Return**

Return 0 if operation was successful.

void security\_inode\_free(struct [inode](#c.security_inode_free "inode") \*inode)
:   Free an inode’s LSM blob

**Parameters**

`struct inode *inode`
:   the inode

**Description**

Release any LSM resources associated with **inode**, although due to the
inode’s RCU protections it is possible that the resources will not be
fully released until after the current RCU grace period has elapsed.

It is important for LSMs to note that despite being present in a call to
[`security_inode_free()`](#c.security_inode_free "security_inode_free"), **inode** may still be referenced in a VFS path walk
and calls to [`security_inode_permission()`](#c.security_inode_permission "security_inode_permission") may be made during, or after,
a call to [`security_inode_free()`](#c.security_inode_free "security_inode_free"). For this reason the inode->i\_security
field is released via a [`call_rcu()`](#c.call_rcu "call_rcu") callback and any LSMs which need to
retain inode state for use in [`security_inode_permission()`](#c.security_inode_permission "security_inode_permission") should only
release that state in the `inode_free_security_rcu()` LSM hook callback.

int security\_inode\_init\_security\_anon(struct [inode](#c.security_inode_init_security_anon "inode") \*inode, const struct qstr \*name, const struct [inode](#c.security_inode_init_security_anon "inode") \*context\_inode)
:   Initialize an anonymous inode

**Parameters**

`struct inode *inode`
:   the inode

`const struct qstr *name`
:   the anonymous inode class

`const struct inode *context_inode`
:   an optional related inode

**Description**

Set up the incore security field for the new anonymous inode and return
whether the inode creation is permitted by the security module or not.

**Return**

Returns 0 on success, -EACCES if the security module denies the
creation of this inode, or another -errno upon other errors.

void security\_path\_post\_mknod(struct mnt\_idmap \*idmap, struct [dentry](#c.security_path_post_mknod "dentry") \*dentry)
:   Update inode security after reg file creation

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   new file

**Description**

Update inode security field after a regular file has been created.

int security\_path\_rmdir(const struct path \*dir, struct [dentry](#c.security_path_rmdir "dentry") \*dentry)
:   Check if removing a directory is allowed

**Parameters**

`const struct path *dir`
:   parent directory

`struct dentry *dentry`
:   directory to remove

**Description**

Check the permission to remove a directory.

**Return**

Returns 0 if permission is granted.

int security\_path\_symlink(const struct path \*dir, struct [dentry](#c.security_path_symlink "dentry") \*dentry, const char \*old\_name)
:   Check if creating a symbolic link is allowed

**Parameters**

`const struct path *dir`
:   parent directory

`struct dentry *dentry`
:   symbolic link

`const char *old_name`
:   file pathname

**Description**

Check the permission to create a symbolic link to a file.

**Return**

Returns 0 if permission is granted.

int security\_path\_link(struct dentry \*old\_dentry, const struct path \*new\_dir, struct dentry \*new\_dentry)
:   Check if creating a hard link is allowed

**Parameters**

`struct dentry *old_dentry`
:   existing file

`const struct path *new_dir`
:   new parent directory

`struct dentry *new_dentry`
:   new link

**Description**

Check permission before creating a new hard link to a file.

**Return**

Returns 0 if permission is granted.

int security\_path\_truncate(const struct [path](#c.security_path_truncate "path") \*path)
:   Check if truncating a file is allowed

**Parameters**

`const struct path *path`
:   file

**Description**

Check permission before truncating the file indicated by path. Note that
truncation permissions may also be checked based on already opened files,
using the [`security_file_truncate()`](#c.security_file_truncate "security_file_truncate") hook.

**Return**

Returns 0 if permission is granted.

int security\_path\_chmod(const struct [path](#c.security_path_chmod "path") \*path, umode\_t mode)
:   Check if changing the file’s mode is allowed

**Parameters**

`const struct path *path`
:   file

`umode_t mode`
:   new mode

**Description**

Check for permission to change a mode of the file **path**. The new mode is
specified in **mode** which is a bitmask of constants from
<include/uapi/linux/stat.h>.

**Return**

Returns 0 if permission is granted.

int security\_path\_chown(const struct [path](#c.security_path_chown "path") \*path, kuid\_t uid, kgid\_t gid)
:   Check if changing the file’s owner/group is allowed

**Parameters**

`const struct path *path`
:   file

`kuid_t uid`
:   file owner

`kgid_t gid`
:   file group

**Description**

Check for permission to change owner/group of a file or directory.

**Return**

Returns 0 if permission is granted.

int security\_path\_chroot(const struct [path](#c.security_path_chroot "path") \*path)
:   Check if changing the root directory is allowed

**Parameters**

`const struct path *path`
:   directory

**Description**

Check for permission to change root directory.

**Return**

Returns 0 if permission is granted.

void security\_inode\_post\_create\_tmpfile(struct mnt\_idmap \*idmap, struct [inode](#c.security_inode_post_create_tmpfile "inode") \*inode)
:   Update inode security of new tmpfile

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct inode *inode`
:   inode of the new tmpfile

**Description**

Update inode security data after a tmpfile has been created.

int security\_inode\_link(struct dentry \*old\_dentry, struct inode \*dir, struct dentry \*new\_dentry)
:   Check if creating a hard link is allowed

**Parameters**

`struct dentry *old_dentry`
:   existing file

`struct inode *dir`
:   new parent directory

`struct dentry *new_dentry`
:   new link

**Description**

Check permission before creating a new hard link to a file.

**Return**

Returns 0 if permission is granted.

int security\_inode\_unlink(struct inode \*dir, struct [dentry](#c.security_inode_unlink "dentry") \*dentry)
:   Check if removing a hard link is allowed

**Parameters**

`struct inode *dir`
:   parent directory

`struct dentry *dentry`
:   file

**Description**

Check the permission to remove a hard link to a file.

**Return**

Returns 0 if permission is granted.

int security\_inode\_symlink(struct inode \*dir, struct [dentry](#c.security_inode_symlink "dentry") \*dentry, const char \*old\_name)
:   Check if creating a symbolic link is allowed

**Parameters**

`struct inode *dir`
:   parent directory

`struct dentry *dentry`
:   symbolic link

`const char *old_name`
:   existing filename

**Description**

Check the permission to create a symbolic link to a file.

**Return**

Returns 0 if permission is granted.

int security\_inode\_rmdir(struct inode \*dir, struct [dentry](#c.security_inode_rmdir "dentry") \*dentry)
:   Check if removing a directory is allowed

**Parameters**

`struct inode *dir`
:   parent directory

`struct dentry *dentry`
:   directory to be removed

**Description**

Check the permission to remove a directory.

**Return**

Returns 0 if permission is granted.

int security\_inode\_mknod(struct inode \*dir, struct [dentry](#c.security_inode_mknod "dentry") \*dentry, umode\_t mode, dev\_t dev)
:   Check if creating a special file is allowed

**Parameters**

`struct inode *dir`
:   parent directory

`struct dentry *dentry`
:   new file

`umode_t mode`
:   new file mode

`dev_t dev`
:   device number

**Description**

Check permissions when creating a special file (or a socket or a fifo file
created via the mknod system call). Note that if mknod operation is being
done for a regular file, then the create hook will be called and not this
hook.

**Return**

Returns 0 if permission is granted.

int security\_inode\_rename(struct inode \*old\_dir, struct dentry \*old\_dentry, struct inode \*new\_dir, struct dentry \*new\_dentry, unsigned int flags)
:   Check if renaming a file is allowed

**Parameters**

`struct inode *old_dir`
:   parent directory of the old file

`struct dentry *old_dentry`
:   the old file

`struct inode *new_dir`
:   parent directory of the new file

`struct dentry *new_dentry`
:   the new file

`unsigned int flags`
:   flags

**Description**

Check for permission to rename a file or directory.

**Return**

Returns 0 if permission is granted.

int security\_inode\_readlink(struct [dentry](#c.security_inode_readlink "dentry") \*dentry)
:   Check if reading a symbolic link is allowed

**Parameters**

`struct dentry *dentry`
:   link

**Description**

Check the permission to read the symbolic link.

**Return**

Returns 0 if permission is granted.

int security\_inode\_follow\_link(struct [dentry](#c.security_inode_follow_link "dentry") \*dentry, struct [inode](#c.security_inode_follow_link "inode") \*inode, bool rcu)
:   Check if following a symbolic link is allowed

**Parameters**

`struct dentry *dentry`
:   link dentry

`struct inode *inode`
:   link inode

`bool rcu`
:   true if in RCU-walk mode

**Description**

Check permission to follow a symbolic link when looking up a pathname. If
**rcu** is true, **inode** is not stable.

**Return**

Returns 0 if permission is granted.

int security\_inode\_permission(struct [inode](#c.security_inode_permission "inode") \*inode, int mask)
:   Check if accessing an inode is allowed

**Parameters**

`struct inode *inode`
:   inode

`int mask`
:   access mask

**Description**

Check permission before accessing an inode. This hook is called by the
existing Linux permission function, so a security module can use it to
provide additional checking for existing Linux permission checks. Notice
that this hook is called when a file is opened (as well as many other
operations), whereas the file\_security\_ops permission hook is called when
the actual read/write operations are performed.

**Return**

Returns 0 if permission is granted.

void security\_inode\_post\_setattr(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_post_setattr "dentry") \*dentry, int ia\_valid)
:   Update the inode after a setattr operation

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`int ia_valid`
:   file attributes set

**Description**

Update inode security field after successful setting file attributes.

int security\_inode\_getattr(const struct [path](#c.security_inode_getattr "path") \*path)
:   Check if getting file attributes is allowed

**Parameters**

`const struct path *path`
:   file

**Description**

Check permission before obtaining file attributes.

**Return**

Returns 0 if permission is granted.

int security\_inode\_setxattr(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_setxattr "dentry") \*dentry, const char \*name, const void \*value, size\_t size, int flags)
:   Check if setting file xattrs is allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`const char *name`
:   xattr name

`const void *value`
:   xattr value

`size_t size`
:   size of xattr value

`int flags`
:   flags

**Description**

This hook performs the desired permission checks before setting the extended
attributes (xattrs) on **dentry**. It is important to note that we have some
additional logic before the main LSM implementation calls to detect if we
need to perform an additional capability check at the LSM layer.

Normally we enforce a capability check prior to executing the various LSM
hook implementations, but if a LSM wants to avoid this capability check,
it can register a ‘inode\_xattr\_skipcap’ hook and return a value of 1 for
xattrs that it wants to avoid the capability check, leaving the LSM fully
responsible for enforcing the access control for the specific xattr. If all
of the enabled LSMs refrain from registering a ‘inode\_xattr\_skipcap’ hook,
or return a 0 (the default return value), the capability check is still
performed. If no ‘inode\_xattr\_skipcap’ hooks are registered the capability
check is performed.

**Return**

Returns 0 if permission is granted.

int security\_inode\_set\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_set_acl "dentry") \*dentry, const char \*acl\_name, struct posix\_acl \*kacl)
:   Check if setting posix acls is allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`const char *acl_name`
:   acl name

`struct posix_acl *kacl`
:   acl struct

**Description**

Check permission before setting posix acls, the posix acls in **kacl** are
identified by **acl\_name**.

**Return**

Returns 0 if permission is granted.

void security\_inode\_post\_set\_acl(struct [dentry](#c.security_inode_post_set_acl "dentry") \*dentry, const char \*acl\_name, struct posix\_acl \*kacl)
:   Update inode security from posix acls set

**Parameters**

`struct dentry *dentry`
:   file

`const char *acl_name`
:   acl name

`struct posix_acl *kacl`
:   acl struct

**Description**

Update inode security data after successfully setting posix acls on **dentry**.
The posix acls in **kacl** are identified by **acl\_name**.

int security\_inode\_get\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_get_acl "dentry") \*dentry, const char \*acl\_name)
:   Check if reading posix acls is allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`const char *acl_name`
:   acl name

**Description**

Check permission before getting osix acls, the posix acls are identified by
**acl\_name**.

**Return**

Returns 0 if permission is granted.

int security\_inode\_remove\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_remove_acl "dentry") \*dentry, const char \*acl\_name)
:   Check if removing a posix acl is allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`const char *acl_name`
:   acl name

**Description**

Check permission before removing posix acls, the posix acls are identified
by **acl\_name**.

**Return**

Returns 0 if permission is granted.

void security\_inode\_post\_remove\_acl(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_post_remove_acl "dentry") \*dentry, const char \*acl\_name)
:   Update inode security after rm posix acls

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`const char *acl_name`
:   acl name

**Description**

Update inode security data after successfully removing posix acls on
**dentry** in **idmap**. The posix acls are identified by **acl\_name**.

void security\_inode\_post\_setxattr(struct [dentry](#c.security_inode_post_setxattr "dentry") \*dentry, const char \*name, const void \*value, size\_t size, int flags)
:   Update the inode after a setxattr operation

**Parameters**

`struct dentry *dentry`
:   file

`const char *name`
:   xattr name

`const void *value`
:   xattr value

`size_t size`
:   xattr value size

`int flags`
:   flags

**Description**

Update inode security field after successful setxattr operation.

int security\_inode\_getxattr(struct [dentry](#c.security_inode_getxattr "dentry") \*dentry, const char \*name)
:   Check if xattr access is allowed

**Parameters**

`struct dentry *dentry`
:   file

`const char *name`
:   xattr name

**Description**

Check permission before obtaining the extended attributes identified by
**name** for **dentry**.

**Return**

Returns 0 if permission is granted.

int security\_inode\_listxattr(struct [dentry](#c.security_inode_listxattr "dentry") \*dentry)
:   Check if listing xattrs is allowed

**Parameters**

`struct dentry *dentry`
:   file

**Description**

Check permission before obtaining the list of extended attribute names for
**dentry**.

**Return**

Returns 0 if permission is granted.

int security\_inode\_removexattr(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_removexattr "dentry") \*dentry, const char \*name)
:   Check if removing an xattr is allowed

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   file

`const char *name`
:   xattr name

**Description**

This hook performs the desired permission checks before setting the extended
attributes (xattrs) on **dentry**. It is important to note that we have some
additional logic before the main LSM implementation calls to detect if we
need to perform an additional capability check at the LSM layer.

Normally we enforce a capability check prior to executing the various LSM
hook implementations, but if a LSM wants to avoid this capability check,
it can register a ‘inode\_xattr\_skipcap’ hook and return a value of 1 for
xattrs that it wants to avoid the capability check, leaving the LSM fully
responsible for enforcing the access control for the specific xattr. If all
of the enabled LSMs refrain from registering a ‘inode\_xattr\_skipcap’ hook,
or return a 0 (the default return value), the capability check is still
performed. If no ‘inode\_xattr\_skipcap’ hooks are registered the capability
check is performed.

**Return**

Returns 0 if permission is granted.

void security\_inode\_post\_removexattr(struct [dentry](#c.security_inode_post_removexattr "dentry") \*dentry, const char \*name)
:   Update the inode after a removexattr op

**Parameters**

`struct dentry *dentry`
:   file

`const char *name`
:   xattr name

**Description**

Update the inode after a successful removexattr operation.

int security\_inode\_file\_setattr(struct [dentry](#c.security_inode_file_setattr "dentry") \*dentry, struct file\_kattr \*fa)
:   check if setting fsxattr is allowed

**Parameters**

`struct dentry *dentry`
:   file to set filesystem extended attributes on

`struct file_kattr *fa`
:   extended attributes to set on the inode

**Description**

Called when `file_setattr()` syscall or FS\_IOC\_FSSETXATTR ioctl() is called on
inode

**Return**

Returns 0 if permission is granted.

int security\_inode\_file\_getattr(struct [dentry](#c.security_inode_file_getattr "dentry") \*dentry, struct file\_kattr \*fa)
:   check if retrieving fsxattr is allowed

**Parameters**

`struct dentry *dentry`
:   file to retrieve filesystem extended attributes from

`struct file_kattr *fa`
:   extended attributes to get

**Description**

Called when `file_getattr()` syscall or FS\_IOC\_FSGETXATTR ioctl() is called on
inode

**Return**

Returns 0 if permission is granted.

int security\_inode\_need\_killpriv(struct [dentry](#c.security_inode_need_killpriv "dentry") \*dentry)
:   Check if [`security_inode_killpriv()`](#c.security_inode_killpriv "security_inode_killpriv") required

**Parameters**

`struct dentry *dentry`
:   associated dentry

**Description**

Called when an inode has been changed to determine if
[`security_inode_killpriv()`](#c.security_inode_killpriv "security_inode_killpriv") should be called.

**Return**

Return <0 on error to abort the inode change operation, return 0 if
[`security_inode_killpriv()`](#c.security_inode_killpriv "security_inode_killpriv") does not need to be called, return >0 if
[`security_inode_killpriv()`](#c.security_inode_killpriv "security_inode_killpriv") does need to be called.

int security\_inode\_killpriv(struct mnt\_idmap \*idmap, struct [dentry](#c.security_inode_killpriv "dentry") \*dentry)
:   The setuid bit is removed, update LSM state

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct dentry *dentry`
:   associated dentry

**Description**

The **dentry**’s setuid bit is being removed. Remove similar security labels.
Called with the dentry->d\_inode->i\_mutex held.

**Return**

Return 0 on success. If error is returned, then the operation
causing setuid bit removal is failed.

int security\_inode\_getsecurity(struct mnt\_idmap \*idmap, struct [inode](#c.security_inode_getsecurity "inode") \*inode, const char \*name, void \*\*buffer, bool alloc)
:   Get the xattr security label of an inode

**Parameters**

`struct mnt_idmap *idmap`
:   idmap of the mount

`struct inode *inode`
:   inode

`const char *name`
:   xattr name

`void **buffer`
:   security label buffer

`bool alloc`
:   allocation flag

**Description**

Retrieve a copy of the extended attribute representation of the security
label associated with **name** for **inode** via **buffer**. Note that **name** is the
remainder of the attribute name after the security prefix has been removed.
**alloc** is used to specify if the call should return a value via the buffer
or just the value length.

**Return**

Returns size of buffer on success.

int security\_inode\_setsecurity(struct [inode](#c.security_inode_setsecurity "inode") \*inode, const char \*name, const void \*value, size\_t size, int flags)
:   Set the xattr security label of an inode

**Parameters**

`struct inode *inode`
:   inode

`const char *name`
:   xattr name

`const void *value`
:   security label

`size_t size`
:   length of security label

`int flags`
:   flags

**Description**

Set the security label associated with **name** for **inode** from the extended
attribute value **value**. **size** indicates the size of the **value** in bytes.
**flags** may be XATTR\_CREATE, XATTR\_REPLACE, or 0. Note that **name** is the
remainder of the attribute name after the security. prefix has been removed.

**Return**

Returns 0 on success.

void security\_inode\_getlsmprop(struct [inode](#c.security_inode_getlsmprop "inode") \*inode, struct lsm\_prop \*prop)
:   Get an inode’s LSM data

**Parameters**

`struct inode *inode`
:   inode

`struct lsm_prop *prop`
:   lsm specific information to return

**Description**

Get the lsm specific information associated with the node.

int security\_kernfs\_init\_security(struct kernfs\_node \*kn\_dir, struct kernfs\_node \*kn)
:   Init LSM context for a kernfs node

**Parameters**

`struct kernfs_node *kn_dir`
:   parent kernfs node

`struct kernfs_node *kn`
:   the kernfs node to initialize

**Description**

Initialize the security context of a newly created kernfs node based on its
own and its parent’s attributes.

**Return**

Returns 0 if permission is granted.

int security\_file\_permission(struct [file](#c.security_file_permission "file") \*file, int mask)
:   Check file permissions

**Parameters**

`struct file *file`
:   file

`int mask`
:   requested permissions

**Description**

Check file permissions before accessing an open file. This hook is called
by various operations that read or write files. A security module can use
this hook to perform additional checking on these operations, e.g. to
revalidate permissions on use to support privilege bracketing or policy
changes. Notice that this hook is used when the actual read/write
operations are performed, whereas the inode\_security\_ops hook is called when
a file is opened (as well as many other operations). Although this hook can
be used to revalidate permissions for various system call operations that
read or write files, it does not address the revalidation of permissions for
memory-mapped files. Security modules must handle this separately if they
need such revalidation.

**Return**

Returns 0 if permission is granted.

int security\_file\_alloc(struct [file](#c.security_file_alloc "file") \*file)
:   Allocate and init a file’s LSM blob

**Parameters**

`struct file *file`
:   the file

**Description**

Allocate and attach a security structure to the file->f\_security field. The
security field is initialized to NULL when the structure is first created.

**Return**

Return 0 if the hook is successful and permission is granted.

void security\_file\_release(struct [file](#c.security_file_release "file") \*file)
:   Perform actions before releasing the file ref

**Parameters**

`struct file *file`
:   the file

**Description**

Perform actions before releasing the last reference to a file.

void security\_file\_free(struct [file](#c.security_file_free "file") \*file)
:   Free a file’s LSM blob

**Parameters**

`struct file *file`
:   the file

**Description**

Deallocate and free any security structures stored in file->f\_security.

int security\_backing\_file\_alloc(struct [file](../filesystems/api-summary.html#c.file "file") \*backing\_file, const struct [file](../filesystems/api-summary.html#c.file "file") \*user\_file)
:   Allocate and setup a backing file blob

**Parameters**

`struct file *backing_file`
:   the backing file

`const struct file *user_file`
:   the associated user visible file

**Description**

Allocate a backing file LSM blob and perform any necessary initialization of
the LSM blob. There will be some operations where the LSM will not have
access to **user\_file** after this point, so any important state associated
with **user\_file** that is important to the LSM should be captured in the
backing file’s LSM blob.

LSM’s should avoid taking a reference to **user\_file** in this hook as it will
result in problems later when the system attempts to drop/put the file
references due to a circular dependency.

**Return**

Return 0 if the hook is successful, negative values otherwise.

void security\_backing\_file\_free(struct [file](../filesystems/api-summary.html#c.file "file") \*backing\_file)
:   Free a backing file blob

**Parameters**

`struct file *backing_file`
:   the backing file

**Description**

Free any LSM state associate with a backing file’s LSM blob, including the
blob itself.

int security\_mmap\_file(struct [file](#c.security_mmap_file "file") \*file, unsigned long prot, unsigned long flags)
:   Check if mmap’ing a file is allowed

**Parameters**

`struct file *file`
:   file

`unsigned long prot`
:   protection applied by the kernel

`unsigned long flags`
:   flags

**Description**

Check permissions for a mmap operation. The **file** may be NULL, e.g. if
mapping anonymous memory.

**Return**

Returns 0 if permission is granted.

int security\_mmap\_addr(unsigned long addr)
:   Check if mmap’ing an address is allowed

**Parameters**

`unsigned long addr`
:   address

**Description**

Check permissions for a mmap operation at **addr**.

**Return**

Returns 0 if permission is granted.

int security\_file\_mprotect(struct vm\_area\_struct \*vma, unsigned long reqprot, unsigned long prot)
:   Check if changing memory protections is allowed

**Parameters**

`struct vm_area_struct *vma`
:   memory region

`unsigned long reqprot`
:   application requested protection

`unsigned long prot`
:   protection applied by the kernel

**Description**

Check permissions before changing memory access permissions.

**Return**

Returns 0 if permission is granted.

int security\_file\_lock(struct [file](#c.security_file_lock "file") \*file, unsigned int cmd)
:   Check if a file lock is allowed

**Parameters**

`struct file *file`
:   file

`unsigned int cmd`
:   lock operation (e.g. F\_RDLCK, F\_WRLCK)

**Description**

Check permission before performing file locking operations. Note the hook
mediates both flock and fcntl style locks.

**Return**

Returns 0 if permission is granted.

int security\_file\_fcntl(struct [file](#c.security_file_fcntl "file") \*file, unsigned int cmd, unsigned long arg)
:   Check if fcntl() op is allowed

**Parameters**

`struct file *file`
:   file

`unsigned int cmd`
:   fcntl command

`unsigned long arg`
:   command argument

**Description**

Check permission before allowing the file operation specified by **cmd** from
being performed on the file **file**. Note that **arg** sometimes represents a
user space pointer; in other cases, it may be a simple integer value. When
**arg** represents a user space pointer, it should never be used by the
security module.

**Return**

Returns 0 if permission is granted.

void security\_file\_set\_fowner(struct [file](#c.security_file_set_fowner "file") \*file)
:   Set the file owner info in the LSM blob

**Parameters**

`struct file *file`
:   the file

**Description**

Save owner security information (typically from current->security) in
file->f\_security for later use by the send\_sigiotask hook.

This hook is called with file->f\_owner.lock held.

**Return**

Returns 0 on success.

int security\_file\_send\_sigiotask(struct task\_struct \*tsk, struct fown\_struct \*fown, int sig)
:   Check if sending SIGIO/SIGURG is allowed

**Parameters**

`struct task_struct *tsk`
:   target task

`struct fown_struct *fown`
:   signal sender

`int sig`
:   signal to be sent, SIGIO is sent if 0

**Description**

Check permission for the file owner **fown** to send SIGIO or SIGURG to the
process **tsk**. Note that this hook is sometimes called from interrupt. Note
that the fown\_struct, **fown**, is never outside the context of a [`struct file`](../filesystems/api-summary.html#c.file "file"),
so the file structure (and associated security information) can always be
obtained: container\_of(fown, [`struct file`](../filesystems/api-summary.html#c.file "file"), f\_owner).

**Return**

Returns 0 if permission is granted.

int security\_file\_receive(struct [file](#c.security_file_receive "file") \*file)
:   Check if receiving a file via IPC is allowed

**Parameters**

`struct file *file`
:   file being received

**Description**

This hook allows security modules to control the ability of a process to
receive an open file descriptor via socket IPC.

**Return**

Returns 0 if permission is granted.

int security\_file\_open(struct [file](#c.security_file_open "file") \*file)
:   Save open() time state for late use by the LSM

**Parameters**

`struct file *file`

**Description**

Save open-time permission checking state for later use upon file\_permission,
and recheck access if anything has changed since inode\_permission.

We can check if a file is opened for execution (e.g. execve(2) call), either
directly or indirectly (e.g. ELF’s ld.so) by checking file->f\_flags &
\_\_FMODE\_EXEC .

**Return**

Returns 0 if permission is granted.

int security\_file\_truncate(struct [file](#c.security_file_truncate "file") \*file)
:   Check if truncating a file is allowed

**Parameters**

`struct file *file`
:   file

**Description**

Check permission before truncating a file, i.e. using ftruncate. Note that
truncation permission may also be checked based on the path, using the
**path\_truncate** hook.

**Return**

Returns 0 if permission is granted.

int security\_task\_alloc(struct task\_struct \*task, u64 clone\_flags)
:   Allocate a task’s LSM blob

**Parameters**

`struct task_struct *task`
:   the task

`u64 clone_flags`
:   flags indicating what is being shared

**Description**

Handle allocation of task-related resources.

**Return**

Returns a zero on success, negative values on failure.

void security\_task\_free(struct task\_struct \*task)
:   Free a task’s LSM blob and related resources

**Parameters**

`struct task_struct *task`
:   task

**Description**

Handle release of task-related resources. Note that this can be called from
interrupt context.

int security\_cred\_alloc\_blank(struct [cred](#c.security_cred_alloc_blank "cred") \*cred, gfp\_t gfp)
:   Allocate the min memory to allow cred\_transfer

**Parameters**

`struct cred *cred`
:   credentials

`gfp_t gfp`
:   gfp flags

**Description**

Only allocate sufficient memory and attach to **cred** such that
`cred_transfer()` will not get ENOMEM.

**Return**

Returns 0 on success, negative values on failure.

void security\_cred\_free(struct [cred](#c.security_cred_free "cred") \*cred)
:   Free the cred’s LSM blob and associated resources

**Parameters**

`struct cred *cred`
:   credentials

**Description**

Deallocate and clear the cred->security field in a set of credentials.

int security\_prepare\_creds(struct cred \*new, const struct cred \*old, gfp\_t gfp)
:   Prepare a new set of credentials

**Parameters**

`struct cred *new`
:   new credentials

`const struct cred *old`
:   original credentials

`gfp_t gfp`
:   gfp flags

**Description**

Prepare a new set of credentials by copying the data from the old set.

**Return**

Returns 0 on success, negative values on failure.

void security\_transfer\_creds(struct cred \*new, const struct cred \*old)
:   Transfer creds

**Parameters**

`struct cred *new`
:   target credentials

`const struct cred *old`
:   original credentials

**Description**

Transfer data from original creds to new creds.

int security\_kernel\_act\_as(struct cred \*new, u32 secid)
:   Set the kernel credentials to act as secid

**Parameters**

`struct cred *new`
:   credentials

`u32 secid`
:   secid

**Description**

Set the credentials for a kernel service to act as (subjective context).
The current task must be the one that nominated **secid**.

**Return**

Returns 0 if successful.

int security\_kernel\_create\_files\_as(struct cred \*new, struct [inode](#c.security_kernel_create_files_as "inode") \*inode)
:   Set file creation context using an inode

**Parameters**

`struct cred *new`
:   target credentials

`struct inode *inode`
:   reference inode

**Description**

Set the file creation context in a set of credentials to be the same as the
objective context of the specified inode. The current task must be the one
that nominated **inode**.

**Return**

Returns 0 if successful.

int security\_kernel\_module\_request(char \*kmod\_name)
:   Check if loading a module is allowed

**Parameters**

`char *kmod_name`
:   module name

**Description**

Ability to trigger the kernel to automatically upcall to userspace for
userspace to load a kernel module with the given name.

**Return**

Returns 0 if successful.

int security\_task\_fix\_setuid(struct cred \*new, const struct cred \*old, int flags)
:   Update LSM with new user id attributes

**Parameters**

`struct cred *new`
:   updated credentials

`const struct cred *old`
:   credentials being replaced

`int flags`
:   LSM\_SETID\_\* flag values

**Description**

Update the module’s state after setting one or more of the user identity
attributes of the current process. The **flags** parameter indicates which of
the set\*uid system calls invoked this hook. If **new** is the set of
credentials that will be installed. Modifications should be made to this
rather than to **current->cred**.

**Return**

Returns 0 on success.

int security\_task\_fix\_setgid(struct cred \*new, const struct cred \*old, int flags)
:   Update LSM with new group id attributes

**Parameters**

`struct cred *new`
:   updated credentials

`const struct cred *old`
:   credentials being replaced

`int flags`
:   LSM\_SETID\_\* flag value

**Description**

Update the module’s state after setting one or more of the group identity
attributes of the current process. The **flags** parameter indicates which of
the set\*gid system calls invoked this hook. **new** is the set of credentials
that will be installed. Modifications should be made to this rather than to
**current->cred**.

**Return**

Returns 0 on success.

int security\_task\_fix\_setgroups(struct cred \*new, const struct cred \*old)
:   Update LSM with new supplementary groups

**Parameters**

`struct cred *new`
:   updated credentials

`const struct cred *old`
:   credentials being replaced

**Description**

Update the module’s state after setting the supplementary group identity
attributes of the current process. **new** is the set of credentials that will
be installed. Modifications should be made to this rather than to
**current->cred**.

**Return**

Returns 0 on success.

int security\_task\_setpgid(struct task\_struct \*p, pid\_t pgid)
:   Check if setting the pgid is allowed

**Parameters**

`struct task_struct *p`
:   task being modified

`pid_t pgid`
:   new pgid

**Description**

Check permission before setting the process group identifier of the process
**p** to **pgid**.

**Return**

Returns 0 if permission is granted.

int security\_task\_getpgid(struct task\_struct \*p)
:   Check if getting the pgid is allowed

**Parameters**

`struct task_struct *p`
:   task

**Description**

Check permission before getting the process group identifier of the process
**p**.

**Return**

Returns 0 if permission is granted.

int security\_task\_getsid(struct task\_struct \*p)
:   Check if getting the session id is allowed

**Parameters**

`struct task_struct *p`
:   task

**Description**

Check permission before getting the session identifier of the process **p**.

**Return**

Returns 0 if permission is granted.

int security\_task\_setnice(struct task\_struct \*p, int nice)
:   Check if setting a task’s nice value is allowed

**Parameters**

`struct task_struct *p`
:   target task

`int nice`
:   nice value

**Description**

Check permission before setting the nice value of **p** to **nice**.

**Return**

Returns 0 if permission is granted.

int security\_task\_setioprio(struct task\_struct \*p, int ioprio)
:   Check if setting a task’s ioprio is allowed

**Parameters**

`struct task_struct *p`
:   target task

`int ioprio`
:   ioprio value

**Description**

Check permission before setting the ioprio value of **p** to **ioprio**.

**Return**

Returns 0 if permission is granted.

int security\_task\_getioprio(struct task\_struct \*p)
:   Check if getting a task’s ioprio is allowed

**Parameters**

`struct task_struct *p`
:   task

**Description**

Check permission before getting the ioprio value of **p**.

**Return**

Returns 0 if permission is granted.

int security\_task\_prlimit(const struct [cred](#c.security_task_prlimit "cred") \*cred, const struct [cred](#c.security_task_prlimit "cred") \*tcred, unsigned int flags)
:   Check if get/setting resources limits is allowed

**Parameters**

`const struct cred *cred`
:   current task credentials

`const struct cred *tcred`
:   target task credentials

`unsigned int flags`
:   LSM\_PRLIMIT\_\* flag bits indicating a get/set/both

**Description**

Check permission before getting and/or setting the resource limits of
another task.

**Return**

Returns 0 if permission is granted.

int security\_task\_setrlimit(struct task\_struct \*p, unsigned int resource, struct rlimit \*new\_rlim)
:   Check if setting a new rlimit value is allowed

**Parameters**

`struct task_struct *p`
:   target task’s group leader

`unsigned int resource`
:   resource whose limit is being set

`struct rlimit *new_rlim`
:   new resource limit

**Description**

Check permission before setting the resource limits of process **p** for
**resource** to **new\_rlim**. The old resource limit values can be examined by
dereferencing (p->signal->rlim + resource).

**Return**

Returns 0 if permission is granted.

int security\_task\_setscheduler(struct task\_struct \*p)
:   Check if setting sched policy/param is allowed

**Parameters**

`struct task_struct *p`
:   target task

**Description**

Check permission before setting scheduling policy and/or parameters of
process **p**.

**Return**

Returns 0 if permission is granted.

int security\_task\_getscheduler(struct task\_struct \*p)
:   Check if getting scheduling info is allowed

**Parameters**

`struct task_struct *p`
:   target task

**Description**

Check permission before obtaining scheduling information for process **p**.

**Return**

Returns 0 if permission is granted.

int security\_task\_movememory(struct task\_struct \*p)
:   Check if moving memory is allowed

**Parameters**

`struct task_struct *p`
:   task

**Description**

Check permission before moving memory owned by process **p**.

**Return**

Returns 0 if permission is granted.

int security\_task\_kill(struct task\_struct \*p, struct kernel\_siginfo \*info, int sig, const struct [cred](#c.security_task_kill "cred") \*cred)
:   Check if sending a signal is allowed

**Parameters**

`struct task_struct *p`
:   target process

`struct kernel_siginfo *info`
:   signal information

`int sig`
:   signal value

`const struct cred *cred`
:   credentials of the signal sender, NULL if **current**

**Description**

Check permission before sending signal **sig** to **p**. **info** can be NULL, the
constant 1, or a pointer to a kernel\_siginfo structure. If **info** is 1 or
SI\_FROMKERNEL(info) is true, then the signal should be viewed as coming from
the kernel and should typically be permitted. SIGIO signals are handled
separately by the send\_sigiotask hook in file\_security\_ops.

**Return**

Returns 0 if permission is granted.

int security\_task\_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5)
:   Check if a prctl op is allowed

**Parameters**

`int option`
:   operation

`unsigned long arg2`
:   argument

`unsigned long arg3`
:   argument

`unsigned long arg4`
:   argument

`unsigned long arg5`
:   argument

**Description**

Check permission before performing a process control operation on the
current process.

**Return**

Return -ENOSYS if no-one wanted to handle this op, any other value
to cause `prctl()` to return immediately with that value.

void security\_task\_to\_inode(struct task\_struct \*p, struct [inode](#c.security_task_to_inode "inode") \*inode)
:   Set the security attributes of a task’s inode

**Parameters**

`struct task_struct *p`
:   task

`struct inode *inode`
:   inode

**Description**

Set the security attributes for an inode based on an associated task’s
security attributes, e.g. for /proc/pid inodes.

int security\_create\_user\_ns(const struct [cred](#c.security_create_user_ns "cred") \*cred)
:   Check if creating a new userns is allowed

**Parameters**

`const struct cred *cred`
:   prepared creds

**Description**

Check permission prior to creating a new user namespace.

**Return**

Returns 0 if successful, otherwise < 0 error code.

int security\_ipc\_permission(struct kern\_ipc\_perm \*ipcp, short flag)
:   Check if sysv ipc access is allowed

**Parameters**

`struct kern_ipc_perm *ipcp`
:   ipc permission structure

`short flag`
:   requested permissions

**Description**

Check permissions for access to IPC.

**Return**

Returns 0 if permission is granted.

void security\_ipc\_getlsmprop(struct kern\_ipc\_perm \*ipcp, struct lsm\_prop \*prop)
:   Get the sysv ipc object LSM data

**Parameters**

`struct kern_ipc_perm *ipcp`
:   ipc permission structure

`struct lsm_prop *prop`
:   pointer to lsm information

**Description**

Get the lsm information associated with the ipc object.

int security\_msg\_msg\_alloc(struct msg\_msg \*msg)
:   Allocate a sysv ipc message LSM blob

**Parameters**

`struct msg_msg *msg`
:   message structure

**Description**

Allocate and attach a security structure to the msg->security field. The
security field is initialized to NULL when the structure is first created.

**Return**

Return 0 if operation was successful and permission is granted.

void security\_msg\_msg\_free(struct msg\_msg \*msg)
:   Free a sysv ipc message LSM blob

**Parameters**

`struct msg_msg *msg`
:   message structure

**Description**

Deallocate the security structure for this message.

int security\_msg\_queue\_alloc(struct kern\_ipc\_perm \*msq)
:   Allocate a sysv ipc msg queue LSM blob

**Parameters**

`struct kern_ipc_perm *msq`
:   sysv ipc permission structure

**Description**

Allocate and attach a security structure to **msg**. The security field is
initialized to NULL when the structure is first created.

**Return**

Returns 0 if operation was successful and permission is granted.

void security\_msg\_queue\_free(struct kern\_ipc\_perm \*msq)
:   Free a sysv ipc msg queue LSM blob

**Parameters**

`struct kern_ipc_perm *msq`
:   sysv ipc permission structure

**Description**

Deallocate security field **perm->security** for the message queue.

int security\_msg\_queue\_associate(struct kern\_ipc\_perm \*msq, int msqflg)
:   Check if a msg queue operation is allowed

**Parameters**

`struct kern_ipc_perm *msq`
:   sysv ipc permission structure

`int msqflg`
:   operation flags

**Description**

Check permission when a message queue is requested through the msgget system
call. This hook is only called when returning the message queue identifier
for an existing message queue, not when a new message queue is created.

**Return**

Return 0 if permission is granted.

int security\_msg\_queue\_msgctl(struct kern\_ipc\_perm \*msq, int cmd)
:   Check if a msg queue operation is allowed

**Parameters**

`struct kern_ipc_perm *msq`
:   sysv ipc permission structure

`int cmd`
:   operation

**Description**

Check permission when a message control operation specified by **cmd** is to be
performed on the message queue with permissions.

**Return**

Returns 0 if permission is granted.

int security\_msg\_queue\_msgsnd(struct kern\_ipc\_perm \*msq, struct msg\_msg \*msg, int msqflg)
:   Check if sending a sysv ipc message is allowed

**Parameters**

`struct kern_ipc_perm *msq`
:   sysv ipc permission structure

`struct msg_msg *msg`
:   message

`int msqflg`
:   operation flags

**Description**

Check permission before a message, **msg**, is enqueued on the message queue
with permissions specified in **msq**.

**Return**

Returns 0 if permission is granted.

int security\_msg\_queue\_msgrcv(struct kern\_ipc\_perm \*msq, struct msg\_msg \*msg, struct task\_struct \*target, long type, int mode)
:   Check if receiving a sysv ipc msg is allowed

**Parameters**

`struct kern_ipc_perm *msq`
:   sysv ipc permission structure

`struct msg_msg *msg`
:   message

`struct task_struct *target`
:   target task

`long type`
:   type of message requested

`int mode`
:   operation flags

**Description**

Check permission before a message, **msg**, is removed from the message queue.
The **target** task structure contains a pointer to the process that will be
receiving the message (not equal to the current process when inline receives
are being performed).

**Return**

Returns 0 if permission is granted.

int security\_shm\_alloc(struct kern\_ipc\_perm \*shp)
:   Allocate a sysv shm LSM blob

**Parameters**

`struct kern_ipc_perm *shp`
:   sysv ipc permission structure

**Description**

Allocate and attach a security structure to the **shp** security field. The
security field is initialized to NULL when the structure is first created.

**Return**

Returns 0 if operation was successful and permission is granted.

void security\_shm\_free(struct kern\_ipc\_perm \*shp)
:   Free a sysv shm LSM blob

**Parameters**

`struct kern_ipc_perm *shp`
:   sysv ipc permission structure

**Description**

Deallocate the security structure **perm->security** for the memory segment.

int security\_shm\_associate(struct kern\_ipc\_perm \*shp, int shmflg)
:   Check if a sysv shm operation is allowed

**Parameters**

`struct kern_ipc_perm *shp`
:   sysv ipc permission structure

`int shmflg`
:   operation flags

**Description**

Check permission when a shared memory region is requested through the shmget
system call. This hook is only called when returning the shared memory
region identifier for an existing region, not when a new shared memory
region is created.

**Return**

Returns 0 if permission is granted.

int security\_shm\_shmctl(struct kern\_ipc\_perm \*shp, int cmd)
:   Check if a sysv shm operation is allowed

**Parameters**

`struct kern_ipc_perm *shp`
:   sysv ipc permission structure

`int cmd`
:   operation

**Description**

Check permission when a shared memory control operation specified by **cmd** is
to be performed on the shared memory region with permissions in **shp**.

**Return**

Return 0 if permission is granted.

int security\_shm\_shmat(struct kern\_ipc\_perm \*shp, char \_\_user \*shmaddr, int shmflg)
:   Check if a sysv shm attach operation is allowed

**Parameters**

`struct kern_ipc_perm *shp`
:   sysv ipc permission structure

`char __user *shmaddr`
:   address of memory region to attach

`int shmflg`
:   operation flags

**Description**

Check permissions prior to allowing the shmat system call to attach the
shared memory segment with permissions **shp** to the data segment of the
calling process. The attaching address is specified by **shmaddr**.

**Return**

Returns 0 if permission is granted.

int security\_sem\_alloc(struct kern\_ipc\_perm \*sma)
:   Allocate a sysv semaphore LSM blob

**Parameters**

`struct kern_ipc_perm *sma`
:   sysv ipc permission structure

**Description**

Allocate and attach a security structure to the **sma** security field. The
security field is initialized to NULL when the structure is first created.

**Return**

Returns 0 if operation was successful and permission is granted.

void security\_sem\_free(struct kern\_ipc\_perm \*sma)
:   Free a sysv semaphore LSM blob

**Parameters**

`struct kern_ipc_perm *sma`
:   sysv ipc permission structure

**Description**

Deallocate security structure **sma->security** for the semaphore.

int security\_sem\_associate(struct kern\_ipc\_perm \*sma, int semflg)
:   Check if a sysv semaphore operation is allowed

**Parameters**

`struct kern_ipc_perm *sma`
:   sysv ipc permission structure

`int semflg`
:   operation flags

**Description**

Check permission when a semaphore is requested through the semget system
call. This hook is only called when returning the semaphore identifier for
an existing semaphore, not when a new one must be created.

**Return**

Returns 0 if permission is granted.

int security\_sem\_semctl(struct kern\_ipc\_perm \*sma, int cmd)
:   Check if a sysv semaphore operation is allowed

**Parameters**

`struct kern_ipc_perm *sma`
:   sysv ipc permission structure

`int cmd`
:   operation

**Description**

Check permission when a semaphore operation specified by **cmd** is to be
performed on the semaphore.

**Return**

Returns 0 if permission is granted.

int security\_sem\_semop(struct kern\_ipc\_perm \*sma, struct sembuf \*sops, unsigned nsops, int alter)
:   Check if a sysv semaphore operation is allowed

**Parameters**

`struct kern_ipc_perm *sma`
:   sysv ipc permission structure

`struct sembuf *sops`
:   operations to perform

`unsigned nsops`
:   number of operations

`int alter`
:   flag indicating changes will be made

**Description**

Check permissions before performing operations on members of the semaphore
set. If the **alter** flag is nonzero, the semaphore set may be modified.

**Return**

Returns 0 if permission is granted.

int security\_getselfattr(unsigned int attr, struct lsm\_ctx \_\_user \*uctx, u32 \_\_user \*size, u32 flags)
:   Read an LSM attribute of the current process.

**Parameters**

`unsigned int attr`
:   which attribute to return

`struct lsm_ctx __user *uctx`
:   the user-space destination for the information, or NULL

`u32 __user *size`
:   pointer to the size of space available to receive the data

`u32 flags`
:   special handling options. LSM\_FLAG\_SINGLE indicates that only
    attributes associated with the LSM identified in the passed **ctx** be
    reported.

**Description**

A NULL value for **uctx** can be used to get both the number of attributes
and the size of the data.

Returns the number of attributes found on success, negative value
on error. **size** is reset to the total size of the data.
If **size** is insufficient to contain the data -E2BIG is returned.

int security\_setselfattr(unsigned int attr, struct lsm\_ctx \_\_user \*uctx, u32 size, u32 flags)
:   Set an LSM attribute on the current process.

**Parameters**

`unsigned int attr`
:   which attribute to set

`struct lsm_ctx __user *uctx`
:   the user-space source for the information

`u32 size`
:   the size of the data

`u32 flags`
:   reserved for future use, must be 0

**Description**

Set an LSM attribute for the current process. The LSM, attribute
and new value are included in **uctx**.

Returns 0 on success, -EINVAL if the input is inconsistent, -EFAULT
if the user buffer is inaccessible, E2BIG if size is too big, or an
LSM specific failure.

int security\_getprocattr(struct task\_struct \*p, int lsmid, const char \*name, char \*\*value)
:   Read an attribute for a task

**Parameters**

`struct task_struct *p`
:   the task

`int lsmid`
:   LSM identification

`const char *name`
:   attribute name

`char **value`
:   attribute value

**Description**

Read attribute **name** for task **p** and store it into **value** if allowed.

**Return**

Returns the length of **value** on success, a negative value otherwise.

int security\_setprocattr(int lsmid, const char \*name, void \*value, size\_t size)
:   Set an attribute for a task

**Parameters**

`int lsmid`
:   LSM identification

`const char *name`
:   attribute name

`void *value`
:   attribute value

`size_t size`
:   attribute value size

**Description**

Write (set) the current task’s attribute **name** to **value**, size **size** if
allowed.

**Return**

Returns bytes written on success, a negative value otherwise.

int security\_post\_notification(const struct [cred](#c.security_post_notification "cred") \*w\_cred, const struct [cred](#c.security_post_notification "cred") \*cred, struct watch\_notification \*n)
:   Check if a watch notification can be posted

**Parameters**

`const struct cred *w_cred`
:   credentials of the task that set the watch

`const struct cred *cred`
:   credentials of the task which triggered the watch

`struct watch_notification *n`
:   the notification

**Description**

Check to see if a watch notification can be posted to a particular queue.

**Return**

Returns 0 if permission is granted.

int security\_watch\_key(struct [key](#c.security_watch_key "key") \*key)
:   Check if a task is allowed to watch for key events

**Parameters**

`struct key *key`
:   the key to watch

**Description**

Check to see if a process is allowed to watch for event notifications from
a key or keyring.

**Return**

Returns 0 if permission is granted.

int security\_netlink\_send(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb)
:   Save info and check if netlink sending is allowed

**Parameters**

`struct sock *sk`
:   sending socket

`struct sk_buff *skb`
:   netlink message

**Description**

Save security information for a netlink message so that permission checking
can be performed when the message is processed. The security information
can be saved using the eff\_cap field of the netlink\_skb\_parms structure.
Also may be used to provide fine grained control over message transmission.

**Return**

Returns 0 if the information was successfully saved and message is
allowed to be transmitted.

int security\_socket\_create(int family, int type, int protocol, int kern)
:   Check if creating a new socket is allowed

**Parameters**

`int family`
:   protocol family

`int type`
:   communications type

`int protocol`
:   requested protocol

`int kern`
:   set to 1 if a kernel socket is requested

**Description**

Check permissions prior to creating a new socket.

**Return**

Returns 0 if permission is granted.

int security\_socket\_post\_create(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, int family, int type, int protocol, int kern)
:   Initialize a newly created socket

**Parameters**

`struct socket *sock`
:   socket

`int family`
:   protocol family

`int type`
:   communications type

`int protocol`
:   requested protocol

`int kern`
:   set to 1 if a kernel socket is requested

**Description**

This hook allows a module to update or allocate a per-socket security
structure. Note that the security field was not added directly to the socket
structure, but rather, the socket security information is stored in the
associated inode. Typically, the inode alloc\_security hook will allocate
and attach security information to SOCK\_INODE(sock)->i\_security. This hook
may be used to update the SOCK\_INODE(sock)->i\_security field with additional
information that wasn’t available when the inode was allocated.

**Return**

Returns 0 if permission is granted.

int security\_socket\_bind(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct sockaddr \*address, int addrlen)
:   Check if a socket bind operation is allowed

**Parameters**

`struct socket *sock`
:   socket

`struct sockaddr *address`
:   requested bind address

`int addrlen`
:   length of address

**Description**

Check permission before socket protocol layer bind operation is performed
and the socket **sock** is bound to the address specified in the **address**
parameter.

**Return**

Returns 0 if permission is granted.

int security\_socket\_connect(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct sockaddr \*address, int addrlen)
:   Check if a socket connect operation is allowed

**Parameters**

`struct socket *sock`
:   socket

`struct sockaddr *address`
:   address of remote connection point

`int addrlen`
:   length of address

**Description**

Check permission before socket protocol layer connect operation attempts to
connect socket **sock** to a remote address, **address**.

**Return**

Returns 0 if permission is granted.

int security\_socket\_listen(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, int backlog)
:   Check if a socket is allowed to listen

**Parameters**

`struct socket *sock`
:   socket

`int backlog`
:   connection queue size

**Description**

Check permission before socket protocol layer listen operation.

**Return**

Returns 0 if permission is granted.

int security\_socket\_accept(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct [socket](../networking/kapi.html#c.socket "socket") \*newsock)
:   Check if a socket is allowed to accept connections

**Parameters**

`struct socket *sock`
:   listening socket

`struct socket *newsock`
:   newly creation connection socket

**Description**

Check permission before accepting a new connection. Note that the new
socket, **newsock**, has been created and some information copied to it, but
the accept operation has not actually been performed.

**Return**

Returns 0 if permission is granted.

int security\_socket\_sendmsg(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct msghdr \*msg, int size)
:   Check if sending a message is allowed

**Parameters**

`struct socket *sock`
:   sending socket

`struct msghdr *msg`
:   message to send

`int size`
:   size of message

**Description**

Check permission before transmitting a message to another socket.

**Return**

Returns 0 if permission is granted.

int security\_socket\_recvmsg(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, struct msghdr \*msg, int size, int flags)
:   Check if receiving a message is allowed

**Parameters**

`struct socket *sock`
:   receiving socket

`struct msghdr *msg`
:   message to receive

`int size`
:   size of message

`int flags`
:   operational flags

**Description**

Check permission before receiving a message from a socket.

**Return**

Returns 0 if permission is granted.

int security\_socket\_getsockname(struct [socket](../networking/kapi.html#c.socket "socket") \*sock)
:   Check if reading the socket addr is allowed

**Parameters**

`struct socket *sock`
:   socket

**Description**

Check permission before reading the local address (name) of the socket
object.

**Return**

Returns 0 if permission is granted.

int security\_socket\_getpeername(struct [socket](../networking/kapi.html#c.socket "socket") \*sock)
:   Check if reading the peer’s addr is allowed

**Parameters**

`struct socket *sock`
:   socket

**Description**

Check permission before the remote address (name) of a socket object.

**Return**

Returns 0 if permission is granted.

int security\_socket\_getsockopt(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, int level, int optname)
:   Check if reading a socket option is allowed

**Parameters**

`struct socket *sock`
:   socket

`int level`
:   option’s protocol level

`int optname`
:   option name

**Description**

Check permissions before retrieving the options associated with socket
**sock**.

**Return**

Returns 0 if permission is granted.

int security\_socket\_setsockopt(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, int level, int optname)
:   Check if setting a socket option is allowed

**Parameters**

`struct socket *sock`
:   socket

`int level`
:   option’s protocol level

`int optname`
:   option name

**Description**

Check permissions before setting the options associated with socket **sock**.

**Return**

Returns 0 if permission is granted.

int security\_socket\_shutdown(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, int how)
:   Checks if shutting down the socket is allowed

**Parameters**

`struct socket *sock`
:   socket

`int how`
:   flag indicating how sends and receives are handled

**Description**

Checks permission before all or part of a connection on the socket **sock** is
shut down.

**Return**

Returns 0 if permission is granted.

int security\_socket\_getpeersec\_stream(struct [socket](../networking/kapi.html#c.socket "socket") \*sock, sockptr\_t optval, sockptr\_t optlen, unsigned int len)
:   Get the remote peer label

**Parameters**

`struct socket *sock`
:   socket

`sockptr_t optval`
:   destination buffer

`sockptr_t optlen`
:   size of peer label copied into the buffer

`unsigned int len`
:   maximum size of the destination buffer

**Description**

This hook allows the security module to provide peer socket security state
for unix or connected tcp sockets to userspace via getsockopt SO\_GETPEERSEC.
For tcp sockets this can be meaningful if the socket is associated with an
ipsec SA.

**Return**

Returns 0 if all is well, otherwise, typical getsockopt return
values.

int lsm\_sock\_alloc(struct [sock](#c.lsm_sock_alloc "sock") \*sock, gfp\_t gfp)
:   allocate a composite sock blob

**Parameters**

`struct sock *sock`
:   the sock that needs a blob

`gfp_t gfp`
:   allocation mode

**Description**

Allocate the sock blob for all the modules

Returns 0, or -ENOMEM if memory can’t be allocated.

int security\_sk\_alloc(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, int family, gfp\_t priority)
:   Allocate and initialize a sock’s LSM blob

**Parameters**

`struct sock *sk`
:   sock

`int family`
:   protocol family

`gfp_t priority`
:   gfp flags

**Description**

Allocate and attach a security structure to the sk->sk\_security field, which
is used to copy security attributes between local stream sockets.

**Return**

Returns 0 on success, error on failure.

void security\_sk\_free(struct [sock](../networking/kapi.html#c.sock "sock") \*sk)
:   Free the sock’s LSM blob

**Parameters**

`struct sock *sk`
:   sock

**Description**

Deallocate security structure.

void security\_inet\_csk\_clone(struct [sock](../networking/kapi.html#c.sock "sock") \*newsk, const struct request\_sock \*req)
:   Set new sock LSM state based on request\_sock

**Parameters**

`struct sock *newsk`
:   new sock

`const struct request_sock *req`
:   connection request\_sock

**Description**

Set that LSM state of **sock** using the LSM state from **req**.

int security\_mptcp\_add\_subflow(struct [sock](../networking/kapi.html#c.sock "sock") \*sk, struct [sock](../networking/kapi.html#c.sock "sock") \*ssk)
:   Inherit the LSM label from the MPTCP socket

**Parameters**

`struct sock *sk`
:   the owning MPTCP socket

`struct sock *ssk`
:   the new subflow

**Description**

Update the labeling for the given MPTCP subflow, to match the one of the
owning MPTCP socket. This hook has to be called after the socket creation and
initialization via the [`security_socket_create()`](#c.security_socket_create "security_socket_create") and
[`security_socket_post_create()`](#c.security_socket_post_create "security_socket_post_create") LSM hooks.

**Return**

Returns 0 on success or a negative error code on failure.

int security\_xfrm\_policy\_clone(struct xfrm\_sec\_ctx \*old\_ctx, struct xfrm\_sec\_ctx \*\*new\_ctxp)
:   Clone xfrm policy LSM state

**Parameters**

`struct xfrm_sec_ctx *old_ctx`
:   xfrm security context

`struct xfrm_sec_ctx **new_ctxp`
:   target xfrm security context

**Description**

Allocate a security structure in new\_ctxp that contains the information from
the old\_ctx structure.

**Return**

Return 0 if operation was successful.

int security\_xfrm\_policy\_delete(struct xfrm\_sec\_ctx \*ctx)
:   Check if deleting a xfrm policy is allowed

**Parameters**

`struct xfrm_sec_ctx *ctx`
:   xfrm security context

**Description**

Authorize deletion of a SPD entry.

**Return**

Returns 0 if permission is granted.

int security\_xfrm\_state\_alloc\_acquire(struct xfrm\_state \*x, struct xfrm\_sec\_ctx \*polsec, u32 secid)
:   Allocate a xfrm state LSM blob

**Parameters**

`struct xfrm_state *x`
:   xfrm state being added to the SAD

`struct xfrm_sec_ctx *polsec`
:   associated policy’s security context

`u32 secid`
:   secid from the flow

**Description**

Allocate a security structure to the x->security field; the security field
is initialized to NULL when the xfrm\_state is allocated. Set the context to
correspond to secid.

**Return**

Returns 0 if operation was successful.

void security\_xfrm\_state\_free(struct xfrm\_state \*x)
:   Free a xfrm state

**Parameters**

`struct xfrm_state *x`
:   xfrm state

**Description**

Deallocate x->security.

int security\_xfrm\_policy\_lookup(struct xfrm\_sec\_ctx \*ctx, u32 fl\_secid)
:   Check if using a xfrm policy is allowed

**Parameters**

`struct xfrm_sec_ctx *ctx`
:   target xfrm security context

`u32 fl_secid`
:   flow secid used to authorize access

**Description**

Check permission when a flow selects a xfrm\_policy for processing XFRMs on a
packet. The hook is called when selecting either a per-socket policy or a
generic xfrm policy.

**Return**

Return 0 if permission is granted, -ESRCH otherwise, or -errno on
other errors.

int security\_xfrm\_state\_pol\_flow\_match(struct xfrm\_state \*x, struct xfrm\_policy \*xp, const struct flowi\_common \*flic)
:   Check for a xfrm match

**Parameters**

`struct xfrm_state *x`
:   xfrm state to match

`struct xfrm_policy *xp`
:   xfrm policy to check for a match

`const struct flowi_common *flic`
:   flow to check for a match.

**Description**

Check **xp** and **flic** for a match with **x**.

**Return**

Returns 1 if there is a match.

int security\_xfrm\_decode\_session(struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, u32 \*secid)
:   Determine the xfrm secid for a packet

**Parameters**

`struct sk_buff *skb`
:   xfrm packet

`u32 *secid`
:   secid

**Description**

Decode the packet in **skb** and return the security label in **secid**.

**Return**

Return 0 if all xfrms used have the same secid.

int security\_key\_alloc(struct [key](#c.security_key_alloc "key") \*key, const struct [cred](#c.security_key_alloc "cred") \*cred, unsigned long flags)
:   Allocate and initialize a kernel key LSM blob

**Parameters**

`struct key *key`
:   key

`const struct cred *cred`
:   credentials

`unsigned long flags`
:   allocation flags

**Description**

Permit allocation of a key and assign security data. Note that key does not
have a serial number assigned at this point.

**Return**

Return 0 if permission is granted, -ve error otherwise.

void security\_key\_free(struct [key](#c.security_key_free "key") \*key)
:   Free a kernel key LSM blob

**Parameters**

`struct key *key`
:   key

**Description**

Notification of destruction; free security data.

int security\_key\_permission(key\_ref\_t key\_ref, const struct [cred](#c.security_key_permission "cred") \*cred, enum key\_need\_perm need\_perm)
:   Check if a kernel key operation is allowed

**Parameters**

`key_ref_t key_ref`
:   key reference

`const struct cred *cred`
:   credentials of actor requesting access

`enum key_need_perm need_perm`
:   requested permissions

**Description**

See whether a specific operational right is granted to a process on a key.

**Return**

Return 0 if permission is granted, -ve error otherwise.

int security\_key\_getsecurity(struct [key](#c.security_key_getsecurity "key") \*key, char \*\*buffer)
:   Get the key’s security label

**Parameters**

`struct key *key`
:   key

`char **buffer`
:   security label buffer

**Description**

Get a textual representation of the security context attached to a key for
the purposes of honouring KEYCTL\_GETSECURITY. This function allocates the
storage for the NUL-terminated string and the caller should free it.

**Return**

Returns the length of **buffer** (including terminating NUL) or -ve if
an error occurs. May also return 0 (and a NULL buffer pointer) if
there is no security label assigned to the key.

void security\_key\_post\_create\_or\_update(struct [key](#c.security_key_post_create_or_update "key") \*keyring, struct [key](#c.security_key_post_create_or_update "key") \*key, const void \*payload, size\_t payload\_len, unsigned long flags, bool create)
:   Notification of key create or update

**Parameters**

`struct key *keyring`
:   keyring to which the key is linked to

`struct key *key`
:   created or updated key

`const void *payload`
:   data used to instantiate or update the key

`size_t payload_len`
:   length of payload

`unsigned long flags`
:   key flags

`bool create`
:   flag indicating whether the key was created or updated

**Description**

Notify the caller of a key creation or update.

int security\_audit\_rule\_init(u32 field, u32 op, char \*rulestr, void \*\*lsmrule, gfp\_t gfp)
:   Allocate and init an LSM audit rule struct

**Parameters**

`u32 field`
:   audit action

`u32 op`
:   rule operator

`char *rulestr`
:   rule context

`void **lsmrule`
:   receive buffer for audit rule struct

`gfp_t gfp`
:   GFP flag used for kmalloc

**Description**

Allocate and initialize an LSM audit rule structure.

**Return**

Return 0 if **lsmrule** has been successfully set, -EINVAL in case of
an invalid rule.

int security\_audit\_rule\_known(struct audit\_krule \*krule)
:   Check if an audit rule contains LSM fields

**Parameters**

`struct audit_krule *krule`
:   audit rule

**Description**

Specifies whether given **krule** contains any fields related to the current
LSM.

**Return**

Returns 1 in case of relation found, 0 otherwise.

void security\_audit\_rule\_free(void \*lsmrule)
:   Free an LSM audit rule struct

**Parameters**

`void *lsmrule`
:   audit rule struct

**Description**

Deallocate the LSM audit rule structure previously allocated by
`audit_rule_init()`.

int security\_audit\_rule\_match(struct lsm\_prop \*prop, u32 field, u32 op, void \*lsmrule)
:   Check if a label matches an audit rule

**Parameters**

`struct lsm_prop *prop`
:   security label

`u32 field`
:   LSM audit field

`u32 op`
:   matching operator

`void *lsmrule`
:   audit rule

**Description**

Determine if given **secid** matches a rule previously approved by
[`security_audit_rule_known()`](#c.security_audit_rule_known "security_audit_rule_known").

**Return**

Returns 1 if secid matches the rule, 0 if it does not, -ERRNO on
failure.

int security\_bpf(int cmd, union bpf\_attr \*attr, unsigned int size, bool kernel)
:   Check if the bpf syscall operation is allowed

**Parameters**

`int cmd`
:   command

`union bpf_attr *attr`
:   bpf attribute

`unsigned int size`
:   size

`bool kernel`
:   whether or not call originated from kernel

**Description**

Do a initial check for all bpf syscalls after the attribute is copied into
the kernel. The actual security module can implement their own rules to
check the specific cmd they need.

**Return**

Returns 0 if permission is granted.

int security\_bpf\_map(struct bpf\_map \*map, fmode\_t fmode)
:   Check if access to a bpf map is allowed

**Parameters**

`struct bpf_map *map`
:   bpf map

`fmode_t fmode`
:   mode

**Description**

Do a check when the kernel generates and returns a file descriptor for eBPF
maps.

**Return**

Returns 0 if permission is granted.

int security\_bpf\_prog(struct bpf\_prog \*prog)
:   Check if access to a bpf program is allowed

**Parameters**

`struct bpf_prog *prog`
:   bpf program

**Description**

Do a check when the kernel generates and returns a file descriptor for eBPF
programs.

**Return**

Returns 0 if permission is granted.

int security\_bpf\_map\_create(struct bpf\_map \*map, union bpf\_attr \*attr, struct bpf\_token \*token, bool kernel)
:   Check if BPF map creation is allowed

**Parameters**

`struct bpf_map *map`
:   BPF map object

`union bpf_attr *attr`
:   BPF syscall attributes used to create BPF map

`struct bpf_token *token`
:   BPF token used to grant user access

`bool kernel`
:   whether or not call originated from kernel

**Description**

Do a check when the kernel creates a new BPF map. This is also the
point where LSM blob is allocated for LSMs that need them.

**Return**

Returns 0 on success, error on failure.

int security\_bpf\_prog\_load(struct bpf\_prog \*prog, union bpf\_attr \*attr, struct bpf\_token \*token, bool kernel)
:   Check if loading of BPF program is allowed

**Parameters**

`struct bpf_prog *prog`
:   BPF program object

`union bpf_attr *attr`
:   BPF syscall attributes used to create BPF program

`struct bpf_token *token`
:   BPF token used to grant user access to BPF subsystem

`bool kernel`
:   whether or not call originated from kernel

**Description**

Perform an access control check when the kernel loads a BPF program and
allocates associated BPF program object. This hook is also responsible for
allocating any required LSM state for the BPF program.

**Return**

Returns 0 on success, error on failure.

int security\_bpf\_token\_create(struct bpf\_token \*token, union bpf\_attr \*attr, const struct [path](#c.security_bpf_token_create "path") \*path)
:   Check if creating of BPF token is allowed

**Parameters**

`struct bpf_token *token`
:   BPF token object

`union bpf_attr *attr`
:   BPF syscall attributes used to create BPF token

`const struct path *path`
:   path pointing to BPF FS mount point from which BPF token is created

**Description**

Do a check when the kernel instantiates a new BPF token object from BPF FS
instance. This is also the point where LSM blob can be allocated for LSMs.

**Return**

Returns 0 on success, error on failure.

int security\_bpf\_token\_cmd(const struct bpf\_token \*token, enum bpf\_cmd cmd)
:   Check if BPF token is allowed to delegate requested BPF syscall command

**Parameters**

`const struct bpf_token *token`
:   BPF token object

`enum bpf_cmd cmd`
:   BPF syscall command requested to be delegated by BPF token

**Description**

Do a check when the kernel decides whether provided BPF token should allow
delegation of requested BPF syscall command.

**Return**

Returns 0 on success, error on failure.

int security\_bpf\_token\_capable(const struct bpf\_token \*token, int cap)
:   Check if BPF token is allowed to delegate requested BPF-related capability

**Parameters**

`const struct bpf_token *token`
:   BPF token object

`int cap`
:   capabilities requested to be delegated by BPF token

**Description**

Do a check when the kernel decides whether provided BPF token should allow
delegation of requested BPF-related capabilities.

**Return**

Returns 0 on success, error on failure.

void security\_bpf\_map\_free(struct bpf\_map \*map)
:   Free a bpf map’s LSM blob

**Parameters**

`struct bpf_map *map`
:   bpf map

**Description**

Clean up the security information stored inside bpf map.

void security\_bpf\_prog\_free(struct bpf\_prog \*prog)
:   Free a BPF program’s LSM blob

**Parameters**

`struct bpf_prog *prog`
:   BPF program struct

**Description**

Clean up the security information stored inside BPF program.

void security\_bpf\_token\_free(struct bpf\_token \*token)
:   Free a BPF token’s LSM blob

**Parameters**

`struct bpf_token *token`
:   BPF token struct

**Description**

Clean up the security information stored inside BPF token.

int security\_perf\_event\_open(int type)
:   Check if a perf event open is allowed

**Parameters**

`int type`
:   type of event

**Description**

Check whether the **type** of perf\_event\_open syscall is allowed.

**Return**

Returns 0 if permission is granted.

int security\_perf\_event\_alloc(struct perf\_event \*event)
:   Allocate a perf event LSM blob

**Parameters**

`struct perf_event *event`
:   perf event

**Description**

Allocate and save perf\_event security info.

**Return**

Returns 0 on success, error on failure.

void security\_perf\_event\_free(struct perf\_event \*event)
:   Free a perf event LSM blob

**Parameters**

`struct perf_event *event`
:   perf event

**Description**

Release (free) perf\_event security info.

int security\_perf\_event\_read(struct perf\_event \*event)
:   Check if reading a perf event label is allowed

**Parameters**

`struct perf_event *event`
:   perf event

**Description**

Read perf\_event security info if allowed.

**Return**

Returns 0 if permission is granted.

int security\_perf\_event\_write(struct perf\_event \*event)
:   Check if writing a perf event label is allowed

**Parameters**

`struct perf_event *event`
:   perf event

**Description**

Write perf\_event security info if allowed.

**Return**

Returns 0 if permission is granted.

int security\_uring\_override\_creds(const struct cred \*new)
:   Check if overriding creds is allowed

**Parameters**

`const struct cred *new`
:   new credentials

**Description**

Check if the current task, executing an io\_uring operation, is allowed to
override it’s credentials with **new**.

**Return**

Returns 0 if permission is granted.

int security\_uring\_sqpoll(void)
:   Check if IORING\_SETUP\_SQPOLL is allowed

**Parameters**

`void`
:   no arguments

**Description**

Check whether the current task is allowed to spawn a io\_uring polling thread
(IORING\_SETUP\_SQPOLL).

**Return**

Returns 0 if permission is granted.

int security\_uring\_cmd(struct io\_uring\_cmd \*ioucmd)
:   Check if a io\_uring passthrough command is allowed

**Parameters**

`struct io_uring_cmd *ioucmd`
:   command

**Description**

Check whether the file\_operations uring\_cmd is allowed to run.

**Return**

Returns 0 if permission is granted.

int security\_uring\_allowed(void)
:   Check if `io_uring_setup()` is allowed

**Parameters**

`void`
:   no arguments

**Description**

Check whether the current task is allowed to call `io_uring_setup()`.

**Return**

Returns 0 if permission is granted.

void security\_initramfs\_populated(void)
:   Notify LSMs that initramfs has been loaded

**Parameters**

`void`
:   no arguments

**Description**

Tells the LSMs the initramfs has been unpacked into the rootfs.

struct dentry \*securityfs\_create\_file(const char \*name, umode\_t mode, struct dentry \*parent, void \*data, const struct file\_operations \*fops)
:   create a file in the securityfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the file to create.

`umode_t mode`
:   the permission that the file should have

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    file will be created in the root of the securityfs filesystem.

`void *data`
:   a pointer to something that the caller will want to get to later
    on. The inode.i\_private pointer will point to this value on
    the open() call.

`const struct file_operations *fops`
:   a pointer to a `struct file_operations` that should be used for
    this file.

**Description**

This function creates a file in securityfs with the given **name**.

This function returns a pointer to a dentry if it succeeds. This
pointer must be passed to the [`securityfs_remove()`](#c.securityfs_remove "securityfs_remove") function when the file is
to be removed (no automatic cleanup happens if your module is unloaded,
you are responsible here). If an error occurs, the function will return
the error value (via ERR\_PTR).

If securityfs is not enabled in the kernel, the value `-ENODEV` is
returned.

struct dentry \*securityfs\_create\_dir(const char \*name, struct dentry \*parent)
:   create a directory in the securityfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the directory to
    create.

`struct dentry *parent`
:   a pointer to the parent dentry for this file. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    directory will be created in the root of the securityfs filesystem.

**Description**

This function creates a directory in securityfs with the given **name**.

This function returns a pointer to a dentry if it succeeds. This
pointer must be passed to the [`securityfs_remove()`](#c.securityfs_remove "securityfs_remove") function when the file is
to be removed (no automatic cleanup happens if your module is unloaded,
you are responsible here). If an error occurs, the function will return
the error value (via ERR\_PTR).

If securityfs is not enabled in the kernel, the value `-ENODEV` is
returned.

struct dentry \*securityfs\_create\_symlink(const char \*name, struct dentry \*parent, const char \*target, const struct inode\_operations \*iops)
:   create a symlink in the securityfs filesystem

**Parameters**

`const char *name`
:   a pointer to a string containing the name of the symlink to
    create.

`struct dentry *parent`
:   a pointer to the parent dentry for the symlink. This should be a
    directory dentry if set. If this parameter is `NULL`, then the
    directory will be created in the root of the securityfs filesystem.

`const char *target`
:   a pointer to a string containing the name of the symlink’s target.
    If this parameter is `NULL`, then the **iops** parameter needs to be
    setup to handle .readlink and .get\_link inode\_operations.

`const struct inode_operations *iops`
:   a pointer to the `struct inode_operations` to use for the symlink. If
    this parameter is `NULL`, then the default simple\_symlink\_inode
    operations will be used.

**Description**

This function creates a symlink in securityfs with the given **name**.

This function returns a pointer to a dentry if it succeeds. This
pointer must be passed to the [`securityfs_remove()`](#c.securityfs_remove "securityfs_remove") function when the file is
to be removed (no automatic cleanup happens if your module is unloaded,
you are responsible here). If an error occurs, the function will return
the error value (via ERR\_PTR).

If securityfs is not enabled in the kernel, the value `-ENODEV` is
returned.

void securityfs\_remove(struct [dentry](#c.securityfs_remove "dentry") \*dentry)
:   removes a file or directory from the securityfs filesystem

**Parameters**

`struct dentry *dentry`
:   a pointer to a the dentry of the file or directory to be removed.

**Description**

This function removes a file or directory in securityfs that was previously
created with a call to another securityfs function (like
[`securityfs_create_file()`](#c.securityfs_create_file "securityfs_create_file") or variants thereof.)

This function is required to be called in order for the file to be
removed. No automatic cleanup of files will happen when a module is
removed; you are responsible here.

AV: when applied to directory it will take all children out; no need to call
it for descendents if ancestor is getting killed.

## Audit Interfaces

struct audit\_buffer \*audit\_log\_start(struct audit\_context \*ctx, gfp\_t gfp\_mask, int type)
:   obtain an audit buffer

**Parameters**

`struct audit_context *ctx`
:   audit\_context (may be NULL)

`gfp_t gfp_mask`
:   type of allocation

`int type`
:   audit message type

**Description**

Returns audit\_buffer pointer on success or NULL on error.

Obtain an audit buffer. This routine does locking to obtain the
audit buffer, but then no locking is required for calls to
audit\_log\_\*format. If the task (ctx) is a task that is currently in a
syscall, then the syscall is marked as auditable and an audit record
will be written at syscall exit. If there is no associated task, then
task context (ctx) should be NULL.

void audit\_log\_format(struct audit\_buffer \*ab, const char \*fmt, ...)
:   format a message into the audit buffer.

**Parameters**

`struct audit_buffer *ab`
:   audit\_buffer

`const char *fmt`
:   format string

`...`
:   optional parameters matching **fmt** string

**Description**

All the work is done in audit\_log\_vformat.

int audit\_log\_subj\_ctx(struct audit\_buffer \*ab, struct lsm\_prop \*prop)
:   Add LSM subject information

**Parameters**

`struct audit_buffer *ab`
:   audit\_buffer

`struct lsm_prop *prop`
:   LSM subject properties.

**Description**

Add a subj= field and, if necessary, a AUDIT\_MAC\_TASK\_CONTEXTS record.

void audit\_log\_end(struct audit\_buffer \*ab)
:   end one audit record

**Parameters**

`struct audit_buffer *ab`
:   the audit\_buffer

**Description**

We can not do a netlink send inside an irq context because it blocks (last
arg, flags, is not set to MSG\_DONTWAIT), so the audit buffer is placed on a
queue and a kthread is scheduled to remove them from the queue outside the
irq context. May be called in any context.

void audit\_log(struct audit\_context \*ctx, gfp\_t gfp\_mask, int type, const char \*fmt, ...)
:   Log an audit record

**Parameters**

`struct audit_context *ctx`
:   audit context

`gfp_t gfp_mask`
:   type of allocation

`int type`
:   audit message type

`const char *fmt`
:   format string to use

`...`
:   variable parameters matching the format string

**Description**

This is a convenience function that calls audit\_log\_start,
audit\_log\_vformat, and audit\_log\_end. It may be called
in any context.

int \_\_audit\_filter\_op(struct task\_struct \*tsk, struct audit\_context \*ctx, struct list\_head \*list, struct audit\_names \*name, unsigned long op)
:   common filter helper for operations (syscall/uring/etc)

**Parameters**

`struct task_struct *tsk`
:   associated task

`struct audit_context *ctx`
:   audit context

`struct list_head *list`
:   audit filter list

`struct audit_names *name`
:   audit\_name (can be NULL)

`unsigned long op`
:   current syscall/uring\_op

**Description**

Run the udit filters specified in **list** against **tsk** using **ctx**,
**name**, and **op**, as necessary; the caller is responsible for ensuring
that the call is made while the RCU read lock is held. The **name**
parameter can be NULL, but all others must be specified.
Returns 1/true if the filter finds a match, 0/false if none are found.

void audit\_filter\_uring(struct task\_struct \*tsk, struct audit\_context \*ctx)
:   apply filters to an io\_uring operation

**Parameters**

`struct task_struct *tsk`
:   associated task

`struct audit_context *ctx`
:   audit context

void audit\_reset\_context(struct audit\_context \*ctx)
:   reset a audit\_context structure

**Parameters**

`struct audit_context *ctx`
:   the audit\_context to reset

**Description**

All fields in the audit\_context will be reset to an initial state, all
references held by fields will be dropped, and private memory will be
released. When this function returns the audit\_context will be suitable
for reuse, so long as the passed context is not NULL or a dummy context.

int audit\_alloc(struct task\_struct \*tsk)
:   allocate an audit context block for a task

**Parameters**

`struct task_struct *tsk`
:   task

**Description**

Filter on the task information and allocate a per-task audit context
if necessary. Doing so turns on system call auditing for the
specified task. This is called from copy\_process, so no lock is
needed.

void audit\_log\_uring(struct audit\_context \*ctx)
:   generate a AUDIT\_URINGOP record

**Parameters**

`struct audit_context *ctx`
:   the audit context

void \_\_audit\_free(struct task\_struct \*tsk)
:   free a per-task audit context

**Parameters**

`struct task_struct *tsk`
:   task whose audit context block to free

**Description**

Called from copy\_process, do\_exit, and the io\_uring code

void audit\_return\_fixup(struct audit\_context \*ctx, int success, long code)
:   fixup the return codes in the audit\_context

**Parameters**

`struct audit_context *ctx`
:   the audit\_context

`int success`
:   true/false value to indicate if the operation succeeded or not

`long code`
:   operation return code

**Description**

We need to fixup the return code in the audit logs if the actual return
codes are later going to be fixed by the arch specific signal handlers.

void \_\_audit\_uring\_entry(u8 op)
:   prepare the kernel task’s audit context for io\_uring

**Parameters**

`u8 op`
:   the io\_uring opcode

**Description**

This is similar to `audit_syscall_entry()` but is intended for use by io\_uring
operations. This function should only ever be called from
`audit_uring_entry()` as we rely on the audit context checking present in that
function.

void \_\_audit\_uring\_exit(int success, long code)
:   wrap up the kernel task’s audit context after io\_uring

**Parameters**

`int success`
:   true/false value to indicate if the operation succeeded or not

`long code`
:   operation return code

**Description**

This is similar to `audit_syscall_exit()` but is intended for use by io\_uring
operations. This function should only ever be called from
`audit_uring_exit()` as we rely on the audit context checking present in that
function.

void \_\_audit\_syscall\_entry(int major, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4)
:   fill in an audit record at syscall entry

**Parameters**

`int major`
:   major syscall type (function)

`unsigned long a1`
:   additional syscall register 1

`unsigned long a2`
:   additional syscall register 2

`unsigned long a3`
:   additional syscall register 3

`unsigned long a4`
:   additional syscall register 4

**Description**

Fill in audit context at syscall entry. This only happens if the
audit context was created when the task was created and the state or
filters demand the audit context be built. If the state from the
per-task filter or from the per-syscall filter is AUDIT\_STATE\_RECORD,
then the record will be written at syscall exit time (otherwise, it
will only be written if another part of the kernel requests that it
be written).

void \_\_audit\_syscall\_exit(int success, long return\_code)
:   deallocate audit context after a system call

**Parameters**

`int success`
:   success value of the syscall

`long return_code`
:   return value of the syscall

**Description**

Tear down after system call. If the audit context has been marked as
auditable (either because of the AUDIT\_STATE\_RECORD state from
filtering, or because some other part of the kernel wrote an audit
message), then write out the syscall information. In call cases,
free the names stored from `getname()`.

void \_\_audit\_getname(struct filename \*name)
:   add a name to the list

**Parameters**

`struct filename *name`
:   name to add

**Description**

Add a name to the list of audit names for this context.
Called from fs/namei.c:`getname()`.

void \_\_audit\_inode(struct filename \*name, const struct [dentry](#c.__audit_inode "dentry") \*dentry, unsigned int flags)
:   store the inode and device from a lookup

**Parameters**

`struct filename *name`
:   name being audited

`const struct dentry *dentry`
:   dentry being audited

`unsigned int flags`
:   attributes for this particular entry

int auditsc\_get\_stamp(struct audit\_context \*ctx, struct audit\_stamp \*stamp)
:   get local copies of audit\_context values

**Parameters**

`struct audit_context *ctx`
:   audit\_context for the task

`struct audit_stamp *stamp`
:   timestamp to record

**Description**

Also sets the context as auditable.

void \_\_audit\_mq\_open(int oflag, umode\_t mode, struct mq\_attr \*attr)
:   record audit data for a POSIX MQ open

**Parameters**

`int oflag`
:   open flag

`umode_t mode`
:   mode bits

`struct mq_attr *attr`
:   queue attributes

void \_\_audit\_mq\_sendrecv(mqd\_t mqdes, size\_t msg\_len, unsigned int msg\_prio, const struct timespec64 \*abs\_timeout)
:   record audit data for a POSIX MQ timed send/receive

**Parameters**

`mqd_t mqdes`
:   MQ descriptor

`size_t msg_len`
:   Message length

`unsigned int msg_prio`
:   Message priority

`const struct timespec64 *abs_timeout`
:   Message timeout in absolute time

void \_\_audit\_mq\_notify(mqd\_t mqdes, const struct sigevent \*notification)
:   record audit data for a POSIX MQ notify

**Parameters**

`mqd_t mqdes`
:   MQ descriptor

`const struct sigevent *notification`
:   Notification event

void \_\_audit\_mq\_getsetattr(mqd\_t mqdes, struct mq\_attr \*mqstat)
:   record audit data for a POSIX MQ get/set attribute

**Parameters**

`mqd_t mqdes`
:   MQ descriptor

`struct mq_attr *mqstat`
:   MQ flags

void \_\_audit\_ipc\_obj(struct kern\_ipc\_perm \*ipcp)
:   record audit data for ipc object

**Parameters**

`struct kern_ipc_perm *ipcp`
:   ipc permissions

void \_\_audit\_ipc\_set\_perm(unsigned long qbytes, uid\_t uid, gid\_t gid, umode\_t mode)
:   record audit data for new ipc permissions

**Parameters**

`unsigned long qbytes`
:   msgq bytes

`uid_t uid`
:   msgq user id

`gid_t gid`
:   msgq group id

`umode_t mode`
:   msgq mode (permissions)

**Description**

Called only after `audit_ipc_obj()`.

int \_\_audit\_socketcall(int nargs, unsigned long \*args)
:   record audit data for sys\_socketcall

**Parameters**

`int nargs`
:   number of args, which should not be more than AUDITSC\_ARGS.

`unsigned long *args`
:   args array

void \_\_audit\_fd\_pair(int fd1, int fd2)
:   record audit data for pipe and socketpair

**Parameters**

`int fd1`
:   the first file descriptor

`int fd2`
:   the second file descriptor

int \_\_audit\_sockaddr(int len, void \*a)
:   record audit data for sys\_bind, sys\_connect, sys\_sendto

**Parameters**

`int len`
:   data length in user space

`void *a`
:   data address in kernel space

**Description**

Returns 0 for success or NULL context or < 0 on error.

int audit\_signal\_info\_syscall(struct task\_struct \*t)
:   record signal info for syscalls

**Parameters**

`struct task_struct *t`
:   task being signaled

**Description**

If the audit subsystem is being terminated, record the task (pid)
and uid that is doing that.

int \_\_audit\_log\_bprm\_fcaps(struct linux\_binprm \*bprm, const struct cred \*new, const struct cred \*old)
:   store information about a loading bprm and relevant fcaps

**Parameters**

`struct linux_binprm *bprm`
:   pointer to the bprm being processed

`const struct cred *new`
:   the proposed new credentials

`const struct cred *old`
:   the old credentials

**Description**

Simply check if the proc already has the caps given by the file and if not
store the priv escalation info for later auditing at the end of the syscall

-Eric

void \_\_audit\_log\_capset(const struct cred \*new, const struct cred \*old)
:   store information about the arguments to the capset syscall

**Parameters**

`const struct cred *new`
:   the new credentials

`const struct cred *old`
:   the old (current) credentials

**Description**

Record the arguments userspace sent to sys\_capset for later printing by the
audit system if applicable

void audit\_core\_dumps(long signr)
:   record information about processes that end abnormally

**Parameters**

`long signr`
:   signal value

**Description**

If a process ends with a core dump, something fishy is going on and we
should record the event for investigation.

void audit\_seccomp(unsigned long syscall, long signr, int code)
:   record information about a seccomp action

**Parameters**

`unsigned long syscall`
:   syscall number

`long signr`
:   signal value

`int code`
:   the seccomp action

**Description**

Record the information associated with a seccomp action. Event filtering for
seccomp actions that are not to be logged is done in `seccomp_log()`.
Therefore, this function forces auditing independent of the audit\_enabled
and dummy context state because seccomp actions should be logged even when
audit is not in use.

int audit\_rule\_change(int type, int seq, void \*data, size\_t datasz)
:   apply all rules to the specified message type

**Parameters**

`int type`
:   audit message type

`int seq`
:   netlink audit message sequence (serial) number

`void *data`
:   payload data

`size_t datasz`
:   size of payload data

int audit\_list\_rules\_send(struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*request\_skb, int seq)
:   list the audit rules

**Parameters**

`struct sk_buff *request_skb`
:   skb of request we are replying to (used to target the reply)

`int seq`
:   netlink audit message sequence (serial) number

int parent\_len(const char \*path)
:   find the length of the parent portion of a pathname

**Parameters**

`const char *path`
:   pathname of which to determine length

int audit\_compare\_dname\_path(const struct qstr \*dname, const char \*path, int parentlen)
:   compare given dentry name with last component in given path. Return of 0 indicates a match.

**Parameters**

`const struct qstr *dname`
:   dentry name that we’re comparing

`const char *path`
:   full pathname that we’re comparing

`int parentlen`
:   length of the parent if known. Passing in AUDIT\_NAME\_FULL
    here indicates that we must compute this value.

## Accounting Framework

long sys\_acct(const char \_\_user \*name)
:   enable/disable process accounting

**Parameters**

`const char __user * name`
:   file name for accounting records or NULL to shutdown accounting

**Description**

[`sys_acct()`](#c.sys_acct "sys_acct") is the only system call needed to implement process
accounting. It takes the name of the file where accounting records
should be written. If the filename is NULL, accounting will be
shutdown.

**Return**

0 for success or negative errno values for failure.

void acct\_collect(long exitcode, int group\_dead)
:   collect accounting information into pacct\_struct

**Parameters**

`long exitcode`
:   task exit code

`int group_dead`
:   not 0, if this thread is the last one in the process.

void acct\_process(void)
:   handles process accounting for an exiting task

**Parameters**

`void`
:   no arguments

## Block Devices

void bio\_advance(struct [bio](#c.bio_advance "bio") \*bio, unsigned int nbytes)
:   increment/complete a bio by some number of bytes

**Parameters**

`struct bio *bio`
:   bio to advance

`unsigned int nbytes`
:   number of bytes to complete

**Description**

This updates bi\_sector, bi\_size and bi\_idx; if the number of bytes to
complete doesn’t align with a bvec boundary, then bv\_len and bv\_offset will
be updated on the last bvec as well.

**bio** will then represent the remaining, uncompleted portion of the io.

struct folio\_iter
:   State for iterating all folios in a bio.

**Definition**:

```
struct folio_iter {
    struct folio *folio;
    size_t offset;
    size_t length;
};
```

**Members**

`folio`
:   The current folio we’re iterating. NULL after the last folio.

`offset`
:   The byte offset within the current folio.

`length`
:   The number of bytes in this iteration (will not cross folio
    boundary).

bio\_for\_each\_folio\_all

`bio_for_each_folio_all (fi, bio)`

> Iterate over each folio in a bio.

**Parameters**

`fi`
:   [`struct folio_iter`](#c.folio_iter "folio_iter") which is updated for each folio.

`bio`
:   `struct bio` to iterate over.

struct [bio](#c.bio_next_split "bio") \*bio\_next\_split(struct [bio](#c.bio_next_split "bio") \*bio, int sectors, gfp\_t gfp, struct bio\_set \*bs)
:   get next **sectors** from a bio, splitting if necessary

**Parameters**

`struct bio *bio`
:   bio to split

`int sectors`
:   number of sectors to split from the front of **bio**

`gfp_t gfp`
:   gfp mask

`struct bio_set *bs`
:   bio set to allocate from

**Return**

a bio representing the next **sectors** of **bio** - if the bio is smaller
than **sectors**, returns the original bio unchanged.

unsigned short bio\_iov\_bounce\_nr\_vecs(struct iov\_iter \*iter, blk\_opf\_t op)
:   calculate number of bvecs for a bounce bio

**Parameters**

`struct iov_iter *iter`
:   iter to bounce from

`blk_opf_t op`
:   REQ\_OP\_\* for the bio

**Description**

Calculates how many bvecs are needed for the next bio to bounce from/to
**iter**.

unsigned int bio\_add\_max\_vecs(void \*kaddr, unsigned int len)
:   number of bio\_vecs needed to add data to a bio

**Parameters**

`void *kaddr`
:   kernel virtual address to add

`unsigned int len`
:   length in bytes to add

**Description**

Calculate how many bio\_vecs need to be allocated to add the kernel virtual
address range in [**kaddr**:**len**] in the worse case.

bool bio\_is\_zone\_append(struct [bio](#c.bio_is_zone_append "bio") \*bio)
:   is this a zone append bio?

**Parameters**

`struct bio *bio`
:   bio to check

**Description**

Check if **bio** is a zone append operation. Core block layer code and end\_io
handlers must use this instead of an open coded REQ\_OP\_ZONE\_APPEND check
because the block layer can rewrite REQ\_OP\_ZONE\_APPEND to REQ\_OP\_WRITE if
it is not natively supported.

void blk\_queue\_flag\_set(unsigned int flag, struct request\_queue \*q)
:   atomically set a queue flag

**Parameters**

`unsigned int flag`
:   flag to be set

`struct request_queue *q`
:   request queue

void blk\_queue\_flag\_clear(unsigned int flag, struct request\_queue \*q)
:   atomically clear a queue flag

**Parameters**

`unsigned int flag`
:   flag to be cleared

`struct request_queue *q`
:   request queue

const char \*blk\_op\_str(enum req\_op op)
:   Return the string “name” for an operation REQ\_OP\_name.

**Parameters**

`enum req_op op`
:   a request operation.

**Description**

Convert a request operation REQ\_OP\_name into the string “name”. Useful for
debugging and tracing BIOs and requests. For an invalid request operation
code, the string “UNKNOWN” is returned.

void blk\_sync\_queue(struct request\_queue \*q)
:   cancel any pending callbacks on a queue

**Parameters**

`struct request_queue *q`
:   the queue

**Description**

> The block layer may perform asynchronous callback activity
> on a queue, such as calling the unplug function after a timeout.
> A block device may call blk\_sync\_queue to ensure that any
> such activity is cancelled, thus allowing it to release resources
> that the callbacks might use. The caller must already have made sure
> that its ->submit\_bio will not re-add plugging prior to calling
> this function.
>
> This function does not cancel any asynchronous activity arising
> out of elevator or throttling code. That would require `elevator_exit()`
> and `blkcg_exit_queue()` to be called with queue lock initialized.

void blk\_set\_pm\_only(struct request\_queue \*q)
:   increment pm\_only counter

**Parameters**

`struct request_queue *q`
:   request queue pointer

void blk\_put\_queue(struct request\_queue \*q)
:   decrement the request\_queue refcount

**Parameters**

`struct request_queue *q`
:   the request\_queue structure to decrement the refcount for

**Description**

Decrements the refcount of the request\_queue and free it when the refcount
reaches 0.

bool blk\_get\_queue(struct request\_queue \*q)
:   increment the request\_queue refcount

**Parameters**

`struct request_queue *q`
:   the request\_queue structure to increment the refcount for

**Description**

Increment the refcount of the request\_queue kobject.

**Context**

Any context.

void submit\_bio\_noacct(struct [bio](#c.submit_bio_noacct "bio") \*bio)
:   re-submit a bio to the block device layer for I/O

**Parameters**

`struct bio *bio`
:   The bio describing the location in memory and on the device.

**Description**

This is a version of [`submit_bio()`](#c.submit_bio "submit_bio") that shall only be used for I/O that is
resubmitted to lower level drivers by stacking block drivers. All file
systems and other upper level users of the block layer should use
[`submit_bio()`](#c.submit_bio "submit_bio") instead.

void submit\_bio(struct [bio](#c.submit_bio "bio") \*bio)
:   submit a bio to the block device layer for I/O

**Parameters**

`struct bio *bio`
:   The `struct bio` which describes the I/O

**Description**

[`submit_bio()`](#c.submit_bio "submit_bio") is used to submit I/O requests to block devices. It is passed a
fully set up `struct bio` that describes the I/O that needs to be done. The
bio will be sent to the device described by the bi\_bdev field.

The success/failure status of the request, along with notification of
completion, is delivered asynchronously through the ->`bi_end_io()` callback
in **bio**. The bio must NOT be touched by the caller until ->`bi_end_io()` has
been called.

int bio\_poll(struct [bio](#c.bio_poll "bio") \*bio, struct io\_comp\_batch \*iob, unsigned int flags)
:   poll for BIO completions

**Parameters**

`struct bio *bio`
:   bio to poll for

`struct io_comp_batch *iob`
:   batches of IO

`unsigned int flags`
:   BLK\_POLL\_\* flags that control the behavior

**Description**

Poll for completions on queue associated with the bio. Returns number of
completed entries found.

**Note**

the caller must either be the context that submitted **bio**, or
be in a RCU critical section to prevent freeing of **bio**.

unsigned long bio\_start\_io\_acct(struct [bio](#c.bio_start_io_acct "bio") \*bio)
:   start I/O accounting for bio based drivers

**Parameters**

`struct bio *bio`
:   bio to start account for

**Description**

Returns the start time that should be passed back to `bio_end_io_acct()`.

int blk\_lld\_busy(struct request\_queue \*q)
:   Check if underlying low-level drivers of a device are busy

**Parameters**

`struct request_queue *q`
:   the queue of the device being checked

**Description**

> Check if underlying low-level drivers of a device are busy.
> If the drivers want to export their busy state, they must set own
> exporting function using `blk_queue_lld_busy()` first.
>
> Basically, this function is used only by request stacking drivers
> to stop dispatching requests to underlying devices when underlying
> devices are busy. This behavior helps more I/O merging on the queue
> of the request stacking driver and prevents I/O throughput regression
> on burst I/O load.

**Return**

0 - Not busy (The request stacking driver should dispatch request)
1 - Busy (The request stacking driver should stop dispatching request)

void blk\_start\_plug(struct blk\_plug \*plug)
:   initialize blk\_plug and track it inside the task\_struct

**Parameters**

`struct blk_plug *plug`
:   The `struct blk_plug` that needs to be initialized

**Description**

> [`blk_start_plug()`](#c.blk_start_plug "blk_start_plug") indicates to the block layer an intent by the caller
> to submit multiple I/O requests in a batch. The block layer may use
> this hint to defer submitting I/Os from the caller until [`blk_finish_plug()`](#c.blk_finish_plug "blk_finish_plug")
> is called. However, the block layer may choose to submit requests
> before a call to [`blk_finish_plug()`](#c.blk_finish_plug "blk_finish_plug") if the number of queued I/Os
> exceeds `BLK_MAX_REQUEST_COUNT`, or if the size of the I/O is larger than
> `BLK_PLUG_FLUSH_SIZE`. The queued I/Os may also be submitted early if
> the task schedules (see below).
>
> Tracking blk\_plug inside the task\_struct will help with auto-flushing the
> pending I/O should the task end up blocking between [`blk_start_plug()`](#c.blk_start_plug "blk_start_plug") and
> [`blk_finish_plug()`](#c.blk_finish_plug "blk_finish_plug"). This is important from a performance perspective, but
> also ensures that we don’t deadlock. For instance, if the task is blocking
> for a memory allocation, memory reclaim could end up wanting to free a
> page belonging to that request that is currently residing in our private
> plug. By flushing the pending I/O when the process goes to sleep, we avoid
> this kind of deadlock.

void blk\_finish\_plug(struct blk\_plug \*plug)
:   mark the end of a batch of submitted I/O

**Parameters**

`struct blk_plug *plug`
:   The `struct blk_plug` passed to [`blk_start_plug()`](#c.blk_start_plug "blk_start_plug")

**Description**

Indicate that a batch of I/O submissions is complete. This function
must be paired with an initial call to [`blk_start_plug()`](#c.blk_start_plug "blk_start_plug"). The intent
is to allow the block layer to optimize I/O submission. See the
documentation for [`blk_start_plug()`](#c.blk_start_plug "blk_start_plug") for more information.

int blk\_queue\_enter(struct request\_queue \*q, blk\_mq\_req\_flags\_t flags)
:   try to increase q->q\_usage\_counter

**Parameters**

`struct request_queue *q`
:   request queue pointer

`blk_mq_req_flags_t flags`
:   BLK\_MQ\_REQ\_NOWAIT and/or BLK\_MQ\_REQ\_PM

int blk\_rq\_map\_user\_iov(struct request\_queue \*q, struct request \*rq, struct rq\_map\_data \*map\_data, const struct iov\_iter \*iter, gfp\_t gfp\_mask)
:   map user data to a request, for passthrough requests

**Parameters**

`struct request_queue *q`
:   request queue where request should be inserted

`struct request *rq`
:   request to map data to

`struct rq_map_data *map_data`
:   pointer to the rq\_map\_data holding pages (if necessary)

`const struct iov_iter *iter`
:   iovec iterator

`gfp_t gfp_mask`
:   memory allocation flags

**Description**

> Data will be mapped directly for zero copy I/O, if possible. Otherwise
> a kernel bounce buffer is used.
>
> A matching [`blk_rq_unmap_user()`](#c.blk_rq_unmap_user "blk_rq_unmap_user") must be issued at the end of I/O, while
> still in process context.

int blk\_rq\_unmap\_user(struct [bio](#c.blk_rq_unmap_user "bio") \*bio)
:   unmap a request with user data

**Parameters**

`struct bio *bio`
:   start of bio list

**Description**

> Unmap a rq previously mapped by `blk_rq_map_user()`. The caller must
> supply the original rq->bio from the `blk_rq_map_user()` return, since
> the I/O completion may have changed rq->bio.

int blk\_rq\_map\_kern(struct request \*rq, void \*kbuf, unsigned int len, gfp\_t gfp\_mask)
:   map kernel data to a request, for passthrough requests

**Parameters**

`struct request *rq`
:   request to fill

`void *kbuf`
:   the kernel buffer

`unsigned int len`
:   length of user data

`gfp_t gfp_mask`
:   memory allocation flags

**Description**

> Data will be mapped directly if possible. Otherwise a bounce
> buffer is used. Can be called multiple times to append multiple
> buffers.

int blk\_register\_queue(struct gendisk \*disk)
:   register a block layer queue with sysfs

**Parameters**

`struct gendisk *disk`
:   Disk of which the request queue should be registered with sysfs.

void blk\_unregister\_queue(struct gendisk \*disk)
:   counterpart of [`blk_register_queue()`](#c.blk_register_queue "blk_register_queue")

**Parameters**

`struct gendisk *disk`
:   Disk of which the request queue should be unregistered from sysfs.

**Note**

the caller is responsible for guaranteeing that this function is called
after [`blk_register_queue()`](#c.blk_register_queue "blk_register_queue") has finished.

void blk\_set\_stacking\_limits(struct queue\_limits \*lim)
:   set default limits for stacking devices

**Parameters**

`struct queue_limits *lim`
:   the queue\_limits structure to reset

**Description**

Prepare queue limits for applying limits from underlying devices using
[`blk_stack_limits()`](#c.blk_stack_limits "blk_stack_limits").

int queue\_limits\_commit\_update(struct request\_queue \*q, struct queue\_limits \*lim)
:   commit an atomic update of queue limits

**Parameters**

`struct request_queue *q`
:   queue to update

`struct queue_limits *lim`
:   limits to apply

**Description**

Apply the limits in **lim** that were obtained from `queue_limits_start_update()`
and updated by the caller to **q**. The caller must have frozen the queue or
ensure that there are no outstanding I/Os by other means.

Returns 0 if successful, else a negative error code.

int queue\_limits\_commit\_update\_frozen(struct request\_queue \*q, struct queue\_limits \*lim)
:   commit an atomic update of queue limits

**Parameters**

`struct request_queue *q`
:   queue to update

`struct queue_limits *lim`
:   limits to apply

**Description**

Apply the limits in **lim** that were obtained from `queue_limits_start_update()`
and updated with the new values by the caller to **q**. Freezes the queue
before the update and unfreezes it after.

Returns 0 if successful, else a negative error code.

int queue\_limits\_set(struct request\_queue \*q, struct queue\_limits \*lim)
:   apply queue limits to queue

**Parameters**

`struct request_queue *q`
:   queue to update

`struct queue_limits *lim`
:   limits to apply

**Description**

Apply the limits in **lim** that were freshly initialized to **q**.
To update existing limits use `queue_limits_start_update()` and
[`queue_limits_commit_update()`](#c.queue_limits_commit_update "queue_limits_commit_update") instead.

Returns 0 if successful, else a negative error code.

int blk\_stack\_limits(struct queue\_limits \*t, struct queue\_limits \*b, sector\_t start)
:   adjust queue\_limits for stacked devices

**Parameters**

`struct queue_limits *t`
:   the stacking driver limits (top device)

`struct queue_limits *b`
:   the underlying queue limits (bottom, component device)

`sector_t start`
:   first data sector within component device

**Description**

> This function is used by stacking drivers like MD and DM to ensure
> that all component devices have compatible block sizes and
> alignments. The stacking driver must provide a queue\_limits
> struct (top) and then iteratively call the stacking function for
> all component (bottom) devices. The stacking function will
> attempt to combine the values and ensure proper alignment.
>
> Returns 0 if the top and bottom queue\_limits are compatible. The
> top device’s block sizes and alignment offsets may be adjusted to
> ensure alignment with the bottom device. If no compatible sizes
> and alignments exist, -1 is returned and the resulting top
> queue\_limits will have the misaligned flag set to indicate that
> the alignment\_offset is undefined.

void queue\_limits\_stack\_bdev(struct queue\_limits \*t, struct block\_device \*bdev, sector\_t offset, const char \*pfx)
:   adjust queue\_limits for stacked devices

**Parameters**

`struct queue_limits *t`
:   the stacking driver limits (top device)

`struct block_device *bdev`
:   the underlying block device (bottom)

`sector_t offset`
:   offset to beginning of data within component device

`const char *pfx`
:   prefix to use for warnings logged

**Description**

> This function is used by stacking drivers like MD and DM to ensure
> that all component devices have compatible block sizes and
> alignments. The stacking driver must provide a queue\_limits
> struct (top) and then iteratively call the stacking function for
> all component (bottom) devices. The stacking function will
> attempt to combine the values and ensure proper alignment.

bool queue\_limits\_stack\_integrity(struct queue\_limits \*t, struct queue\_limits \*b)
:   stack integrity profile

**Parameters**

`struct queue_limits *t`
:   target queue limits

`struct queue_limits *b`
:   base queue limits

**Description**

Check if the integrity profile in the **b** can be stacked into the
target **t**. Stacking is possible if either:

> 1. does not have any integrity information stacked into it yet
> 2. the integrity profile in **b** is identical to the one in **t**

If **b** can be stacked into **t**, return `true`. Else return `false` and clear the
integrity information in **t**.

void blk\_set\_queue\_depth(struct request\_queue \*q, unsigned int depth)
:   tell the block layer about the device queue depth

**Parameters**

`struct request_queue *q`
:   the request queue for the device

`unsigned int depth`
:   queue depth

int blkdev\_issue\_flush(struct block\_device \*bdev)
:   queue a flush

**Parameters**

`struct block_device *bdev`
:   blockdev to issue flush for

**Description**

> Issue a flush for the block device in question.

int blkdev\_issue\_discard(struct block\_device \*bdev, sector\_t sector, sector\_t nr\_sects, gfp\_t gfp\_mask)
:   queue a discard

**Parameters**

`struct block_device *bdev`
:   blockdev to issue discard for

`sector_t sector`
:   start sector

`sector_t nr_sects`
:   number of sectors to discard

`gfp_t gfp_mask`
:   memory allocation flags (for bio\_alloc)

**Description**

> Issue a discard request for the sectors in question.

int \_\_blkdev\_issue\_zeroout(struct block\_device \*bdev, sector\_t sector, sector\_t nr\_sects, gfp\_t gfp\_mask, struct bio \*\*biop, unsigned flags)
:   generate number of zero filed write bios

**Parameters**

`struct block_device *bdev`
:   blockdev to issue

`sector_t sector`
:   start sector

`sector_t nr_sects`
:   number of sectors to write

`gfp_t gfp_mask`
:   memory allocation flags (for bio\_alloc)

`struct bio **biop`
:   pointer to anchor bio

`unsigned flags`
:   controls detailed behavior

**Description**

> Zero-fill a block range, either using hardware offload or by explicitly
> writing zeroes to the device.
>
> If a device is using logical block provisioning, the underlying space will
> not be released if `flags` contains BLKDEV\_ZERO\_NOUNMAP.
>
> If `flags` contains BLKDEV\_ZERO\_NOFALLBACK, the function will return
> -EOPNOTSUPP if no explicit hardware offload for zeroing is provided.

int blkdev\_issue\_zeroout(struct block\_device \*bdev, sector\_t sector, sector\_t nr\_sects, gfp\_t gfp\_mask, unsigned flags)
:   zero-fill a block range

**Parameters**

`struct block_device *bdev`
:   blockdev to write

`sector_t sector`
:   start sector

`sector_t nr_sects`
:   number of sectors to write

`gfp_t gfp_mask`
:   memory allocation flags (for bio\_alloc)

`unsigned flags`
:   controls detailed behavior

**Description**

> Zero-fill a block range, either using hardware offload or by explicitly
> writing zeroes to the device. See [`__blkdev_issue_zeroout()`](#c.__blkdev_issue_zeroout "__blkdev_issue_zeroout") for the
> valid values for `flags`.

int blk\_trace\_ioctl(struct block\_device \*bdev, unsigned cmd, char \_\_user \*arg)
:   handle the ioctls associated with tracing

**Parameters**

`struct block_device *bdev`
:   the block device

`unsigned cmd`
:   the ioctl cmd

`char __user *arg`
:   the argument data, if any

void blk\_trace\_shutdown(struct request\_queue \*q)
:   stop and cleanup trace structures

**Parameters**

`struct request_queue *q`
:   the request queue associated with the device

void blk\_add\_trace\_rq(struct request \*rq, blk\_status\_t error, unsigned int nr\_bytes, u64 what, u64 cgid)
:   Add a trace for a request oriented action

**Parameters**

`struct request *rq`
:   the source request

`blk_status_t error`
:   return status to log

`unsigned int nr_bytes`
:   number of completed bytes

`u64 what`
:   the action

`u64 cgid`
:   the cgroup info

**Description**

> Records an action against a request. Will log the bio offset + size.

void blk\_add\_trace\_bio(struct request\_queue \*q, struct [bio](#c.blk_add_trace_bio "bio") \*bio, u64 what, int error)
:   Add a trace for a bio oriented action

**Parameters**

`struct request_queue *q`
:   queue the io is for

`struct bio *bio`
:   the source bio

`u64 what`
:   the action

`int error`
:   error, if any

**Description**

> Records an action against a bio. Will log the bio offset + size.

void blk\_add\_trace\_bio\_remap(void \*ignore, struct [bio](#c.blk_add_trace_bio_remap "bio") \*bio, dev\_t dev, sector\_t from)
:   Add a trace for a bio-remap operation

**Parameters**

`void *ignore`
:   trace callback data parameter (not used)

`struct bio *bio`
:   the source bio

`dev_t dev`
:   source device

`sector_t from`
:   source sector

**Description**

Called after a bio is remapped to a different device and/or sector.

void blk\_add\_trace\_rq\_remap(void \*ignore, struct request \*rq, dev\_t dev, sector\_t from)
:   Add a trace for a request-remap operation

**Parameters**

`void *ignore`
:   trace callback data parameter (not used)

`struct request *rq`
:   the source request

`dev_t dev`
:   target device

`sector_t from`
:   source sector

**Description**

> Device mapper remaps request to other devices.
> Add a trace for that action.

void disk\_release(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   releases all allocated resources of the gendisk

**Parameters**

`struct device *dev`
:   the device representing this disk

**Description**

This function releases all allocated resources of the gendisk.

Drivers which used [`device_add_disk()`](#c.device_add_disk "device_add_disk") have a gendisk with a request\_queue
assigned. Since the request\_queue sits on top of the gendisk for these
drivers we also call [`blk_put_queue()`](#c.blk_put_queue "blk_put_queue") for them, and we expect the
request\_queue refcount to reach 0 at this point, and so the request\_queue
will also be freed prior to the disk.

**Context**

can sleep

unsigned int bdev\_count\_inflight(struct block\_device \*part)
:   get the number of inflight IOs for a block device.

**Parameters**

`struct block_device *part`
:   the block device.

**Description**

Inflight here means started IO accounting, from `bdev_start_io_acct()` for
bio-based block device, and from `blk_account_io_start()` for rq-based block
device.

int \_\_register\_blkdev(unsigned int major, const char \*name, void (\*probe)(dev\_t devt))
:   register a new block device

**Parameters**

`unsigned int major`
:   the requested major device number [1..BLKDEV\_MAJOR\_MAX-1]. If
    **major** = 0, try to allocate any unused major number.

`const char *name`
:   the name of the new block device as a zero terminated string

`void (*probe)(dev_t devt)`
:   pre-devtmpfs / pre-udev callback used to create disks when their
    pre-created device node is accessed. When a probe call uses
    `add_disk()` and it fails the driver must cleanup resources. This
    interface may soon be removed.

**Description**

The **name** must be unique within the system.

The return value depends on the **major** input parameter:

> * if a major device number was requested in range [1..BLKDEV\_MAJOR\_MAX-1]
>   then the function returns zero on success, or a negative error code
> * if any unused major number was requested with **major** = 0 parameter
>   then the return value is the allocated major number in range
>   [1..BLKDEV\_MAJOR\_MAX-1] or a negative error code otherwise

See [Linux allocated devices (4.x+ version)](../admin-guide/devices.html) for the list of allocated
major numbers.

Use register\_blkdev instead for any new code.

int add\_disk\_fwnode(struct [device](../driver-api/infrastructure.html#c.device "device") \*parent, struct gendisk \*disk, const struct attribute\_group \*\*groups, struct fwnode\_handle \*fwnode)
:   add disk information to kernel list with fwnode

**Parameters**

`struct device *parent`
:   parent device for the disk

`struct gendisk *disk`
:   per-device partitioning information

`const struct attribute_group **groups`
:   Additional per-device sysfs groups

`struct fwnode_handle *fwnode`
:   attached disk fwnode

**Description**

This function registers the partitioning information in **disk**
with the kernel. Also attach a fwnode to the disk device.

int device\_add\_disk(struct [device](../driver-api/infrastructure.html#c.device "device") \*parent, struct gendisk \*disk, const struct attribute\_group \*\*groups)
:   add disk information to kernel list

**Parameters**

`struct device *parent`
:   parent device for the disk

`struct gendisk *disk`
:   per-device partitioning information

`const struct attribute_group **groups`
:   Additional per-device sysfs groups

**Description**

This function registers the partitioning information in **disk**
with the kernel.

void blk\_mark\_disk\_dead(struct gendisk \*disk)
:   mark a disk as dead

**Parameters**

`struct gendisk *disk`
:   disk to mark as dead

**Description**

Mark as disk as dead (e.g. surprise removed) and don’t accept any new I/O
to this disk.

void del\_gendisk(struct gendisk \*disk)
:   remove the gendisk

**Parameters**

`struct gendisk *disk`
:   the `struct gendisk` to remove

**Description**

Removes the gendisk and all its associated resources. This deletes the
partitions associated with the gendisk, and unregisters the associated
request\_queue.

This is the counter to the respective [`device_add_disk()`](#c.device_add_disk "device_add_disk") call.

The final removal of the `struct gendisk` happens when its refcount reaches 0
with [`put_disk()`](#c.put_disk "put_disk"), which should be called after [`del_gendisk()`](#c.del_gendisk "del_gendisk"), if
[`device_add_disk()`](#c.device_add_disk "device_add_disk") was used.

Drivers exist which depend on the release of the gendisk to be synchronous,
it should not be deferred.

**Context**

can sleep

void invalidate\_disk(struct gendisk \*disk)
:   invalidate the disk

**Parameters**

`struct gendisk *disk`
:   the `struct gendisk` to invalidate

**Description**

A helper to invalidates the disk. It will clean the disk’s associated
buffer/page caches and reset its internal states so that the disk
can be reused by the drivers.

**Context**

can sleep

void put\_disk(struct gendisk \*disk)
:   decrements the gendisk refcount

**Parameters**

`struct gendisk *disk`
:   the `struct gendisk` to decrement the refcount for

**Description**

This decrements the refcount for the `struct gendisk`. When this reaches 0
we’ll have [`disk_release()`](#c.disk_release "disk_release") called.

**Note**

for blk-mq disk put\_disk must be called before freeing the tag\_set
when handling probe errors (that is before `add_disk()` is called).

**Context**

Any context, but the last reference must not be dropped from
atomic context.

void set\_disk\_ro(struct gendisk \*disk, bool read\_only)
:   set a gendisk read-only

**Parameters**

`struct gendisk *disk`
:   gendisk to operate on

`bool read_only`
:   `true` to set the disk read-only, `false` set the disk read/write

**Description**

This function is used to indicate whether a given disk device should have its
read-only flag set. [`set_disk_ro()`](#c.set_disk_ro "set_disk_ro") is typically used by device drivers to
indicate whether the underlying physical device is write-protected.

int bdev\_validate\_blocksize(struct block\_device \*bdev, int block\_size)
:   check that this block size is acceptable

**Parameters**

`struct block_device *bdev`
:   blockdevice to check

`int block_size`
:   block size to check

**Description**

For block device users that do not use buffer heads or the block device
page cache, make sure that this block size can be used with the device.

**Return**

On success zero is returned, negative error code on failure.

int bdev\_freeze(struct block\_device \*bdev)
:   lock a filesystem and force it into a consistent state

**Parameters**

`struct block_device *bdev`
:   blockdevice to lock

**Description**

If a superblock is found on this device, we take the s\_umount semaphore
on it to make sure nobody unmounts until the snapshot creation is done.
The reference counter (bd\_fsfreeze\_count) guarantees that only the last
unfreeze process can unfreeze the frozen filesystem actually when multiple
freeze requests arrive simultaneously. It counts up in [`bdev_freeze()`](#c.bdev_freeze "bdev_freeze") and
count down in [`bdev_thaw()`](#c.bdev_thaw "bdev_thaw"). When it becomes 0, `thaw_bdev()` will unfreeze
actually.

**Return**

On success zero is returned, negative error code on failure.

int bdev\_thaw(struct block\_device \*bdev)
:   unlock filesystem

**Parameters**

`struct block_device *bdev`
:   blockdevice to unlock

**Description**

Unlocks the filesystem and marks it writeable again after [`bdev_freeze()`](#c.bdev_freeze "bdev_freeze").

**Return**

On success zero is returned, negative error code on failure.

int bd\_prepare\_to\_claim(struct block\_device \*bdev, void \*holder, const struct blk\_holder\_ops \*hops)
:   claim a block device

**Parameters**

`struct block_device *bdev`
:   block device of interest

`void *holder`
:   holder trying to claim **bdev**

`const struct blk_holder_ops *hops`
:   holder ops.

**Description**

Claim **bdev**. This function fails if **bdev** is already claimed by another
holder and waits if another claiming is in progress. return, the caller
has ownership of bd\_claiming and bd\_holder[s].

**Return**

0 if **bdev** can be claimed, -EBUSY otherwise.

void bd\_abort\_claiming(struct block\_device \*bdev, void \*holder)
:   abort claiming of a block device

**Parameters**

`struct block_device *bdev`
:   block device of interest

`void *holder`
:   holder that has claimed **bdev**

**Description**

Abort claiming of a block device when the exclusive open failed. This can be
also used when exclusive open is not actually desired and we just needed
to block other exclusive openers for a while.

void bdev\_fput(struct [file](../filesystems/api-summary.html#c.file "file") \*bdev\_file)
:   yield claim to the block device and put the file

**Parameters**

`struct file *bdev_file`
:   open block device

**Description**

Yield claim on the block device and put the file. Ensure that the
block device can be reclaimed before the file is closed which is a
deferred operation.

int lookup\_bdev(const char \*pathname, dev\_t \*dev)
:   Look up a `struct block_device` by name.

**Parameters**

`const char *pathname`
:   Name of the block device in the filesystem.

`dev_t *dev`
:   Pointer to the block device’s dev\_t, if found.

**Description**

Lookup the block device’s dev\_t at **pathname** in the current
namespace if possible and return it in **dev**.

**Context**

May sleep.

**Return**

0 if succeeded, negative errno otherwise.

void bdev\_mark\_dead(struct block\_device \*bdev, bool surprise)
:   mark a block device as dead

**Parameters**

`struct block_device *bdev`
:   block device to operate on

`bool surprise`
:   indicate a surprise removal

**Description**

Tell the file system that this devices or media is dead. If **surprise** is set
to `true` the device or media is already gone, if not we are preparing for an
orderly removal.

This calls into the file system, which then typicall syncs out all dirty data
and writes back inodes and then invalidates any cached data in the inodes on
the file system. In addition we also invalidate the block device mapping.

## Char devices

int register\_chrdev\_region(dev\_t from, unsigned count, const char \*name)
:   register a range of device numbers

**Parameters**

`dev_t from`
:   the first in the desired range of device numbers; must include
    the major number.

`unsigned count`
:   the number of consecutive device numbers required

`const char *name`
:   the name of the device or driver.

**Description**

Return value is zero on success, a negative error code on failure.

int alloc\_chrdev\_region(dev\_t \*dev, unsigned baseminor, unsigned count, const char \*name)
:   register a range of char device numbers

**Parameters**

`dev_t *dev`
:   output parameter for first assigned number

`unsigned baseminor`
:   first of the requested range of minor numbers

`unsigned count`
:   the number of minor numbers required

`const char *name`
:   the name of the associated device or driver

**Description**

Allocates a range of char device numbers. The major number will be
chosen dynamically, and returned (along with the first minor number)
in **dev**. Returns zero or a negative error code.

int \_\_register\_chrdev(unsigned int major, unsigned int baseminor, unsigned int count, const char \*name, const struct file\_operations \*fops)
:   create and register a cdev occupying a range of minors

**Parameters**

`unsigned int major`
:   major device number or 0 for dynamic allocation

`unsigned int baseminor`
:   first of the requested range of minor numbers

`unsigned int count`
:   the number of minor numbers required

`const char *name`
:   name of this range of devices

`const struct file_operations *fops`
:   file operations associated with this devices

**Description**

If **major** == 0 this functions will dynamically allocate a major and return
its number.

If **major** > 0 this function will attempt to reserve a device with the given
major number and will return zero on success.

Returns a -ve errno on failure.

The name of this device has nothing to do with the name of the device in
/dev. It only helps to keep track of the different owners of devices. If
your module name has only one type of devices it’s ok to use e.g. the name
of the module here.

void unregister\_chrdev\_region(dev\_t from, unsigned count)
:   unregister a range of device numbers

**Parameters**

`dev_t from`
:   the first in the range of numbers to unregister

`unsigned count`
:   the number of device numbers to unregister

**Description**

This function will unregister a range of **count** device numbers,
starting with **from**. The caller should normally be the one who
allocated those numbers in the first place...

void \_\_unregister\_chrdev(unsigned int major, unsigned int baseminor, unsigned int count, const char \*name)
:   unregister and destroy a cdev

**Parameters**

`unsigned int major`
:   major device number

`unsigned int baseminor`
:   first of the range of minor numbers

`unsigned int count`
:   the number of minor numbers this cdev is occupying

`const char *name`
:   name of this range of devices

**Description**

Unregister and destroy the cdev occupying the region described by
**major**, **baseminor** and **count**. This function undoes what
[`__register_chrdev()`](#c.__register_chrdev "__register_chrdev") did.

int cdev\_add(struct cdev \*p, dev\_t dev, unsigned count)
:   add a char device to the system

**Parameters**

`struct cdev *p`
:   the cdev structure for the device

`dev_t dev`
:   the first device number for which this device is responsible

`unsigned count`
:   the number of consecutive minor numbers corresponding to this
    device

**Description**

[`cdev_add()`](#c.cdev_add "cdev_add") adds the device represented by **p** to the system, making it
live immediately. A negative error code is returned on failure.

void cdev\_set\_parent(struct cdev \*p, struct kobject \*kobj)
:   set the parent kobject for a char device

**Parameters**

`struct cdev *p`
:   the cdev structure

`struct kobject *kobj`
:   the kobject to take a reference to

**Description**

[`cdev_set_parent()`](#c.cdev_set_parent "cdev_set_parent") sets a parent kobject which will be referenced
appropriately so the parent is not freed before the cdev. This
should be called before cdev\_add.

int cdev\_device\_add(struct [cdev](#c.cdev_device_add "cdev") \*cdev, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   add a char device and it’s corresponding [`struct device`](../driver-api/infrastructure.html#c.device "device"), linkink

**Parameters**

`struct cdev *cdev`
:   the cdev structure

`struct device *dev`
:   the device structure

**Description**

[`cdev_device_add()`](#c.cdev_device_add "cdev_device_add") adds the char device represented by **cdev** to the system,
just as cdev\_add does. It then adds **dev** to the system using device\_add
The dev\_t for the char device will be taken from the [`struct device`](../driver-api/infrastructure.html#c.device "device") which
needs to be initialized first. This helper function correctly takes a
reference to the parent device so the parent will not get released until
all references to the cdev are released.

This helper uses dev->devt for the device number. If it is not set
it will not add the cdev and it will be equivalent to device\_add.

This function should be used whenever the `struct cdev` and the
[`struct device`](../driver-api/infrastructure.html#c.device "device") are members of the same structure whose lifetime is
managed by the [`struct device`](../driver-api/infrastructure.html#c.device "device").

**NOTE**

Callers must assume that userspace was able to open the cdev and
can call cdev fops callbacks at any time, even if this function fails.

void cdev\_device\_del(struct [cdev](#c.cdev_device_del "cdev") \*cdev, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   inverse of cdev\_device\_add

**Parameters**

`struct cdev *cdev`
:   the cdev structure

`struct device *dev`
:   the device structure

**Description**

[`cdev_device_del()`](#c.cdev_device_del "cdev_device_del") is a helper function to call cdev\_del and device\_del.
It should be used whenever cdev\_device\_add is used.

If dev->devt is not set it will not remove the cdev and will be equivalent
to device\_del.

**NOTE**

This guarantees that associated sysfs callbacks are not running
or runnable, however any cdevs already open will remain and their fops
will still be callable even after this function returns.

void cdev\_del(struct cdev \*p)
:   remove a cdev from the system

**Parameters**

`struct cdev *p`
:   the cdev structure to be removed

**Description**

[`cdev_del()`](#c.cdev_del "cdev_del") removes **p** from the system, possibly freeing the structure
itself.

**NOTE**

This guarantees that cdev device will no longer be able to be
opened, however any cdevs already open will remain and their fops will
still be callable even after cdev\_del returns.

struct cdev \*cdev\_alloc(void)
:   allocate a cdev structure

**Parameters**

`void`
:   no arguments

**Description**

Allocates and returns a cdev structure, or NULL on failure.

void cdev\_init(struct [cdev](#c.cdev_init "cdev") \*cdev, const struct file\_operations \*fops)
:   initialize a cdev structure

**Parameters**

`struct cdev *cdev`
:   the structure to initialize

`const struct file_operations *fops`
:   the file\_operations for this device

**Description**

Initializes **cdev**, remembering **fops**, making it ready to add to the
system with [`cdev_add()`](#c.cdev_add "cdev_add").

## Clock Framework

The clock framework defines programming interfaces to support software
management of the system clock tree. This framework is widely used with
System-On-Chip (SOC) platforms to support power management and various
devices which may need custom clock rates. Note that these “clocks”
don’t relate to timekeeping or real time clocks (RTCs), each of which
have separate frameworks. These `struct clk`
instances may be used to manage for example a 96 MHz signal that is used
to shift bits into and out of peripherals or busses, or otherwise
trigger synchronous state machine transitions in system hardware.

Power management is supported by explicit software clock gating: unused
clocks are disabled, so the system doesn’t waste power changing the
state of transistors that aren’t in active use. On some systems this may
be backed by hardware clock gating, where clocks are gated without being
disabled in software. Sections of chips that are powered but not clocked
may be able to retain their last state. This low power state is often
called a *retention mode*. This mode still incurs leakage currents,
especially with finer circuit geometries, but for CMOS circuits power is
mostly used by clocked state changes.

Power-aware drivers only enable their clocks when the device they manage
is in active use. Also, system sleep states often differ according to
which clock domains are active: while a “standby” state may allow wakeup
from several active domains, a “mem” (suspend-to-RAM) state may require
a more wholesale shutdown of clocks derived from higher speed PLLs and
oscillators, limiting the number of possible wakeup event sources. A
driver’s suspend method may need to be aware of system-specific clock
constraints on the target sleep state.

Some platforms support programmable clock generators. These can be used
by external chips of various kinds, such as other CPUs, multimedia
codecs, and devices with strict requirements for interface clocking.

struct clk\_notifier
:   associate a clk with a notifier

**Definition**:

```
struct clk_notifier {
    struct clk                      *clk;
    struct srcu_notifier_head       notifier_head;
    struct list_head                node;
};
```

**Members**

`clk`
:   `struct clk` \* to associate the notifier with

`notifier_head`
:   a blocking\_notifier\_head for this clk

`node`
:   linked list pointers

**Description**

A list of [`struct clk_notifier`](#c.clk_notifier "clk_notifier") is maintained by the notifier code.
An entry is created whenever code registers the first notifier on a
particular **clk**. Future notifiers on that **clk** are added to the
**notifier\_head**.

struct clk\_notifier\_data
:   rate data to pass to the notifier callback

**Definition**:

```
struct clk_notifier_data {
    struct clk              *clk;
    unsigned long           old_rate;
    unsigned long           new_rate;
};
```

**Members**

`clk`
:   `struct clk` \* being changed

`old_rate`
:   previous rate of this clk

`new_rate`
:   new rate of this clk

**Description**

For a pre-notifier, old\_rate is the clk’s rate before this rate
change, and new\_rate is what the rate will be in the future. For a
post-notifier, old\_rate and new\_rate are both set to the clk’s
current rate (this was done to optimize the implementation).

struct clk\_bulk\_data
:   Data used for bulk clk operations.

**Definition**:

```
struct clk_bulk_data {
    const char              *id;
    struct clk              *clk;
};
```

**Members**

`id`
:   clock consumer ID

`clk`
:   `struct clk` \* to store the associated clock

**Description**

The CLK APIs provide a series of `clk_bulk_()` API calls as
a convenience to consumers which require multiple clks. This
structure is used to manage data for these calls.

int clk\_notifier\_register(struct [clk](#c.clk_notifier_register "clk") \*clk, struct notifier\_block \*nb)
:   register a clock rate-change notifier callback

**Parameters**

`struct clk *clk`
:   clock whose rate we are interested in

`struct notifier_block *nb`
:   notifier block with callback function pointer

**Description**

ProTip: debugging across notifier chains can be frustrating. Make sure that
your notifier callback function prints a nice big warning in case of
failure.

int clk\_notifier\_unregister(struct [clk](#c.clk_notifier_unregister "clk") \*clk, struct notifier\_block \*nb)
:   unregister a clock rate-change notifier callback

**Parameters**

`struct clk *clk`
:   clock whose rate we are no longer interested in

`struct notifier_block *nb`
:   notifier block which will be unregistered

int devm\_clk\_notifier\_register(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [clk](#c.devm_clk_notifier_register "clk") \*clk, struct notifier\_block \*nb)
:   register a managed rate-change notifier callback

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`struct clk *clk`
:   clock whose rate we are interested in

`struct notifier_block *nb`
:   notifier block with callback function pointer

**Description**

Returns 0 on success, -EERROR otherwise

long clk\_get\_accuracy(struct [clk](#c.clk_get_accuracy "clk") \*clk)
:   obtain the clock accuracy in ppb (parts per billion) for a clock source.

**Parameters**

`struct clk *clk`
:   clock source

**Description**

This gets the clock source accuracy expressed in ppb.
A perfect clock returns 0.

int clk\_set\_phase(struct [clk](#c.clk_set_phase "clk") \*clk, int degrees)
:   adjust the phase shift of a clock signal

**Parameters**

`struct clk *clk`
:   clock signal source

`int degrees`
:   number of degrees the signal is shifted

**Description**

Shifts the phase of a clock signal by the specified degrees. Returns 0 on
success, -EERROR otherwise.

int clk\_get\_phase(struct [clk](#c.clk_get_phase "clk") \*clk)
:   return the phase shift of a clock signal

**Parameters**

`struct clk *clk`
:   clock signal source

**Description**

Returns the phase shift of a clock node in degrees, otherwise returns
-EERROR.

int clk\_set\_duty\_cycle(struct [clk](#c.clk_set_duty_cycle "clk") \*clk, unsigned int num, unsigned int den)
:   adjust the duty cycle ratio of a clock signal

**Parameters**

`struct clk *clk`
:   clock signal source

`unsigned int num`
:   numerator of the duty cycle ratio to be applied

`unsigned int den`
:   denominator of the duty cycle ratio to be applied

**Description**

Adjust the duty cycle of a clock signal by the specified ratio. Returns 0 on
success, -EERROR otherwise.

int clk\_get\_scaled\_duty\_cycle(struct [clk](#c.clk_get_scaled_duty_cycle "clk") \*clk, unsigned int scale)
:   return the duty cycle ratio of a clock signal

**Parameters**

`struct clk *clk`
:   clock signal source

`unsigned int scale`
:   scaling factor to be applied to represent the ratio as an integer

**Description**

Returns the duty cycle ratio multiplied by the scale provided, otherwise
returns -EERROR.

bool clk\_is\_match(const struct clk \*p, const struct clk \*q)
:   check if two clk’s point to the same hardware clock

**Parameters**

`const struct clk *p`
:   clk compared against q

`const struct clk *q`
:   clk compared against p

**Description**

Returns true if the two `struct clk` pointers both point to the same hardware
clock node. Put differently, returns true if **p** and **q**
share the same `struct clk_core` object.

Returns false otherwise. Note that two NULL clks are treated as matching.

int clk\_rate\_exclusive\_get(struct [clk](#c.clk_rate_exclusive_get "clk") \*clk)
:   get exclusivity over the rate control of a producer

**Parameters**

`struct clk *clk`
:   clock source

**Description**

This function allows drivers to get exclusive control over the rate of a
provider. It prevents any other consumer to execute, even indirectly,
opereation which could alter the rate of the provider or cause glitches

If exlusivity is claimed more than once on clock, even by the same driver,
the rate effectively gets locked as exclusivity can’t be preempted.

Must not be called from within atomic context.

Returns success (0) or negative errno.

int devm\_clk\_rate\_exclusive\_get(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [clk](#c.devm_clk_rate_exclusive_get "clk") \*clk)
:   devm variant of clk\_rate\_exclusive\_get

**Parameters**

`struct device *dev`
:   device the exclusivity is bound to

`struct clk *clk`
:   clock source

**Description**

Calls [`clk_rate_exclusive_get()`](#c.clk_rate_exclusive_get "clk_rate_exclusive_get") on **clk** and registers a devm cleanup handler
on **dev** to call [`clk_rate_exclusive_put()`](#c.clk_rate_exclusive_put "clk_rate_exclusive_put").

Must not be called from within atomic context.

void clk\_rate\_exclusive\_put(struct [clk](#c.clk_rate_exclusive_put "clk") \*clk)
:   release exclusivity over the rate control of a producer

**Parameters**

`struct clk *clk`
:   clock source

**Description**

This function allows drivers to release the exclusivity it previously got
from [`clk_rate_exclusive_get()`](#c.clk_rate_exclusive_get "clk_rate_exclusive_get")

The caller must balance the number of [`clk_rate_exclusive_get()`](#c.clk_rate_exclusive_get "clk_rate_exclusive_get") and
[`clk_rate_exclusive_put()`](#c.clk_rate_exclusive_put "clk_rate_exclusive_put") calls.

Must not be called from within atomic context.

int clk\_save\_context(void)
:   save clock context for poweroff

**Parameters**

`void`
:   no arguments

**Description**

Saves the context of the clock register for powerstates in which the
contents of the registers will be lost. Occurs deep within the suspend
code so locking is not necessary.

void clk\_restore\_context(void)
:   restore clock context after poweroff

**Parameters**

`void`
:   no arguments

**Description**

This occurs with all clocks enabled. Occurs deep within the resume code
so locking is not necessary.

int clk\_prepare(struct [clk](#c.clk_prepare "clk") \*clk)
:   prepare a clock source

**Parameters**

`struct clk *clk`
:   clock source

**Description**

This prepares the clock source for use.

Must not be called from within atomic context.

void clk\_unprepare(struct [clk](#c.clk_unprepare "clk") \*clk)
:   undo preparation of a clock source

**Parameters**

`struct clk *clk`
:   clock source

**Description**

This undoes a previously prepared clock. The caller must balance
the number of prepare and unprepare calls.

Must not be called from within atomic context.

bool clk\_is\_enabled\_when\_prepared(struct [clk](#c.clk_is_enabled_when_prepared "clk") \*clk)
:   indicate if preparing a clock also enables it.

**Parameters**

`struct clk *clk`
:   clock source

**Description**

Returns true if [`clk_prepare()`](#c.clk_prepare "clk_prepare") implicitly enables the clock, effectively
making [`clk_enable()`](#c.clk_enable "clk_enable")/[`clk_disable()`](#c.clk_disable "clk_disable") no-ops, false otherwise.

This is of interest mainly to the power management code where actually
disabling the clock also requires unpreparing it to have any material
effect.

Regardless of the value returned here, the caller must always invoke
[`clk_enable()`](#c.clk_enable "clk_enable") or `clk_prepare_enable()` and counterparts for usage counts
to be right.

struct clk \*clk\_get(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   lookup and obtain a reference to a clock producer.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Description**

Returns a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. (IOW, **id** may be identical strings, but
clk\_get may return different clock producers depending on **dev**.)

Drivers must assume that the clock source is not enabled.

clk\_get should not be called from within interrupt context.

int clk\_bulk\_get(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   lookup and obtain a number of references to clock producer.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Description**

This helper function allows drivers to get several clk consumers in one
operation. If any of the clk cannot be acquired then any clks
that were obtained will be freed before returning to the caller.

Returns 0 if all clocks specified in clk\_bulk\_data table are obtained
successfully, or valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno.
The implementation uses **dev** and **clk\_bulk\_data.id** to determine the
clock consumer, and thereby the clock producer.
The clock returned is stored in each **clk\_bulk\_data.clk** field.

Drivers must assume that the clock source is not enabled.

clk\_bulk\_get should not be called from within interrupt context.

int clk\_bulk\_get\_all(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*\*clks)
:   lookup and obtain all available references to clock producer.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`struct clk_bulk_data **clks`
:   pointer to the clk\_bulk\_data table of consumer

**Description**

This helper function allows drivers to get all clk consumers in one
operation. If any of the clk cannot be acquired then any clks
that were obtained will be freed before returning to the caller.

Returns a positive value for the number of clocks obtained while the
clock references are stored in the clk\_bulk\_data table in **clks** field.
Returns 0 if there’re none and a negative value if something failed.

Drivers must assume that the clock source is not enabled.

clk\_bulk\_get should not be called from within interrupt context.

int clk\_bulk\_get\_optional(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   lookup and obtain a number of references to clock producer

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Description**

Behaves the same as [`clk_bulk_get()`](#c.clk_bulk_get "clk_bulk_get") except where there is no clock producer.
In this case, instead of returning -ENOENT, the function returns 0 and
NULL for a clk for which a clock producer could not be determined.

int devm\_clk\_bulk\_get(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   managed get multiple clk consumers

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Description**

Return 0 on success, an errno on failure.

This helper function allows drivers to get several clk
consumers in one operation with management, the clks will
automatically be freed when the device is unbound.

int devm\_clk\_bulk\_get\_optional(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   managed get multiple optional consumer clocks

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   pointer to the clk\_bulk\_data table of consumer

**Description**

Behaves the same as [`devm_clk_bulk_get()`](#c.devm_clk_bulk_get "devm_clk_bulk_get") except where there is no clock
producer. In this case, instead of returning -ENOENT, the function returns
NULL for given clk. It is assumed all clocks in clk\_bulk\_data are optional.

Returns 0 if all clocks specified in clk\_bulk\_data table are obtained
successfully or for any clk there was no clk provider available, otherwise
returns valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno.
The implementation uses **dev** and **clk\_bulk\_data.id** to determine the
clock consumer, and thereby the clock producer.
The clock returned is stored in each **clk\_bulk\_data.clk** field.

Drivers must assume that the clock source is not enabled.

clk\_bulk\_get should not be called from within interrupt context.

int devm\_clk\_bulk\_get\_optional\_enable(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   Get and enable optional bulk clocks (managed)

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   pointer to the clk\_bulk\_data table of consumer

**Description**

Behaves the same as [`devm_clk_bulk_get_optional()`](#c.devm_clk_bulk_get_optional "devm_clk_bulk_get_optional") but also prepares and enables
the clocks in one operation with management. The clks will automatically be
disabled, unprepared and freed when the device is unbound.

**Return**

0 if all clocks specified in clk\_bulk\_data table are obtained
and enabled successfully, or for any clk there was no clk provider available.
Otherwise returns valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno.

int devm\_clk\_bulk\_get\_all(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*\*clks)
:   managed get multiple clk consumers

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`struct clk_bulk_data **clks`
:   pointer to the clk\_bulk\_data table of consumer

**Description**

Returns a positive value for the number of clocks obtained while the
clock references are stored in the clk\_bulk\_data table in **clks** field.
Returns 0 if there’re none and a negative value if something failed.

This helper function allows drivers to get several clk
consumers in one operation with management, the clks will
automatically be freed when the device is unbound.

int devm\_clk\_bulk\_get\_all\_enabled(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*\*clks)
:   Get and enable all clocks of the consumer (managed)

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`struct clk_bulk_data **clks`
:   pointer to the clk\_bulk\_data table of consumer

**Description**

Returns a positive value for the number of clocks obtained while the
clock references are stored in the clk\_bulk\_data table in **clks** field.
Returns 0 if there’re none and a negative value if something failed.

This helper function allows drivers to get all clocks of the
consumer and enables them in one operation with management.
The clks will automatically be disabled and freed when the device
is unbound.

struct clk \*devm\_clk\_get(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   lookup and obtain a managed reference to a clock producer.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. (IOW, **id** may be identical strings, but
clk\_get may return different clock producers depending on **dev**.)

**Description**

Drivers must assume that the clock source is neither prepared nor
enabled.

The clock will automatically be freed when the device is unbound
from the bus.

struct clk \*devm\_clk\_get\_prepared(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   [`devm_clk_get()`](#c.devm_clk_get "devm_clk_get") + [`clk_prepare()`](#c.clk_prepare "clk_prepare")

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. (IOW, **id** may be identical strings, but
clk\_get may return different clock producers depending on **dev**.)

**Description**

The returned clk (if valid) is prepared. Drivers must however assume
that the clock is not enabled.

The clock will automatically be unprepared and freed when the device
is unbound from the bus.

struct clk \*devm\_clk\_get\_enabled(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   [`devm_clk_get()`](#c.devm_clk_get "devm_clk_get") + `clk_prepare_enable()`

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. (IOW, **id** may be identical strings, but
clk\_get may return different clock producers depending on **dev**.)

**Description**

The returned clk (if valid) is prepared and enabled.

The clock will automatically be disabled, unprepared and freed
when the device is unbound from the bus.

struct clk \*devm\_clk\_get\_optional(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   lookup and obtain a managed reference to an optional clock producer.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. If no such clk is found, it returns NULL
which serves as a dummy clk. That’s the only difference compared
to [`devm_clk_get()`](#c.devm_clk_get "devm_clk_get").

**Description**

Drivers must assume that the clock source is neither prepared nor
enabled.

The clock will automatically be freed when the device is unbound
from the bus.

struct clk \*devm\_clk\_get\_optional\_prepared(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   [`devm_clk_get_optional()`](#c.devm_clk_get_optional "devm_clk_get_optional") + [`clk_prepare()`](#c.clk_prepare "clk_prepare")

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. If no such clk is found, it returns NULL
which serves as a dummy clk. That’s the only difference compared
to [`devm_clk_get_prepared()`](#c.devm_clk_get_prepared "devm_clk_get_prepared").

**Description**

The returned clk (if valid) is prepared. Drivers must however
assume that the clock is not enabled.

The clock will automatically be unprepared and freed when the
device is unbound from the bus.

struct clk \*devm\_clk\_get\_optional\_enabled(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   [`devm_clk_get_optional()`](#c.devm_clk_get_optional "devm_clk_get_optional") + `clk_prepare_enable()`

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. If no such clk is found, it returns NULL
which serves as a dummy clk. That’s the only difference compared
to [`devm_clk_get_enabled()`](#c.devm_clk_get_enabled "devm_clk_get_enabled").

**Description**

The returned clk (if valid) is prepared and enabled.

The clock will automatically be disabled, unprepared and freed
when the device is unbound from the bus.

struct clk \*devm\_clk\_get\_optional\_enabled\_with\_rate(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id, unsigned long rate)
:   [`devm_clk_get_optional()`](#c.devm_clk_get_optional "devm_clk_get_optional") + [`clk_set_rate()`](#c.clk_set_rate "clk_set_rate") + `clk_prepare_enable()`

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

`unsigned long rate`
:   new clock rate

**Context**

May sleep.

**Return**

a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev** and **id** to determine the clock consumer, and thereby
the clock producer. If no such clk is found, it returns NULL
which serves as a dummy clk. That’s the only difference compared
to [`devm_clk_get_enabled()`](#c.devm_clk_get_enabled "devm_clk_get_enabled").

**Description**

The returned clk (if valid) is prepared and enabled and rate was set.

The clock will automatically be disabled, unprepared and freed
when the device is unbound from the bus.

struct clk \*devm\_get\_clk\_from\_child(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct device\_node \*np, const char \*con\_id)
:   lookup and obtain a managed reference to a clock producer from child node.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`struct device_node *np`
:   pointer to clock consumer node

`const char *con_id`
:   clock consumer ID

**Description**

This function parses the clocks, and uses them to look up the
`struct clk` from the registered list of clock providers by using
**np** and **con\_id**

The clock will automatically be freed when the device is unbound
from the bus.

int clk\_enable(struct [clk](#c.clk_enable "clk") \*clk)
:   inform the system when the clock source should be running.

**Parameters**

`struct clk *clk`
:   clock source

**Description**

If the clock can not be enabled/disabled, this should return success.

May be called from atomic contexts.

Returns success (0) or negative errno.

int clk\_bulk\_enable(int num\_clks, const struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   inform the system when the set of clks should be running.

**Parameters**

`int num_clks`
:   the number of clk\_bulk\_data

`const struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Description**

May be called from atomic contexts.

Returns success (0) or negative errno.

void clk\_disable(struct [clk](#c.clk_disable "clk") \*clk)
:   inform the system when the clock source is no longer required.

**Parameters**

`struct clk *clk`
:   clock source

**Description**

Inform the system that a clock source is no longer required by
a driver and may be shut down.

May be called from atomic contexts.

Implementation detail: if the clock source is shared between
multiple drivers, [`clk_enable()`](#c.clk_enable "clk_enable") calls must be balanced by the
same number of [`clk_disable()`](#c.clk_disable "clk_disable") calls for the clock source to be
disabled.

void clk\_bulk\_disable(int num\_clks, const struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   inform the system when the set of clks is no longer required.

**Parameters**

`int num_clks`
:   the number of clk\_bulk\_data

`const struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Description**

Inform the system that a set of clks is no longer required by
a driver and may be shut down.

May be called from atomic contexts.

Implementation detail: if the set of clks is shared between
multiple drivers, [`clk_bulk_enable()`](#c.clk_bulk_enable "clk_bulk_enable") calls must be balanced by the
same number of [`clk_bulk_disable()`](#c.clk_bulk_disable "clk_bulk_disable") calls for the clock source to be
disabled.

unsigned long clk\_get\_rate(struct [clk](#c.clk_get_rate "clk") \*clk)
:   obtain the current clock rate (in Hz) for a clock source. This is only valid once the clock source has been enabled.

**Parameters**

`struct clk *clk`
:   clock source

void clk\_put(struct [clk](#c.clk_put "clk") \*clk)
:   “free” the clock source

**Parameters**

`struct clk *clk`
:   clock source

**Note**

drivers must ensure that all clk\_enable calls made on this
clock source are balanced by clk\_disable calls prior to calling
this function.

clk\_put should not be called from within interrupt context.

void clk\_bulk\_put(int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   “free” the clock source

**Parameters**

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Note**

drivers must ensure that all clk\_bulk\_enable calls made on this
clock source are balanced by clk\_bulk\_disable calls prior to calling
this function.

clk\_bulk\_put should not be called from within interrupt context.

void clk\_bulk\_put\_all(int num\_clks, struct [clk\_bulk\_data](#c.clk_bulk_data "clk_bulk_data") \*clks)
:   “free” all the clock source

**Parameters**

`int num_clks`
:   the number of clk\_bulk\_data

`struct clk_bulk_data *clks`
:   the clk\_bulk\_data table of consumer

**Note**

drivers must ensure that all clk\_bulk\_enable calls made on this
clock source are balanced by clk\_bulk\_disable calls prior to calling
this function.

clk\_bulk\_put\_all should not be called from within interrupt context.

void devm\_clk\_put(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [clk](#c.devm_clk_put "clk") \*clk)
:   “free” a managed clock source

**Parameters**

`struct device *dev`
:   device used to acquire the clock

`struct clk *clk`
:   clock source acquired with [`devm_clk_get()`](#c.devm_clk_get "devm_clk_get")

**Note**

drivers must ensure that all clk\_enable calls made on this
clock source are balanced by clk\_disable calls prior to calling
this function.

clk\_put should not be called from within interrupt context.

long clk\_round\_rate(struct [clk](#c.clk_round_rate "clk") \*clk, unsigned long rate)
:   adjust a rate to the exact rate a clock can provide

**Parameters**

`struct clk *clk`
:   clock source

`unsigned long rate`
:   desired clock rate in Hz

**Description**

This answers the question “if I were to pass **rate** to [`clk_set_rate()`](#c.clk_set_rate "clk_set_rate"),
what clock rate would I end up with?” without changing the hardware
in any way. In other words:

> rate = clk\_round\_rate(clk, r);

and:

> clk\_set\_rate(clk, r);
> rate = clk\_get\_rate(clk);

are equivalent except the former does not modify the clock hardware
in any way.

Returns rounded clock rate in Hz, or negative errno.

int clk\_set\_rate(struct [clk](#c.clk_set_rate "clk") \*clk, unsigned long rate)
:   set the clock rate for a clock source

**Parameters**

`struct clk *clk`
:   clock source

`unsigned long rate`
:   desired clock rate in Hz

**Description**

Updating the rate starts at the top-most affected clock and then
walks the tree down to the bottom-most clock that needs updating.

Returns success (0) or negative errno.

int clk\_set\_rate\_exclusive(struct [clk](#c.clk_set_rate_exclusive "clk") \*clk, unsigned long rate)
:   set the clock rate and claim exclusivity over clock source

**Parameters**

`struct clk *clk`
:   clock source

`unsigned long rate`
:   desired clock rate in Hz

**Description**

This helper function allows drivers to atomically set the rate of a producer
and claim exclusivity over the rate control of the producer.

It is essentially a combination of [`clk_set_rate()`](#c.clk_set_rate "clk_set_rate") and
`clk_rate_exclusite_get()`. Caller must balance this call with a call to
[`clk_rate_exclusive_put()`](#c.clk_rate_exclusive_put "clk_rate_exclusive_put")

Returns success (0) or negative errno.

bool clk\_has\_parent(const struct [clk](#c.clk_has_parent "clk") \*clk, const struct [clk](#c.clk_has_parent "clk") \*parent)
:   check if a clock is a possible parent for another

**Parameters**

`const struct clk *clk`
:   clock source

`const struct clk *parent`
:   parent clock source

**Description**

This function can be used in drivers that need to check that a clock can be
the parent of another without actually changing the parent.

Returns true if **parent** is a possible parent for **clk**, false otherwise.

int clk\_set\_rate\_range(struct [clk](#c.clk_set_rate_range "clk") \*clk, unsigned long min, unsigned long max)
:   set a rate range for a clock source

**Parameters**

`struct clk *clk`
:   clock source

`unsigned long min`
:   desired minimum clock rate in Hz, inclusive

`unsigned long max`
:   desired maximum clock rate in Hz, inclusive

**Description**

Returns success (0) or negative errno.

int clk\_set\_min\_rate(struct [clk](#c.clk_set_min_rate "clk") \*clk, unsigned long rate)
:   set a minimum clock rate for a clock source

**Parameters**

`struct clk *clk`
:   clock source

`unsigned long rate`
:   desired minimum clock rate in Hz, inclusive

**Description**

Returns success (0) or negative errno.

int clk\_set\_max\_rate(struct [clk](#c.clk_set_max_rate "clk") \*clk, unsigned long rate)
:   set a maximum clock rate for a clock source

**Parameters**

`struct clk *clk`
:   clock source

`unsigned long rate`
:   desired maximum clock rate in Hz, inclusive

**Description**

Returns success (0) or negative errno.

int clk\_set\_parent(struct [clk](#c.clk_set_parent "clk") \*clk, struct [clk](#c.clk_set_parent "clk") \*parent)
:   set the parent clock source for this clock

**Parameters**

`struct clk *clk`
:   clock source

`struct clk *parent`
:   parent clock source

**Description**

Returns success (0) or negative errno.

struct [clk](#c.clk_get_parent "clk") \*clk\_get\_parent(struct [clk](#c.clk_get_parent "clk") \*clk)
:   get the parent clock source for this clock

**Parameters**

`struct clk *clk`
:   clock source

**Description**

Returns `struct clk` corresponding to parent clock source, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno.

struct clk \*clk\_get\_sys(const char \*dev\_id, const char \*con\_id)
:   get a clock based upon the device name

**Parameters**

`const char *dev_id`
:   device name

`const char *con_id`
:   connection ID

**Description**

Returns a `struct clk` corresponding to the clock producer, or
valid [`IS_ERR()`](#c.IS_ERR "IS_ERR") condition containing errno. The implementation
uses **dev\_id** and **con\_id** to determine the clock consumer, and
thereby the clock producer. In contrast to [`clk_get()`](#c.clk_get "clk_get") this function
takes the device name instead of the device itself for identification.

Drivers must assume that the clock source is not enabled.

clk\_get\_sys should not be called from within interrupt context.

int clk\_drop\_range(struct [clk](#c.clk_drop_range "clk") \*clk)
:   Reset any range set on that clock

**Parameters**

`struct clk *clk`
:   clock source

**Description**

Returns success (0) or negative errno.

struct clk \*clk\_get\_optional(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*id)
:   lookup and obtain a reference to an optional clock producer.

**Parameters**

`struct device *dev`
:   device for clock “consumer”

`const char *id`
:   clock consumer ID

**Description**

Behaves the same as [`clk_get()`](#c.clk_get "clk_get") except where there is no clock producer. In
this case, instead of returning -ENOENT, the function returns NULL.

## Synchronization Primitives

### Read-Copy Update (RCU)

bool same\_state\_synchronize\_rcu(unsigned long oldstate1, unsigned long oldstate2)
:   Are two old-state values identical?

**Parameters**

`unsigned long oldstate1`
:   First old-state value.

`unsigned long oldstate2`
:   Second old-state value.

**Description**

The two old-state values must have been obtained from either
[`get_state_synchronize_rcu()`](#c.get_state_synchronize_rcu "get_state_synchronize_rcu"), [`start_poll_synchronize_rcu()`](#c.start_poll_synchronize_rcu "start_poll_synchronize_rcu"), or
[`get_completed_synchronize_rcu()`](#c.get_completed_synchronize_rcu "get_completed_synchronize_rcu"). Returns **true** if the two values are
identical and **false** otherwise. This allows structures whose lifetimes
are tracked by old-state values to push these values to a list header,
allowing those structures to be slightly smaller.

cond\_resched\_tasks\_rcu\_qs

`cond_resched_tasks_rcu_qs ()`

> > Report potential quiescent states to RCU
>
> **Description**
>
> This macro resembles `cond_resched()`, except that it is defined to
> report potential quiescent states to RCU-tasks even if the `cond_resched()`
> machinery were to be shut off, as some advocate for PREEMPTION kernels.

rcu\_softirq\_qs\_periodic

`rcu_softirq_qs_periodic (old_ts)`

> Report RCU and RCU-Tasks quiescent states

**Parameters**

`old_ts`
:   jiffies at start of processing.

**Description**

This helper is for long-running softirq handlers, such as NAPI threads in
networking. The caller should initialize the variable passed in as **old\_ts**
at the beginning of the softirq handler. When invoked frequently, this macro
will invoke [`rcu_softirq_qs()`](#c.rcu_softirq_qs "rcu_softirq_qs") every 100 milliseconds thereafter, which will
provide both RCU and RCU-Tasks quiescent states. Note that this macro
modifies its old\_ts argument.

Because regions of code that have disabled softirq act as RCU read-side
critical sections, this macro should be invoked with softirq (and
preemption) enabled.

The macro is not needed when CONFIG\_PREEMPT\_RT is defined. RT kernels would
have more chance to invoke `schedule()` calls and provide necessary quiescent
states. As a contrast, calling `cond_resched()` only won’t achieve the same
effect because `cond_resched()` does not provide RCU-Tasks quiescent states.

RCU\_LOCKDEP\_WARN

`RCU_LOCKDEP_WARN (c, s)`

> emit lockdep splat if specified condition is met

**Parameters**

`c`
:   condition to check

`s`
:   informative message

**Description**

This checks `debug_lockdep_rcu_enabled()` before checking (c) to
prevent early boot splats due to lockdep not yet being initialized,
and rechecks it after checking (c) to prevent false-positive splats
due to races with lockdep being disabled. See [commit 3066820034b5dd](https://git.kernel.org/torvalds/c/3066820034b5dd)
(“rcu: Reject [`RCU_LOCKDEP_WARN()`](#c.RCU_LOCKDEP_WARN "RCU_LOCKDEP_WARN") false positives”) for more detail.

lockdep\_assert\_in\_rcu\_read\_lock

`lockdep_assert_in_rcu_read_lock ()`

> > WARN if not protected by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock")
>
> **Description**
>
> Splats if lockdep is enabled and there is no [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") in effect.

lockdep\_assert\_in\_rcu\_read\_lock\_bh

`lockdep_assert_in_rcu_read_lock_bh ()`

> > WARN if not protected by [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh")
>
> **Description**
>
> Splats if lockdep is enabled and there is no [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") in effect.
> Note that `local_bh_disable()` and friends do not suffice here, instead an
> actual [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") is required.

lockdep\_assert\_in\_rcu\_read\_lock\_sched

`lockdep_assert_in_rcu_read_lock_sched ()`

> > WARN if not protected by [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched")
>
> **Description**
>
> Splats if lockdep is enabled and there is no [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched")
> in effect. Note that `preempt_disable()` and friends do not suffice here,
> instead an actual [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched") is required.

lockdep\_assert\_in\_rcu\_reader

`lockdep_assert_in_rcu_reader ()`

> > WARN if not within some type of RCU reader
>
> **Description**
>
> Splats if lockdep is enabled and there is no RCU reader of any
> type in effect. Note that regions of code protected by things like
> preempt\_disable, `local_bh_disable()`, and `local_irq_disable()` all qualify
> as RCU readers.
>
> Note that this will never trigger in PREEMPT\_NONE or PREEMPT\_VOLUNTARY
> kernels that are not also built with PREEMPT\_COUNT. But if you have
> lockdep enabled, you might as well also enable PREEMPT\_COUNT.

unrcu\_pointer

`unrcu_pointer (p)`

> mark a pointer as not being RCU protected

**Parameters**

`p`
:   pointer needing to lose its \_\_rcu property

**Description**

Converts **p** from an \_\_rcu pointer to a \_\_kernel pointer.
This allows an \_\_rcu pointer to be used with `xchg()` and friends.

RCU\_INITIALIZER

`RCU_INITIALIZER (v)`

> statically initialize an RCU-protected global variable

**Parameters**

`v`
:   The value to statically initialize with.

rcu\_assign\_pointer

`rcu_assign_pointer (p, v)`

> assign to RCU-protected pointer

**Parameters**

`p`
:   pointer to assign to

`v`
:   value to assign (publish)

**Description**

Assigns the specified value to the specified RCU-protected
pointer, ensuring that any concurrent RCU readers will see
any prior initialization.

Inserts memory barriers on architectures that require them
(which is most of them), and also prevents the compiler from
reordering the code that initializes the structure after the pointer
assignment. More importantly, this call documents which pointers
will be dereferenced by RCU read-side code.

In some special cases, you may use [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") instead
of [`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer"). [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") is a bit faster due
to the fact that it does not constrain either the CPU or the compiler.
That said, using [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") when you should have used
[`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer") is a very bad thing that results in
impossible-to-diagnose memory corruption. So please be careful.
See the [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") comment header for details.

Note that [`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer") evaluates each of its arguments only
once, appearances notwithstanding. One of the “extra” evaluations
is in `typeof()` and the other visible only to sparse (\_\_CHECKER\_\_),
neither of which actually execute the argument. As with most cpp
macros, this execute-arguments-only-once property is important, so
please be careful when making changes to [`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer") and the
other macros that it invokes.

rcu\_replace\_pointer

`rcu_replace_pointer (rcu_ptr, ptr, c)`

> replace an RCU pointer, returning its old value

**Parameters**

`rcu_ptr`
:   RCU pointer, whose old value is returned

`ptr`
:   regular pointer

`c`
:   the lockdep conditions under which the dereference will take place

**Description**

Perform a replacement, where **rcu\_ptr** is an RCU-annotated
pointer and **c** is the lockdep argument that is passed to the
[`rcu_dereference_protected()`](#c.rcu_dereference_protected "rcu_dereference_protected") call used to read that pointer. The old
value of **rcu\_ptr** is returned, and **rcu\_ptr** is set to **ptr**.

rcu\_access\_pointer

`rcu_access_pointer (p)`

> fetch RCU pointer with no dereferencing

**Parameters**

`p`
:   The pointer to read

**Description**

Return the value of the specified RCU-protected pointer, but omit the
lockdep checks for being in an RCU read-side critical section. This is
useful when the value of this pointer is accessed, but the pointer is
not dereferenced, for example, when testing an RCU-protected pointer
against NULL. Although [`rcu_access_pointer()`](#c.rcu_access_pointer "rcu_access_pointer") may also be used in cases
where update-side locks prevent the value of the pointer from changing,
you should instead use [`rcu_dereference_protected()`](#c.rcu_dereference_protected "rcu_dereference_protected") for this use case.
Within an RCU read-side critical section, there is little reason to
use [`rcu_access_pointer()`](#c.rcu_access_pointer "rcu_access_pointer").

It is usually best to test the [`rcu_access_pointer()`](#c.rcu_access_pointer "rcu_access_pointer") return value
directly in order to avoid accidental dereferences being introduced
by later inattentive changes. In other words, assigning the
[`rcu_access_pointer()`](#c.rcu_access_pointer "rcu_access_pointer") return value to a local variable results in an
accident waiting to happen.

It is also permissible to use [`rcu_access_pointer()`](#c.rcu_access_pointer "rcu_access_pointer") when read-side
access to the pointer was removed at least one grace period ago, as is
the case in the context of the RCU callback that is freeing up the data,
or after a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") returns. This can be useful when tearing
down multi-linked structures after a grace period has elapsed. However,
[`rcu_dereference_protected()`](#c.rcu_dereference_protected "rcu_dereference_protected") is normally preferred for this use case.

rcu\_dereference\_check

`rcu_dereference_check (p, c)`

> rcu\_dereference with debug checking

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

`c`
:   The conditions under which the dereference will take place

**Description**

Do an [`rcu_dereference()`](#c.rcu_dereference "rcu_dereference"), but check that the conditions under which the
dereference will take place are correct. Typically the conditions
indicate the various locking conditions that should be held at that
point. The check should return true if the conditions are satisfied.
An implicit check for being in an RCU read-side critical section
([`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock")) is included.

For example:

> bar = rcu\_dereference\_check(foo->bar, lockdep\_is\_held(`foo->lock`));

could be used to indicate to lockdep that foo->bar may only be dereferenced
if either [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") is held, or that the lock required to replace
the bar `struct at` foo->bar is held.

Note that the list of conditions may also include indications of when a lock
need not be held, for example during initialisation or destruction of the
target struct:

> bar = rcu\_dereference\_check(foo->bar, lockdep\_is\_held(`foo->lock`) ||
> :   atomic\_read(`foo->usage`) == 0);

Inserts memory barriers on architectures that require them
(currently only the Alpha), prevents the compiler from refetching
(and from merging fetches), and, more importantly, documents exactly
which pointers are protected by RCU and checks that the pointer is
annotated as \_\_rcu.

rcu\_dereference\_bh\_check

`rcu_dereference_bh_check (p, c)`

> rcu\_dereference\_bh with debug checking

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

`c`
:   The conditions under which the dereference will take place

**Description**

This is the RCU-bh counterpart to [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check"). However,
please note that starting in v5.0 kernels, vanilla RCU grace periods
wait for `local_bh_disable()` regions of code in addition to regions of
code demarked by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock"). This means
that [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu"), call\_rcu, and friends all take not only
[`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") but also [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") into account.

rcu\_dereference\_sched\_check

`rcu_dereference_sched_check (p, c)`

> rcu\_dereference\_sched with debug checking

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

`c`
:   The conditions under which the dereference will take place

**Description**

This is the RCU-sched counterpart to [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check").
However, please note that starting in v5.0 kernels, vanilla RCU grace
periods wait for `preempt_disable()` regions of code in addition to
regions of code demarked by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock").
This means that [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu"), call\_rcu, and friends all take not
only [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") but also [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched") into account.

rcu\_dereference\_all\_check

`rcu_dereference_all_check (p, c)`

> rcu\_dereference\_all with debug checking

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

`c`
:   The conditions under which the dereference will take place

**Description**

This is similar to [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check"), but allows protection
by all forms of vanilla RCU readers, including preemption disabled,
bh-disabled, and interrupt-disabled regions of code. Note that “vanilla
RCU” excludes SRCU and the various Tasks RCU flavors. Please note
that this macro should not be backported to any Linux-kernel version
preceding v5.0 due to changes in [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") semantics prior
to that version.

rcu\_dereference\_protected

`rcu_dereference_protected (p, c)`

> fetch RCU pointer when updates prevented

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

`c`
:   The conditions under which the dereference will take place

**Description**

Return the value of the specified RCU-protected pointer, but omit
the `READ_ONCE()`. This is useful in cases where update-side locks
prevent the value of the pointer from changing. Please note that this
primitive does *not* prevent the compiler from repeating this reference
or combining it with other references, so it should not be used without
protection of appropriate locks.

This function is only for update-side use. Using this function
when protected only by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") will result in infrequent
but very ugly failures.

rcu\_dereference

`rcu_dereference (p)`

> fetch RCU-protected pointer for dereferencing

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

**Description**

This is a simple wrapper around [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check").

rcu\_dereference\_bh

`rcu_dereference_bh (p)`

> fetch an RCU-bh-protected pointer for dereferencing

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

**Description**

Makes [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check") do the dirty work.

rcu\_dereference\_sched

`rcu_dereference_sched (p)`

> fetch RCU-sched-protected pointer for dereferencing

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

**Description**

Makes [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check") do the dirty work.

rcu\_dereference\_all

`rcu_dereference_all (p)`

> fetch RCU-all-protected pointer for dereferencing

**Parameters**

`p`
:   The pointer to read, prior to dereferencing

**Description**

Makes [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check") do the dirty work.

rcu\_pointer\_handoff

`rcu_pointer_handoff (p)`

> Hand off a pointer from RCU to other mechanism

**Parameters**

`p`
:   The pointer to hand off

**Description**

This is simply an identity function, but it documents where a pointer
is handed off from RCU to some other synchronization mechanism, for
example, reference counting or locking. In C11, it would map to
`kill_dependency()`. It could be used as follows:

```
rcu_read_lock();
p = rcu_dereference(gp);
long_lived = is_long_lived(p);
if (long_lived) {
        if (!atomic_inc_not_zero(p->refcnt))
                long_lived = false;
        else
                p = rcu_pointer_handoff(p);
}
rcu_read_unlock();
```

void rcu\_read\_lock(void)
:   mark the beginning of an RCU read-side critical section

**Parameters**

`void`
:   no arguments

**Description**

When [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") is invoked on one CPU while other CPUs
are within RCU read-side critical sections, then the
[`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") is guaranteed to block until after all the other
CPUs exit their critical sections. Similarly, if [`call_rcu()`](#c.call_rcu "call_rcu") is invoked
on one CPU while other CPUs are within RCU read-side critical
sections, invocation of the corresponding RCU callback is deferred
until after the all the other CPUs exit their critical sections.

Both [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") and [`call_rcu()`](#c.call_rcu "call_rcu") also wait for regions of code
with preemption disabled, including regions of code with interrupts or
softirqs disabled.

Note, however, that RCU callbacks are permitted to run concurrently
with new RCU read-side critical sections. One way that this can happen
is via the following sequence of events: (1) CPU 0 enters an RCU
read-side critical section, (2) CPU 1 invokes [`call_rcu()`](#c.call_rcu "call_rcu") to register
an RCU callback, (3) CPU 0 exits the RCU read-side critical section,
(4) CPU 2 enters a RCU read-side critical section, (5) the RCU
callback is invoked. This is legal, because the RCU read-side critical
section that was running concurrently with the [`call_rcu()`](#c.call_rcu "call_rcu") (and which
therefore might be referencing something that the corresponding RCU
callback would free up) has completed before the corresponding
RCU callback is invoked.

RCU read-side critical sections may be nested. Any deferred actions
will be deferred until the outermost RCU read-side critical section
completes.

You can avoid reading and understanding the next paragraph by
following this rule: don’t put anything in an [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") RCU
read-side critical section that would block in a !PREEMPTION kernel.
But if you want the full story, read on!

In non-preemptible RCU implementations (pure TREE\_RCU and TINY\_RCU),
it is illegal to block while in an RCU read-side critical section.
In preemptible RCU implementations (PREEMPT\_RCU) in CONFIG\_PREEMPTION
kernel builds, RCU read-side critical sections may be preempted,
but explicit blocking is illegal. Finally, in preemptible RCU
implementations in real-time (with -rt patchset) kernel builds, RCU
read-side critical sections may be preempted and they may also block, but
only when acquiring spinlocks that are subject to priority inheritance.

void rcu\_read\_unlock(void)
:   marks the end of an RCU read-side critical section.

**Parameters**

`void`
:   no arguments

**Description**

In almost all situations, [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock") is immune from deadlock.
This deadlock immunity also extends to the scheduler’s runqueue
and priority-inheritance spinlocks, courtesy of the quiescent-state
deferral that is carried out when [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock") is invoked with
interrupts disabled.

See [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") for more information.

void rcu\_read\_lock\_bh(void)
:   mark the beginning of an RCU-bh critical section

**Parameters**

`void`
:   no arguments

**Description**

This is equivalent to [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock"), but also disables softirqs.
Note that anything else that disables softirqs can also serve as an RCU
read-side critical section. However, please note that this equivalence
applies only to v5.0 and later. Before v5.0, [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and
[`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") were unrelated.

Note that [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") and the matching [`rcu_read_unlock_bh()`](#c.rcu_read_unlock_bh "rcu_read_unlock_bh")
must occur in the same context, for example, it is illegal to invoke
[`rcu_read_unlock_bh()`](#c.rcu_read_unlock_bh "rcu_read_unlock_bh") from one task if the matching [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh")
was invoked from some other task.

void rcu\_read\_unlock\_bh(void)
:   marks the end of a softirq-only RCU critical section

**Parameters**

`void`
:   no arguments

**Description**

See [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") for more information.

void rcu\_read\_lock\_sched(void)
:   mark the beginning of a RCU-sched critical section

**Parameters**

`void`
:   no arguments

**Description**

This is equivalent to [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock"), but also disables preemption.
Read-side critical sections can also be introduced by anything else that
disables preemption, including `local_irq_disable()` and friends. However,
please note that the equivalence to [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") applies only to
v5.0 and later. Before v5.0, [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched")
were unrelated.

Note that [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched") and the matching [`rcu_read_unlock_sched()`](#c.rcu_read_unlock_sched "rcu_read_unlock_sched")
must occur in the same context, for example, it is illegal to invoke
[`rcu_read_unlock_sched()`](#c.rcu_read_unlock_sched "rcu_read_unlock_sched") from process context if the matching
[`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched") was invoked from an NMI handler.

void rcu\_read\_unlock\_sched(void)
:   marks the end of a RCU-classic critical section

**Parameters**

`void`
:   no arguments

**Description**

See [`rcu_read_lock_sched()`](#c.rcu_read_lock_sched "rcu_read_lock_sched") for more information.

RCU\_INIT\_POINTER

`RCU_INIT_POINTER (p, v)`

> initialize an RCU protected pointer

**Parameters**

`p`
:   The pointer to be initialized.

`v`
:   The value to initialized the pointer to.

**Description**

Initialize an RCU-protected pointer in special cases where readers
do not need ordering constraints on the CPU or the compiler. These
special cases are:

1. This use of [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") is NULLing out the pointer *or*
2. The caller has taken whatever steps are required to prevent
   RCU readers from concurrently accessing this pointer *or*
3. The referenced data structure has already been exposed to
   readers either at compile time or via [`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer") *and*

   1. You have not made *any* reader-visible changes to
      this structure since then *or*
   2. It is OK for readers accessing this structure from its
      new location to see the old state of the structure. (For
      example, the changes were to statistical counters or to
      other state where exact synchronization is not required.)

Failure to follow these rules governing use of [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") will
result in impossible-to-diagnose memory corruption. As in the structures
will look OK in crash dumps, but any concurrent RCU readers might
see pre-initialized values of the referenced data structure. So
please be very careful how you use [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER")!!!

If you are creating an RCU-protected linked structure that is accessed
by a single external-to-structure RCU-protected pointer, then you may
use [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") to initialize the internal RCU-protected
pointers, but you must use [`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer") to initialize the
external-to-structure pointer *after* you have completely initialized
the reader-accessible portions of the linked structure.

Note that unlike [`rcu_assign_pointer()`](#c.rcu_assign_pointer "rcu_assign_pointer"), [`RCU_INIT_POINTER()`](#c.RCU_INIT_POINTER "RCU_INIT_POINTER") provides no
ordering guarantees for either the CPU or the compiler.

RCU\_POINTER\_INITIALIZER

`RCU_POINTER_INITIALIZER (p, v)`

> statically initialize an RCU protected pointer

**Parameters**

`p`
:   The pointer to be initialized.

`v`
:   The value to initialized the pointer to.

**Description**

GCC-style initialization for an RCU-protected pointer in a structure field.

kfree\_rcu

`kfree_rcu (ptr, rhf)`

> kfree an object after a grace period.

**Parameters**

`ptr`
:   pointer to kfree for double-argument invocations.

`rhf`
:   the name of the `struct rcu_head` within the type of **ptr**.

**Description**

Many rcu callbacks functions just call [`kfree()`](mm-api.html#c.kfree "kfree") on the base structure.
These functions are trivial, but their size adds up, and furthermore
when they are used in a kernel module, that module must invoke the
high-latency [`rcu_barrier()`](#c.rcu_barrier "rcu_barrier") function at module-unload time.

The [`kfree_rcu()`](#c.kfree_rcu "kfree_rcu") function handles this issue. In order to have a universal
callback function handling different offsets of rcu\_head, the callback needs
to determine the starting address of the freed object, which can be a large
kmalloc or vmalloc allocation. To allow simply aligning the pointer down to
page boundary for those, only offsets up to 4095 bytes can be accommodated.
If the offset is larger than 4095 bytes, a compile-time error will
be generated in `kvfree_rcu_arg_2()`. If this error is triggered, you can
either fall back to use of [`call_rcu()`](#c.call_rcu "call_rcu") or rearrange the structure to
position the rcu\_head structure into the first 4096 bytes.

The object to be freed can be allocated either by [`kmalloc()`](mm-api.html#c.kmalloc "kmalloc"),
[`kmalloc_nolock()`](mm-api.html#c.kmalloc_nolock "kmalloc_nolock"), or [`kmem_cache_alloc()`](mm-api.html#c.kmem_cache_alloc "kmem_cache_alloc").

Note that the allowable offset might decrease in the future.

The BUILD\_BUG\_ON check must not involve any function calls, hence the
checks are done in macros here.

kfree\_rcu\_mightsleep

`kfree_rcu_mightsleep (ptr)`

> kfree an object after a grace period.

**Parameters**

`ptr`
:   pointer to kfree for single-argument invocations.

**Description**

When it comes to head-less variant, only one argument
is passed and that is just a pointer which has to be
freed after a grace period. Therefore the semantic is

> kfree\_rcu\_mightsleep(ptr);

where **ptr** is the pointer to be freed by [`kvfree()`](mm-api.html#c.kvfree "kvfree").

Please note, head-less way of freeing is permitted to
use from a context that has to follow `might_sleep()`
annotation. Otherwise, please switch and embed the
rcu\_head structure within the type of **ptr**.

void rcu\_head\_init(struct rcu\_head \*rhp)
:   Initialize rcu\_head for [`rcu_head_after_call_rcu()`](#c.rcu_head_after_call_rcu "rcu_head_after_call_rcu")

**Parameters**

`struct rcu_head *rhp`
:   The rcu\_head structure to initialize.

**Description**

If you intend to invoke [`rcu_head_after_call_rcu()`](#c.rcu_head_after_call_rcu "rcu_head_after_call_rcu") to test whether a
given rcu\_head structure has already been passed to [`call_rcu()`](#c.call_rcu "call_rcu"), then
you must also invoke this [`rcu_head_init()`](#c.rcu_head_init "rcu_head_init") function on it just after
allocating that structure. Calls to this function must not race with
calls to [`call_rcu()`](#c.call_rcu "call_rcu"), [`rcu_head_after_call_rcu()`](#c.rcu_head_after_call_rcu "rcu_head_after_call_rcu"), or callback invocation.

bool rcu\_head\_after\_call\_rcu(struct rcu\_head \*rhp, rcu\_callback\_t f)
:   Has this rcu\_head been passed to [`call_rcu()`](#c.call_rcu "call_rcu")?

**Parameters**

`struct rcu_head *rhp`
:   The rcu\_head structure to test.

`rcu_callback_t f`
:   The function passed to [`call_rcu()`](#c.call_rcu "call_rcu") along with **rhp**.

**Description**

Returns **true** if the **rhp** has been passed to [`call_rcu()`](#c.call_rcu "call_rcu") with **func**,
and **false** otherwise. Emits a warning in any other case, including
the case where **rhp** has already been invoked after a grace period.
Calls to this function must not race with callback invocation. One way
to avoid such races is to enclose the call to [`rcu_head_after_call_rcu()`](#c.rcu_head_after_call_rcu "rcu_head_after_call_rcu")
in an RCU read-side critical section that includes a read-side fetch
of the pointer to the structure containing **rhp**.

void rcu\_softirq\_qs(void)
:   Provide a set of RCU quiescent states in softirq processing

**Parameters**

`void`
:   no arguments

**Description**

Mark a quiescent state for RCU, Tasks RCU, and Tasks Trace RCU.
This is a special-purpose function to be used in the softirq
infrastructure and perhaps the occasional long-running softirq
handler.

Note that from RCU’s viewpoint, a call to [`rcu_softirq_qs()`](#c.rcu_softirq_qs "rcu_softirq_qs") is
equivalent to momentarily completely enabling preemption. For
example, given this code:

```
local_bh_disable();
do_something();
rcu_softirq_qs();  // A
do_something_else();
local_bh_enable();  // B
```

A call to [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that began concurrently with the
call to `do_something()` would be guaranteed to wait only until
execution reached statement A. Without that [`rcu_softirq_qs()`](#c.rcu_softirq_qs "rcu_softirq_qs"),
that same [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") would instead be guaranteed to wait
until execution reached statement B.

bool rcu\_watching\_snap\_stopped\_since(struct rcu\_data \*rdp, int snap)
:   Has RCU stopped watching a given CPU since the specified **snap**?

**Parameters**

`struct rcu_data *rdp`
:   The rcu\_data corresponding to the CPU for which to check EQS.

`int snap`
:   rcu\_watching snapshot taken when the CPU wasn’t in an EQS.

**Description**

Returns true if the CPU corresponding to **rdp** has spent some time in an
extended quiescent state since **snap**. Note that this doesn’t check if it
/still/ is in an EQS, just that it went through one since **snap**.

This is meant to be used in a loop waiting for a CPU to go through an EQS.

int rcu\_is\_cpu\_rrupt\_from\_idle(void)
:   see if ‘interrupted’ from idle

**Parameters**

`void`
:   no arguments

**Description**

If the current CPU is idle and running at a first-level (not nested)
interrupt, or directly, from idle, return true.

The caller must have at least disabled IRQs.

void rcu\_irq\_exit\_check\_preempt(void)
:   Validate that scheduling is possible

**Parameters**

`void`
:   no arguments

void \_\_rcu\_irq\_enter\_check\_tick(void)
:   Enable scheduler tick on CPU if RCU needs it.

**Parameters**

`void`
:   no arguments

**Description**

The scheduler tick is not normally enabled when CPUs enter the kernel
from nohz\_full userspace execution. After all, nohz\_full userspace
execution is an RCU quiescent state and the time executing in the kernel
is quite short. Except of course when it isn’t. And it is not hard to
cause a large system to spend tens of seconds or even minutes looping
in the kernel, which can cause a number of problems, include RCU CPU
stall warnings.

Therefore, if a nohz\_full CPU fails to report a quiescent state
in a timely manner, the RCU grace-period kthread sets that CPU’s
->rcu\_urgent\_qs flag with the expectation that the next interrupt or
exception will invoke this function, which will turn on the scheduler
tick, which will enable RCU to detect that CPU’s quiescent states,
for example, due to `cond_resched()` calls in CONFIG\_PREEMPT=n kernels.
The tick will be disabled once a quiescent state is reported for
this CPU.

Of course, in carefully tuned systems, there might never be an
interrupt or exception. In that case, the RCU grace-period kthread
will eventually cause one to happen. However, in less carefully
controlled environments, this function allows RCU to get what it
needs without creating otherwise useless interruptions.

notrace bool rcu\_is\_watching(void)
:   RCU read-side critical sections permitted on current CPU?

**Parameters**

`void`
:   no arguments

**Description**

Return **true** if RCU is watching the running CPU and **false** otherwise.
An **true** return means that this CPU can safely enter RCU read-side
critical sections.

Although calls to [`rcu_is_watching()`](#c.rcu_is_watching "rcu_is_watching") from most parts of the kernel
will return **true**, there are important exceptions. For example, if the
current CPU is deep within its idle loop, in kernel entry/exit code,
or offline, [`rcu_is_watching()`](#c.rcu_is_watching "rcu_is_watching") will return **false**.

Make notrace because it can be called by the internal functions of
ftrace, and making this notrace removes unnecessary recursion calls.

void rcu\_set\_gpwrap\_lag(unsigned long lag\_gps)
:   Set RCU GP sequence overflow lag value.

**Parameters**

`unsigned long lag_gps`
:   Set overflow lag to this many grace period worth of counters
    which is used by rcutorture to quickly force a gpwrap situation.
    **lag\_gps** = 0 means we reset it back to the boot-time value.

void call\_rcu\_hurry(struct rcu\_head \*head, rcu\_callback\_t func)
:   Queue RCU callback for invocation after grace period, and flush all lazy callbacks (including the new one) to the main ->cblist while doing so.

**Parameters**

`struct rcu_head *head`
:   structure to be used for queueing the RCU updates.

`rcu_callback_t func`
:   actual callback function to be invoked after the grace period

**Description**

The callback function will be invoked some time after a full grace
period elapses, in other words after all pre-existing RCU read-side
critical sections have completed.

Use this API instead of [`call_rcu()`](#c.call_rcu "call_rcu") if you don’t want the callback to be
delayed for very long periods of time, which can happen on systems without
memory pressure and on systems which are lightly loaded or mostly idle.
This function will cause callbacks to be invoked sooner than later at the
expense of extra power. Other than that, this function is identical to, and
reuses [`call_rcu()`](#c.call_rcu "call_rcu")’s logic. Refer to [`call_rcu()`](#c.call_rcu "call_rcu") for more details about memory
ordering and other functionality.

void call\_rcu(struct rcu\_head \*head, rcu\_callback\_t func)
:   Queue an RCU callback for invocation after a grace period. By default the callbacks are ‘lazy’ and are kept hidden from the main ->cblist to prevent starting of grace periods too soon. If you desire grace periods to start very soon, use [`call_rcu_hurry()`](#c.call_rcu_hurry "call_rcu_hurry").

**Parameters**

`struct rcu_head *head`
:   structure to be used for queueing the RCU updates.

`rcu_callback_t func`
:   actual callback function to be invoked after the grace period

**Description**

The callback function will be invoked some time after a full grace
period elapses, in other words after all pre-existing RCU read-side
critical sections have completed. However, the callback function
might well execute concurrently with RCU read-side critical sections
that started after [`call_rcu()`](#c.call_rcu "call_rcu") was invoked.

It is perfectly legal to repost an RCU callback, potentially with
a different callback function, from within its callback function.
The specified function will be invoked after another full grace period
has elapsed. This use case is similar in form to the common practice
of reposting a timer from within its own handler.

RCU read-side critical sections are delimited by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock")
and [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock"), and may be nested. In addition, but only in
v5.0 and later, regions of code across which interrupts, preemption,
or softirqs have been disabled also serve as RCU read-side critical
sections. This includes hardware interrupt handlers, softirq handlers,
and NMI handlers.

Note that all CPUs must agree that the grace period extended beyond
all pre-existing RCU read-side critical section. On systems with more
than one CPU, this means that when “`func()`” is invoked, each CPU is
guaranteed to have executed a full memory barrier since the end of its
last RCU read-side critical section whose beginning preceded the call
to [`call_rcu()`](#c.call_rcu "call_rcu"). It also means that each CPU executing an RCU read-side
critical section that continues beyond the start of “`func()`” must have
executed a memory barrier after the [`call_rcu()`](#c.call_rcu "call_rcu") but before the beginning
of that RCU read-side critical section. Note that these guarantees
include CPUs that are offline, idle, or executing in user mode, as
well as CPUs that are executing in the kernel.

Furthermore, if CPU A invoked [`call_rcu()`](#c.call_rcu "call_rcu") and CPU B invoked the
resulting RCU callback function “`func()`”, then both CPU A and CPU B are
guaranteed to execute a full memory barrier during the time interval
between the call to [`call_rcu()`](#c.call_rcu "call_rcu") and the invocation of “`func()`” -- even
if CPU A and CPU B are the same CPU (but again only if the system has
more than one CPU).

Implementation of these memory-ordering guarantees is described here:
[A Tour Through TREE\_RCU’s Grace-Period Memory Ordering](../RCU/Design/Memory-Ordering/Tree-RCU-Memory-Ordering.html).

Specific to [`call_rcu()`](#c.call_rcu "call_rcu") (as opposed to the other call\_rcu\*() functions),
in kernels built with CONFIG\_RCU\_LAZY=y, [`call_rcu()`](#c.call_rcu "call_rcu") might delay for many
seconds before starting the grace period needed by the corresponding
callback. This delay can significantly improve energy-efficiency
on low-utilization battery-powered devices. To avoid this delay,
in latency-sensitive kernel code, use [`call_rcu_hurry()`](#c.call_rcu_hurry "call_rcu_hurry").

void synchronize\_rcu(void)
:   wait until a grace period has elapsed.

**Parameters**

`void`
:   no arguments

**Description**

Control will return to the caller some time after a full grace
period has elapsed, in other words after all currently executing RCU
read-side critical sections have completed. Note, however, that
upon return from [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu"), the caller might well be executing
concurrently with new RCU read-side critical sections that began while
[`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") was waiting.

RCU read-side critical sections are delimited by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock")
and [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock"), and may be nested. In addition, but only in
v5.0 and later, regions of code across which interrupts, preemption,
or softirqs have been disabled also serve as RCU read-side critical
sections. This includes hardware interrupt handlers, softirq handlers,
and NMI handlers.

Note that this guarantee implies further memory-ordering guarantees.
On systems with more than one CPU, when [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") returns,
each CPU is guaranteed to have executed a full memory barrier since
the end of its last RCU read-side critical section whose beginning
preceded the call to [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu"). In addition, each CPU having
an RCU read-side critical section that extends beyond the return from
[`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") is guaranteed to have executed a full memory barrier
after the beginning of [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") and before the beginning of
that RCU read-side critical section. Note that these guarantees include
CPUs that are offline, idle, or executing in user mode, as well as CPUs
that are executing in the kernel.

Furthermore, if CPU A invoked [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu"), which returned
to its caller on CPU B, then both CPU A and CPU B are guaranteed
to have executed a full memory barrier during the execution of
[`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") -- even if CPU A and CPU B are the same CPU (but
again only if the system has more than one CPU).

Implementation of these memory-ordering guarantees is described here:
[A Tour Through TREE\_RCU’s Grace-Period Memory Ordering](../RCU/Design/Memory-Ordering/Tree-RCU-Memory-Ordering.html).

void get\_completed\_synchronize\_rcu\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Return a full pre-completed polled state cookie

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   Place to put state cookie

**Description**

Stores into **rgosp** a value that will always be treated by functions
like [`poll_state_synchronize_rcu_full()`](#c.poll_state_synchronize_rcu_full "poll_state_synchronize_rcu_full") as a cookie whose grace period
has already completed.

unsigned long get\_state\_synchronize\_rcu(void)
:   Snapshot current RCU state

**Parameters**

`void`
:   no arguments

**Description**

Returns a cookie that is used by a later call to [`cond_synchronize_rcu()`](#c.cond_synchronize_rcu "cond_synchronize_rcu")
or [`poll_state_synchronize_rcu()`](#c.poll_state_synchronize_rcu "poll_state_synchronize_rcu") to determine whether or not a full
grace period has elapsed in the meantime.

void get\_state\_synchronize\_rcu\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Snapshot RCU state, both normal and expedited

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   location to place combined normal/expedited grace-period state

**Description**

Places the normal and expedited grace-period states in **rgosp**. This
state value can be passed to a later call to [`cond_synchronize_rcu_full()`](#c.cond_synchronize_rcu_full "cond_synchronize_rcu_full")
or [`poll_state_synchronize_rcu_full()`](#c.poll_state_synchronize_rcu_full "poll_state_synchronize_rcu_full") to determine whether or not a
grace period (whether normal or expedited) has elapsed in the meantime.
The rcu\_gp\_oldstate structure takes up twice the memory of an unsigned
long, but is guaranteed to see all grace periods. In contrast, the
combined state occupies less memory, but can sometimes fail to take
grace periods into account.

This does not guarantee that the needed grace period will actually
start.

unsigned long start\_poll\_synchronize\_rcu(void)
:   Snapshot and start RCU grace period

**Parameters**

`void`
:   no arguments

**Description**

Returns a cookie that is used by a later call to [`cond_synchronize_rcu()`](#c.cond_synchronize_rcu "cond_synchronize_rcu")
or [`poll_state_synchronize_rcu()`](#c.poll_state_synchronize_rcu "poll_state_synchronize_rcu") to determine whether or not a full
grace period has elapsed in the meantime. If the needed grace period
is not already slated to start, notifies RCU core of the need for that
grace period.

void start\_poll\_synchronize\_rcu\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Take a full snapshot and start RCU grace period

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   value from [`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full") or [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full")

**Description**

Places the normal and expedited grace-period states in **\*rgos**. This
state value can be passed to a later call to [`cond_synchronize_rcu_full()`](#c.cond_synchronize_rcu_full "cond_synchronize_rcu_full")
or [`poll_state_synchronize_rcu_full()`](#c.poll_state_synchronize_rcu_full "poll_state_synchronize_rcu_full") to determine whether or not a
grace period (whether normal or expedited) has elapsed in the meantime.
If the needed grace period is not already slated to start, notifies
RCU core of the need for that grace period.

bool poll\_state\_synchronize\_rcu(unsigned long oldstate)
:   Has the specified RCU grace period completed?

**Parameters**

`unsigned long oldstate`
:   value from [`get_state_synchronize_rcu()`](#c.get_state_synchronize_rcu "get_state_synchronize_rcu") or [`start_poll_synchronize_rcu()`](#c.start_poll_synchronize_rcu "start_poll_synchronize_rcu")

**Description**

If a full RCU grace period has elapsed since the earlier call from
which **oldstate** was obtained, return **true**, otherwise return **false**.
If **false** is returned, it is the caller’s responsibility to invoke this
function later on until it does return **true**. Alternatively, the caller
can explicitly wait for a grace period, for example, by passing **oldstate**
to either [`cond_synchronize_rcu()`](#c.cond_synchronize_rcu "cond_synchronize_rcu") or [`cond_synchronize_rcu_expedited()`](#c.cond_synchronize_rcu_expedited "cond_synchronize_rcu_expedited")
on the one hand or by directly invoking either [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") or
[`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited") on the other.

Yes, this function does not take counter wrap into account.
But counter wrap is harmless. If the counter wraps, we have waited for
more than a billion grace periods (and way more on a 64-bit system!).
Those needing to keep old state values for very long time periods
(many hours even on 32-bit systems) should check them occasionally and
either refresh them or set a flag indicating that the grace period has
completed. Alternatively, they can use [`get_completed_synchronize_rcu()`](#c.get_completed_synchronize_rcu "get_completed_synchronize_rcu")
to get a guaranteed-completed grace-period state.

In addition, because oldstate compresses the grace-period state for
both normal and expedited grace periods into a single unsigned long,
it can miss a grace period when [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") runs concurrently
with [`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited"). If this is unacceptable, please
instead use the `_full()` variant of these polling APIs.

This function provides the same memory-ordering guarantees that
would be provided by a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that was invoked at the call
to the function that provided **oldstate**, and that returned at the end
of this function.

bool poll\_state\_synchronize\_rcu\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Has the specified RCU grace period completed?

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   value from [`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full") or [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full")

**Description**

If a full RCU grace period has elapsed since the earlier call from
which *rgosp was obtained, return \*\*true\**, otherwise return **false**.
If **false** is returned, it is the caller’s responsibility to invoke this
function later on until it does return **true**. Alternatively, the caller
can explicitly wait for a grace period, for example, by passing **rgosp**
to [`cond_synchronize_rcu()`](#c.cond_synchronize_rcu "cond_synchronize_rcu") or by directly invoking [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu").

Yes, this function does not take counter wrap into account.
But counter wrap is harmless. If the counter wraps, we have waited
for more than a billion grace periods (and way more on a 64-bit
system!). Those needing to keep rcu\_gp\_oldstate values for very
long time periods (many hours even on 32-bit systems) should check
them occasionally and either refresh them or set a flag indicating
that the grace period has completed. Alternatively, they can use
[`get_completed_synchronize_rcu_full()`](#c.get_completed_synchronize_rcu_full "get_completed_synchronize_rcu_full") to get a guaranteed-completed
grace-period state.

This function provides the same memory-ordering guarantees that would
be provided by a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that was invoked at the call to
the function that provided **rgosp**, and that returned at the end of this
function. And this guarantee requires that the root rcu\_node structure’s
->gp\_seq field be checked instead of that of the rcu\_state structure.
The problem is that the just-ending grace-period’s callbacks can be
invoked between the time that the root rcu\_node structure’s ->gp\_seq
field is updated and the time that the rcu\_state structure’s ->gp\_seq
field is updated. Therefore, if a single [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") is to
cause a subsequent [`poll_state_synchronize_rcu_full()`](#c.poll_state_synchronize_rcu_full "poll_state_synchronize_rcu_full") to return **true**,
then the root rcu\_node structure is the one that needs to be polled.

void cond\_synchronize\_rcu(unsigned long oldstate)
:   Conditionally wait for an RCU grace period

**Parameters**

`unsigned long oldstate`
:   value from [`get_state_synchronize_rcu()`](#c.get_state_synchronize_rcu "get_state_synchronize_rcu"), [`start_poll_synchronize_rcu()`](#c.start_poll_synchronize_rcu "start_poll_synchronize_rcu"), or [`start_poll_synchronize_rcu_expedited()`](#c.start_poll_synchronize_rcu_expedited "start_poll_synchronize_rcu_expedited")

**Description**

If a full RCU grace period has elapsed since the earlier call to
[`get_state_synchronize_rcu()`](#c.get_state_synchronize_rcu "get_state_synchronize_rcu") or [`start_poll_synchronize_rcu()`](#c.start_poll_synchronize_rcu "start_poll_synchronize_rcu"), just return.
Otherwise, invoke [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") to wait for a full grace period.

Yes, this function does not take counter wrap into account.
But counter wrap is harmless. If the counter wraps, we have waited for
more than 2 billion grace periods (and way more on a 64-bit system!),
so waiting for a couple of additional grace periods should be just fine.

This function provides the same memory-ordering guarantees that
would be provided by a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that was invoked at the call
to the function that provided **oldstate** and that returned at the end
of this function.

void cond\_synchronize\_rcu\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Conditionally wait for an RCU grace period

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   value from [`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full"), [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full"), or [`start_poll_synchronize_rcu_expedited_full()`](#c.start_poll_synchronize_rcu_expedited_full "start_poll_synchronize_rcu_expedited_full")

**Description**

If a full RCU grace period has elapsed since the call to
[`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full"), [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full"),
or [`start_poll_synchronize_rcu_expedited_full()`](#c.start_poll_synchronize_rcu_expedited_full "start_poll_synchronize_rcu_expedited_full") from which **rgosp** was
obtained, just return. Otherwise, invoke [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") to wait
for a full grace period.

Yes, this function does not take counter wrap into account.
But counter wrap is harmless. If the counter wraps, we have waited for
more than 2 billion grace periods (and way more on a 64-bit system!),
so waiting for a couple of additional grace periods should be just fine.

This function provides the same memory-ordering guarantees that
would be provided by a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that was invoked at the call
to the function that provided **rgosp** and that returned at the end of
this function.

void rcu\_barrier(void)
:   Wait until all in-flight [`call_rcu()`](#c.call_rcu "call_rcu") callbacks complete.

**Parameters**

`void`
:   no arguments

**Description**

Note that this primitive does not necessarily wait for an RCU grace period
to complete. For example, if there are no RCU callbacks queued anywhere
in the system, then [`rcu_barrier()`](#c.rcu_barrier "rcu_barrier") is within its rights to return
immediately, without waiting for anything, much less an RCU grace period.
In fact, [`rcu_barrier()`](#c.rcu_barrier "rcu_barrier") will normally not result in any RCU grace periods
beyond those that were already destined to be executed.

In kernels built with CONFIG\_RCU\_LAZY=y, this function also hurries all
pending lazy RCU callbacks.

void rcu\_barrier\_throttled(void)
:   Do [`rcu_barrier()`](#c.rcu_barrier "rcu_barrier"), but limit to one per second

**Parameters**

`void`
:   no arguments

**Description**

This can be thought of as guard rails around [`rcu_barrier()`](#c.rcu_barrier "rcu_barrier") that
permits unrestricted userspace use, at least assuming the hardware’s
`try_cmpxchg()` is robust. There will be at most one call per second to
[`rcu_barrier()`](#c.rcu_barrier "rcu_barrier") system-wide from use of this function, which means that
callers might needlessly wait a second or three.

This is intended for use by test suites to avoid OOM by flushing RCU
callbacks from the previous test before starting the next. See the
rcutree.do\_rcu\_barrier module parameter for more information.

Why not simply make [`rcu_barrier()`](#c.rcu_barrier "rcu_barrier") more scalable? That might be
the eventual endpoint, but let’s keep it simple for the time being.
Note that the module parameter infrastructure serializes calls to a
given .`set()` function, but should concurrent .`set()` invocation ever be
possible, we are ready!

void synchronize\_rcu\_expedited(void)
:   Brute-force RCU grace period

**Parameters**

`void`
:   no arguments

**Description**

Wait for an RCU grace period, but expedite it. The basic idea is to
IPI all non-idle non-nohz online CPUs. The IPI handler checks whether
the CPU is in an RCU critical section, and if so, it sets a flag that
causes the outermost [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock") to report the quiescent state
for RCU-preempt or asks the scheduler for help for RCU-sched. On the
other hand, if the CPU is not in an RCU read-side critical section,
the IPI handler reports the quiescent state immediately.

Although this is a great improvement over previous expedited
implementations, it is still unfriendly to real-time workloads, so is
thus not recommended for any sort of common-case code. In fact, if
you are using [`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited") in a loop, please restructure
your code to batch your updates, and then use a single [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu")
instead.

This has the same semantics as (but is more brutal than) [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu").

unsigned long start\_poll\_synchronize\_rcu\_expedited(void)
:   Snapshot current RCU state and start expedited grace period

**Parameters**

`void`
:   no arguments

**Description**

Returns a cookie to pass to a call to [`cond_synchronize_rcu()`](#c.cond_synchronize_rcu "cond_synchronize_rcu"),
[`cond_synchronize_rcu_expedited()`](#c.cond_synchronize_rcu_expedited "cond_synchronize_rcu_expedited"), or [`poll_state_synchronize_rcu()`](#c.poll_state_synchronize_rcu "poll_state_synchronize_rcu"),
allowing them to determine whether or not any sort of grace period has
elapsed in the meantime. If the needed expedited grace period is not
already slated to start, initiates that grace period.

void start\_poll\_synchronize\_rcu\_expedited\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Take a full snapshot and start expedited grace period

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   Place to put snapshot of grace-period state

**Description**

Places the normal and expedited grace-period states in rgosp. This
state value can be passed to a later call to [`cond_synchronize_rcu_full()`](#c.cond_synchronize_rcu_full "cond_synchronize_rcu_full")
or [`poll_state_synchronize_rcu_full()`](#c.poll_state_synchronize_rcu_full "poll_state_synchronize_rcu_full") to determine whether or not a
grace period (whether normal or expedited) has elapsed in the meantime.
If the needed expedited grace period is not already slated to start,
initiates that grace period.

void cond\_synchronize\_rcu\_expedited(unsigned long oldstate)
:   Conditionally wait for an expedited RCU grace period

**Parameters**

`unsigned long oldstate`
:   value from [`get_state_synchronize_rcu()`](#c.get_state_synchronize_rcu "get_state_synchronize_rcu"), [`start_poll_synchronize_rcu()`](#c.start_poll_synchronize_rcu "start_poll_synchronize_rcu"), or [`start_poll_synchronize_rcu_expedited()`](#c.start_poll_synchronize_rcu_expedited "start_poll_synchronize_rcu_expedited")

**Description**

If any type of full RCU grace period has elapsed since the earlier
call to [`get_state_synchronize_rcu()`](#c.get_state_synchronize_rcu "get_state_synchronize_rcu"), [`start_poll_synchronize_rcu()`](#c.start_poll_synchronize_rcu "start_poll_synchronize_rcu"),
or [`start_poll_synchronize_rcu_expedited()`](#c.start_poll_synchronize_rcu_expedited "start_poll_synchronize_rcu_expedited"), just return. Otherwise,
invoke [`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited") to wait for a full grace period.

Yes, this function does not take counter wrap into account.
But counter wrap is harmless. If the counter wraps, we have waited for
more than 2 billion grace periods (and way more on a 64-bit system!),
so waiting for a couple of additional grace periods should be just fine.

This function provides the same memory-ordering guarantees that
would be provided by a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that was invoked at the call
to the function that provided **oldstate** and that returned at the end
of this function.

void cond\_synchronize\_rcu\_expedited\_full(struct rcu\_gp\_oldstate \*rgosp)
:   Conditionally wait for an expedited RCU grace period

**Parameters**

`struct rcu_gp_oldstate *rgosp`
:   value from [`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full"), [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full"), or [`start_poll_synchronize_rcu_expedited_full()`](#c.start_poll_synchronize_rcu_expedited_full "start_poll_synchronize_rcu_expedited_full")

**Description**

If a full RCU grace period has elapsed since the call to
[`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full"), [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full"),
or [`start_poll_synchronize_rcu_expedited_full()`](#c.start_poll_synchronize_rcu_expedited_full "start_poll_synchronize_rcu_expedited_full") from which **rgosp** was
obtained, just return. Otherwise, invoke [`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited")
to wait for a full grace period.

Yes, this function does not take counter wrap into account.
But counter wrap is harmless. If the counter wraps, we have waited for
more than 2 billion grace periods (and way more on a 64-bit system!),
so waiting for a couple of additional grace periods should be just fine.

This function provides the same memory-ordering guarantees that
would be provided by a [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") that was invoked at the call
to the function that provided **rgosp** and that returned at the end of
this function.

bool rcu\_read\_lock\_held\_common(bool \*ret)
:   might we be in RCU-sched read-side critical section?

**Parameters**

`bool *ret`
:   Best guess answer if lockdep cannot be relied on

**Description**

Returns true if lockdep must be ignored, in which case `*ret` contains
the best guess described below. Otherwise returns false, in which
case `*ret` tells the caller nothing and the caller should instead
consult lockdep.

If CONFIG\_DEBUG\_LOCK\_ALLOC is selected, set `*ret` to nonzero iff in an
RCU-sched read-side critical section. In absence of
CONFIG\_DEBUG\_LOCK\_ALLOC, this assumes we are in an RCU-sched read-side
critical section unless it can prove otherwise. Note that disabling
of preemption (including disabling irqs) counts as an RCU-sched
read-side critical section. This is useful for debug checks in functions
that required that they be called within an RCU-sched read-side
critical section.

Check `debug_lockdep_rcu_enabled()` to prevent false positives during boot
and while lockdep is disabled.

Note that if the CPU is in the idle loop from an RCU point of view (ie:
that we are in the section between `ct_idle_enter()` and `ct_idle_exit()`)
then [`rcu_read_lock_held()`](#c.rcu_read_lock_held "rcu_read_lock_held") sets `*ret` to false even if the CPU did an
[`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock"). The reason for this is that RCU ignores CPUs that are
in such a section, considering these as in extended quiescent state,
so such a CPU is effectively never in an RCU read-side critical section
regardless of what RCU primitives it invokes. This state of affairs is
required --- we need to keep an RCU-free window in idle where the CPU may
possibly enter into low power mode. This way we can notice an extended
quiescent state to other CPUs that started a grace period. Otherwise
we would delay any grace period as long as we run in the idle task.

Similarly, we avoid claiming an RCU read lock held if the current
CPU is offline.

void rcu\_async\_hurry(void)
:   Make future async RCU callbacks not lazy.

**Parameters**

`void`
:   no arguments

**Description**

After a call to this function, future calls to [`call_rcu()`](#c.call_rcu "call_rcu")
will be processed in a timely fashion.

void rcu\_async\_relax(void)
:   Make future async RCU callbacks lazy.

**Parameters**

`void`
:   no arguments

**Description**

After a call to this function, future calls to [`call_rcu()`](#c.call_rcu "call_rcu")
will be processed in a lazy fashion.

void rcu\_expedite\_gp(void)
:   Expedite future RCU grace periods

**Parameters**

`void`
:   no arguments

**Description**

After a call to this function, future calls to [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") and
friends act as the corresponding [`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited") function
had instead been called.

void rcu\_unexpedite\_gp(void)
:   Cancel prior [`rcu_expedite_gp()`](#c.rcu_expedite_gp "rcu_expedite_gp") invocation

**Parameters**

`void`
:   no arguments

**Description**

Undo a prior call to [`rcu_expedite_gp()`](#c.rcu_expedite_gp "rcu_expedite_gp"). If all prior calls to
[`rcu_expedite_gp()`](#c.rcu_expedite_gp "rcu_expedite_gp") are undone by a subsequent call to [`rcu_unexpedite_gp()`](#c.rcu_unexpedite_gp "rcu_unexpedite_gp"),
and if the rcu\_expedited sysfs/boot parameter is not set, then all
subsequent calls to [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") and friends will return to
their normal non-expedited behavior.

int notrace rcu\_read\_lock\_held(void)
:   might we be in RCU read-side critical section?

**Parameters**

`void`
:   no arguments

**Description**

If CONFIG\_DEBUG\_LOCK\_ALLOC is selected, returns nonzero iff in an RCU
read-side critical section. In absence of CONFIG\_DEBUG\_LOCK\_ALLOC,
this assumes we are in an RCU read-side critical section unless it can
prove otherwise. This is useful for debug checks in functions that
require that they be called within an RCU read-side critical section.

Checks `debug_lockdep_rcu_enabled()` to prevent false positives during boot
and while lockdep is disabled.

Note that [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and the matching [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock") must
occur in the same context, for example, it is illegal to invoke
[`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock") in process context if the matching [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock")
was invoked from within an irq handler.

Note that [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") is disallowed if the CPU is either idle or
offline from an RCU perspective, so check for those as well.

int notrace rcu\_read\_lock\_bh\_held(void)
:   might we be in RCU-bh read-side critical section?

**Parameters**

`void`
:   no arguments

**Description**

Check for bottom half being disabled, which covers both the
CONFIG\_PROVE\_RCU and not cases. Note that if someone uses
[`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh"), but then later enables BH, lockdep (if enabled)
will show the situation. This is useful for debug checks in functions
that require that they be called within an RCU read-side critical
section.

Check `debug_lockdep_rcu_enabled()` to prevent false positives during boot.

Note that [`rcu_read_lock_bh()`](#c.rcu_read_lock_bh "rcu_read_lock_bh") is disallowed if the CPU is either idle or
offline from an RCU perspective, so check for those as well.

void wakeme\_after\_rcu(struct rcu\_head \*head)
:   Callback function to awaken a task after grace period

**Parameters**

`struct rcu_head *head`
:   Pointer to rcu\_head member within rcu\_synchronize structure

**Description**

Awaken the corresponding task now that a grace period has elapsed.

void init\_rcu\_head\_on\_stack(struct rcu\_head \*head)
:   initialize on-stack rcu\_head for debugobjects

**Parameters**

`struct rcu_head *head`
:   pointer to rcu\_head structure to be initialized

**Description**

This function informs debugobjects of a new rcu\_head structure that
has been allocated as an auto variable on the stack. This function
is not required for rcu\_head structures that are statically defined or
that are dynamically allocated on the heap. This function has no
effect for !CONFIG\_DEBUG\_OBJECTS\_RCU\_HEAD kernel builds.

void destroy\_rcu\_head\_on\_stack(struct rcu\_head \*head)
:   destroy on-stack rcu\_head for debugobjects

**Parameters**

`struct rcu_head *head`
:   pointer to rcu\_head structure to be initialized

**Description**

This function informs debugobjects that an on-stack rcu\_head structure
is about to go out of scope. As with [`init_rcu_head_on_stack()`](#c.init_rcu_head_on_stack "init_rcu_head_on_stack"), this
function is not required for rcu\_head structures that are statically
defined or that are dynamically allocated on the heap. Also as with
[`init_rcu_head_on_stack()`](#c.init_rcu_head_on_stack "init_rcu_head_on_stack"), this function has no effect for
!CONFIG\_DEBUG\_OBJECTS\_RCU\_HEAD kernel builds.

unsigned long get\_completed\_synchronize\_rcu(void)
:   Return a pre-completed polled state cookie

**Parameters**

`void`
:   no arguments

**Description**

Returns a value that will always be treated by functions like
[`poll_state_synchronize_rcu()`](#c.poll_state_synchronize_rcu "poll_state_synchronize_rcu") as a cookie whose grace period has already
completed.

unsigned long get\_completed\_synchronize\_srcu(void)
:   Return a pre-completed polled state cookie

**Parameters**

`void`
:   no arguments

**Description**

Returns a value that [`poll_state_synchronize_srcu()`](#c.poll_state_synchronize_srcu "poll_state_synchronize_srcu") will always treat
as a cookie whose grace period has already completed.

bool same\_state\_synchronize\_srcu(unsigned long oldstate1, unsigned long oldstate2)
:   Are two old-state values identical?

**Parameters**

`unsigned long oldstate1`
:   First old-state value.

`unsigned long oldstate2`
:   Second old-state value.

**Description**

The two old-state values must have been obtained from either
[`get_state_synchronize_srcu()`](#c.get_state_synchronize_srcu "get_state_synchronize_srcu"), [`start_poll_synchronize_srcu()`](#c.start_poll_synchronize_srcu "start_poll_synchronize_srcu"), or
[`get_completed_synchronize_srcu()`](#c.get_completed_synchronize_srcu "get_completed_synchronize_srcu"). Returns **true** if the two values are
identical and **false** otherwise. This allows structures whose lifetimes
are tracked by old-state values to push these values to a list header,
allowing those structures to be slightly smaller.

int srcu\_read\_lock\_held(const struct srcu\_struct \*ssp)
:   might we be in SRCU read-side critical section?

**Parameters**

`const struct srcu_struct *ssp`
:   The srcu\_struct structure to check

**Description**

If CONFIG\_DEBUG\_LOCK\_ALLOC is selected, returns nonzero iff in an SRCU
read-side critical section. In absence of CONFIG\_DEBUG\_LOCK\_ALLOC,
this assumes we are in an SRCU read-side critical section unless it can
prove otherwise.

Checks `debug_lockdep_rcu_enabled()` to prevent false positives during boot
and while lockdep is disabled.

Note that SRCU is based on its own statemachine and it doesn’t
relies on normal RCU, it can be called from the CPU which
is in the idle loop from an RCU point of view or offline.

srcu\_dereference\_check

`srcu_dereference_check (p, ssp, c)`

> fetch SRCU-protected pointer for later dereferencing

**Parameters**

`p`
:   the pointer to fetch and protect for later dereferencing

`ssp`
:   pointer to the srcu\_struct, which is used to check that we
    really are in an SRCU read-side critical section.

`c`
:   condition to check for update-side use

**Description**

If PROVE\_RCU is enabled, invoking this outside of an RCU read-side
critical section will result in an RCU-lockdep splat, unless **c** evaluates
to 1. The **c** argument will normally be a logical expression containing
`lockdep_is_held()` calls.

srcu\_dereference

`srcu_dereference (p, ssp)`

> fetch SRCU-protected pointer for later dereferencing

**Parameters**

`p`
:   the pointer to fetch and protect for later dereferencing

`ssp`
:   pointer to the srcu\_struct, which is used to check that we
    really are in an SRCU read-side critical section.

**Description**

Makes [`rcu_dereference_check()`](#c.rcu_dereference_check "rcu_dereference_check") do the dirty work. If PROVE\_RCU
is enabled, invoking this outside of an RCU read-side critical
section will result in an RCU-lockdep splat.

srcu\_dereference\_notrace

`srcu_dereference_notrace (p, ssp)`

> no tracing and no lockdep calls from here

**Parameters**

`p`
:   the pointer to fetch and protect for later dereferencing

`ssp`
:   pointer to the srcu\_struct, which is used to check that we
    really are in an SRCU read-side critical section.

int srcu\_read\_lock(struct srcu\_struct \*ssp)
:   register a new reader for an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to register the new reader.

**Description**

Enter an SRCU read-side critical section. Note that SRCU read-side
critical sections may be nested. However, it is illegal to
call anything that waits on an SRCU grace period for the same
srcu\_struct, whether directly or indirectly. Please note that
one way to indirectly wait on an SRCU grace period is to acquire
a mutex that is held elsewhere while calling [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") or
[`synchronize_srcu_expedited()`](#c.synchronize_srcu_expedited "synchronize_srcu_expedited").

The return value from [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") is guaranteed to be
non-negative. This value must be passed unaltered to the matching
[`srcu_read_unlock()`](#c.srcu_read_unlock "srcu_read_unlock"). Note that [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") and the matching
[`srcu_read_unlock()`](#c.srcu_read_unlock "srcu_read_unlock") must occur in the same context, for example, it is
illegal to invoke [`srcu_read_unlock()`](#c.srcu_read_unlock "srcu_read_unlock") in an irq handler if the matching
[`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") was invoked in process context. Or, for that matter to
invoke [`srcu_read_unlock()`](#c.srcu_read_unlock "srcu_read_unlock") from one task and the matching [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock")
from another.

struct srcu\_ctr \_\_percpu \*srcu\_read\_lock\_fast(struct srcu\_struct \*ssp)
:   register a new reader for an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to register the new reader.

**Description**

Enter an SRCU read-side critical section, but for a light-weight
`smp_mb()`-free reader. See [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") for more information. This
function is NMI-safe, in a manner similar to [`srcu_read_lock_nmisafe()`](#c.srcu_read_lock_nmisafe "srcu_read_lock_nmisafe").

For [`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast") to be used on an srcu\_struct structure,
that structure must have been defined using either `DEFINE_SRCU_FAST()`
or `DEFINE_STATIC_SRCU_FAST()` on the one hand or initialized with
[`init_srcu_struct_fast()`](#c.init_srcu_struct_fast "init_srcu_struct_fast") on the other. Such an srcu\_struct structure
cannot be passed to any non-fast variant of srcu\_read\_{,un}`lock()` or
srcu\_{down,up}`_read()`. In kernels built with CONFIG\_PROVE\_RCU=y,
`__srcu_check_read_flavor()` will complain bitterly if you ignore this
restriction.

Grace-period auto-expediting is disabled for SRCU-fast srcu\_struct
structures because SRCU-fast expedited grace periods invoke
[`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited"), IPIs and all. If you need expedited
SRCU-fast grace periods, use [`synchronize_srcu_expedited()`](#c.synchronize_srcu_expedited "synchronize_srcu_expedited").

The [`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast") function can be invoked only from those
contexts where RCU is watching, that is, from contexts where it would
be legal to invoke [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock"). Otherwise, lockdep will complain.

struct srcu\_ctr \_\_percpu \*srcu\_read\_lock\_fast\_updown(struct srcu\_struct \*ssp)
:   register a new reader for an SRCU-fast-updown structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to register the new reader.

**Description**

Enter an SRCU read-side critical section, but for a light-weight
`smp_mb()`-free reader. See [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") for more information.
This function is compatible with [`srcu_down_read_fast()`](#c.srcu_down_read_fast "srcu_down_read_fast"), but is not
NMI-safe.

For [`srcu_read_lock_fast_updown()`](#c.srcu_read_lock_fast_updown "srcu_read_lock_fast_updown") to be used on an srcu\_struct
structure, that structure must have been defined using either
`DEFINE_SRCU_FAST_UPDOWN()` or `DEFINE_STATIC_SRCU_FAST_UPDOWN()` on the one
hand or initialized with [`init_srcu_struct_fast_updown()`](#c.init_srcu_struct_fast_updown "init_srcu_struct_fast_updown") on the other.
Such an srcu\_struct structure cannot be passed to any non-fast-updown
variant of srcu\_read\_{,un}`lock()` or srcu\_{down,up}`_read()`. In kernels
built with CONFIG\_PROVE\_RCU=y, `__srcu_check_read_flavor()` will complain
bitterly if you ignore this \* restriction.

Grace-period auto-expediting is disabled for SRCU-fast-updown
srcu\_struct structures because SRCU-fast-updown expedited grace periods
invoke [`synchronize_rcu_expedited()`](#c.synchronize_rcu_expedited "synchronize_rcu_expedited"), IPIs and all. If you need expedited
SRCU-fast-updown grace periods, use [`synchronize_srcu_expedited()`](#c.synchronize_srcu_expedited "synchronize_srcu_expedited").

The [`srcu_read_lock_fast_updown()`](#c.srcu_read_lock_fast_updown "srcu_read_lock_fast_updown") function can be invoked only from
those contexts where RCU is watching, that is, from contexts where
it would be legal to invoke [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock"). Otherwise, lockdep will
complain.

struct srcu\_ctr \_\_percpu \*srcu\_down\_read\_fast(struct srcu\_struct \*ssp)
:   register a new reader for an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to register the new reader.

**Description**

Enter a semaphore-like SRCU read-side critical section, but for
a light-weight `smp_mb()`-free reader. See [`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast") and
[`srcu_down_read()`](#c.srcu_down_read "srcu_down_read") for more information.

The same srcu\_struct may be used concurrently by [`srcu_down_read_fast()`](#c.srcu_down_read_fast "srcu_down_read_fast")
and [`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast"). However, the same definition/initialization
requirements called out for `srcu_read_lock_safe()` apply.

int srcu\_read\_lock\_nmisafe(struct srcu\_struct \*ssp)
:   register a new reader for an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to register the new reader.

**Description**

Enter an SRCU read-side critical section, but in an NMI-safe manner.
See [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") for more information.

If [`srcu_read_lock_nmisafe()`](#c.srcu_read_lock_nmisafe "srcu_read_lock_nmisafe") is ever used on an srcu\_struct structure,
then none of the other flavors may be used, whether before, during,
or after.

int srcu\_down\_read(struct srcu\_struct \*ssp)
:   register a new reader for an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to register the new reader.

**Description**

Enter a semaphore-like SRCU read-side critical section. Note that
SRCU read-side critical sections may be nested. However, it is
illegal to call anything that waits on an SRCU grace period for the
same srcu\_struct, whether directly or indirectly. Please note that
one way to indirectly wait on an SRCU grace period is to acquire
a mutex that is held elsewhere while calling [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") or
[`synchronize_srcu_expedited()`](#c.synchronize_srcu_expedited "synchronize_srcu_expedited"). But if you want lockdep to help you
keep this stuff straight, you should instead use [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock").

The semaphore-like nature of [`srcu_down_read()`](#c.srcu_down_read "srcu_down_read") means that the matching
[`srcu_up_read()`](#c.srcu_up_read "srcu_up_read") can be invoked from some other context, for example,
from some other task or from an irq handler. However, neither
[`srcu_down_read()`](#c.srcu_down_read "srcu_down_read") nor [`srcu_up_read()`](#c.srcu_up_read "srcu_up_read") may be invoked from an NMI handler.

Calls to [`srcu_down_read()`](#c.srcu_down_read "srcu_down_read") may be nested, similar to the manner in
which calls to `down_read()` may be nested. The same srcu\_struct may be
used concurrently by [`srcu_down_read()`](#c.srcu_down_read "srcu_down_read") and [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock").

void srcu\_read\_unlock(struct srcu\_struct \*ssp, int idx)
:   unregister a old reader from an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to unregister the old reader.

`int idx`
:   return value from corresponding [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock").

**Description**

Exit an SRCU read-side critical section.

void srcu\_read\_unlock\_fast(struct srcu\_struct \*ssp, struct srcu\_ctr \_\_percpu \*scp)
:   unregister a old reader from an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to unregister the old reader.

`struct srcu_ctr __percpu *scp`
:   return value from corresponding [`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast").

**Description**

Exit a light-weight SRCU read-side critical section.

void srcu\_read\_unlock\_fast\_updown(struct srcu\_struct \*ssp, struct srcu\_ctr \_\_percpu \*scp)
:   unregister a old reader from an SRCU-fast-updown structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to unregister the old reader.

`struct srcu_ctr __percpu *scp`
:   return value from corresponding [`srcu_read_lock_fast_updown()`](#c.srcu_read_lock_fast_updown "srcu_read_lock_fast_updown").

**Description**

Exit an SRCU-fast-updown read-side critical section.

void srcu\_up\_read\_fast(struct srcu\_struct \*ssp, struct srcu\_ctr \_\_percpu \*scp)
:   unregister a old reader from an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to unregister the old reader.

`struct srcu_ctr __percpu *scp`
:   return value from corresponding [`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast").

**Description**

Exit an SRCU read-side critical section, but not necessarily from
the same context as the maching [`srcu_down_read_fast()`](#c.srcu_down_read_fast "srcu_down_read_fast").

void srcu\_read\_unlock\_nmisafe(struct srcu\_struct \*ssp, int idx)
:   unregister a old reader from an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to unregister the old reader.

`int idx`
:   return value from corresponding [`srcu_read_lock_nmisafe()`](#c.srcu_read_lock_nmisafe "srcu_read_lock_nmisafe").

**Description**

Exit an SRCU read-side critical section, but in an NMI-safe manner.

void srcu\_up\_read(struct srcu\_struct \*ssp, int idx)
:   unregister a old reader from an SRCU-protected structure.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in which to unregister the old reader.

`int idx`
:   return value from corresponding [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock").

**Description**

Exit an SRCU read-side critical section, but not necessarily from
the same context as the maching [`srcu_down_read()`](#c.srcu_down_read "srcu_down_read").

void smp\_mb\_\_after\_srcu\_read\_unlock(void)
:   ensure full ordering after srcu\_read\_unlock

**Parameters**

`void`
:   no arguments

**Description**

Converts the preceding srcu\_read\_unlock into a two-way memory barrier.

Call this after srcu\_read\_unlock, to guarantee that all memory operations
that occur after smp\_mb\_\_after\_srcu\_read\_unlock will appear to happen after
the preceding srcu\_read\_unlock.

void smp\_mb\_\_after\_srcu\_read\_lock(void)
:   ensure full ordering after srcu\_read\_lock

**Parameters**

`void`
:   no arguments

**Description**

Converts the preceding srcu\_read\_lock into a two-way memory barrier.

Call this after srcu\_read\_lock, to guarantee that all memory operations
that occur after smp\_mb\_\_after\_srcu\_read\_lock will appear to happen after
the preceding srcu\_read\_lock.

int init\_srcu\_struct(struct srcu\_struct \*ssp)
:   initialize a sleep-RCU structure

**Parameters**

`struct srcu_struct *ssp`
:   structure to initialize.

**Description**

Use this in place of `DEFINE_SRCU()` and `DEFINE_STATIC_SRCU()`
for non-static srcu\_struct structures that are to be passed to
[`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock"), [`srcu_read_lock_nmisafe()`](#c.srcu_read_lock_nmisafe "srcu_read_lock_nmisafe"), and friends. It is necessary
to invoke this on a given srcu\_struct before passing that srcu\_struct
to any other function. Each srcu\_struct represents a separate domain
of SRCU protection.

int init\_srcu\_struct\_fast(struct srcu\_struct \*ssp)
:   initialize a fast-reader sleep-RCU structure

**Parameters**

`struct srcu_struct *ssp`
:   structure to initialize.

**Description**

Use this in place of `DEFINE_SRCU_FAST()` and `DEFINE_STATIC_SRCU_FAST()`
for non-static srcu\_struct structures that are to be passed to
[`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast") and friends. It is necessary to invoke this on a
given srcu\_struct before passing that srcu\_struct to any other function.
Each srcu\_struct represents a separate domain of SRCU protection.

int init\_srcu\_struct\_fast\_updown(struct srcu\_struct \*ssp)
:   initialize a fast-reader up/down sleep-RCU structure

**Parameters**

`struct srcu_struct *ssp`
:   structure to initialize.

**Description**

Use this function in place of `DEFINE_SRCU_FAST_UPDOWN()` and
`DEFINE_STATIC_SRCU_FAST_UPDOWN()` for non-static srcu\_struct
structures that are to be passed to [`srcu_read_lock_fast_updown()`](#c.srcu_read_lock_fast_updown "srcu_read_lock_fast_updown"),
[`srcu_down_read_fast()`](#c.srcu_down_read_fast "srcu_down_read_fast"), and friends. It is necessary to invoke this on a
given srcu\_struct before passing that srcu\_struct to any other function.
Each srcu\_struct represents a separate domain of SRCU protection.

bool srcu\_readers\_active(struct srcu\_struct \*ssp)
:   returns true if there are readers. and false otherwise

**Parameters**

`struct srcu_struct *ssp`
:   which srcu\_struct to count active readers (holding srcu\_read\_lock).

**Description**

Note that this is not an atomic primitive, and can therefore suffer
severe errors when invoked on an active srcu\_struct. That said, it
can be useful as an error check at cleanup time.

void cleanup\_srcu\_struct(struct srcu\_struct \*ssp)
:   deconstruct a sleep-RCU structure

**Parameters**

`struct srcu_struct *ssp`
:   structure to clean up.

**Description**

Must invoke this after you are finished using a given srcu\_struct that
was initialized via [`init_srcu_struct()`](#c.init_srcu_struct "init_srcu_struct"), else you leak memory.

void call\_srcu(struct srcu\_struct \*ssp, struct rcu\_head \*rhp, rcu\_callback\_t func)
:   Queue a callback for invocation after an SRCU grace period

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct in queue the callback

`struct rcu_head *rhp`
:   structure to be used for queueing the SRCU callback.

`rcu_callback_t func`
:   function to be invoked after the SRCU grace period

**Description**

The callback function will be invoked some time after a full SRCU
grace period elapses, in other words after all pre-existing SRCU
read-side critical sections have completed. However, the callback
function might well execute concurrently with other SRCU read-side
critical sections that started after [`call_srcu()`](#c.call_srcu "call_srcu") was invoked. SRCU
read-side critical sections are delimited by [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock") and
[`srcu_read_unlock()`](#c.srcu_read_unlock "srcu_read_unlock"), and may be nested.

The callback will be invoked from process context, but with bh
disabled. The callback function must therefore be fast and must
not block.

See the description of [`call_rcu()`](#c.call_rcu "call_rcu") for more detailed information on
memory ordering guarantees.

void synchronize\_srcu\_expedited(struct srcu\_struct \*ssp)
:   Brute-force SRCU grace period

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct with which to synchronize.

**Description**

Wait for an SRCU grace period to elapse, but be more aggressive about
spinning rather than blocking when waiting.

Note that [`synchronize_srcu_expedited()`](#c.synchronize_srcu_expedited "synchronize_srcu_expedited") has the same deadlock and
memory-ordering properties as does [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu").

void synchronize\_srcu(struct srcu\_struct \*ssp)
:   wait for prior SRCU read-side critical-section completion

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct with which to synchronize.

**Description**

Wait for the count to drain to zero of both indexes. To avoid the
possible starvation of [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu"), it waits for the count of
the index=!(ssp->srcu\_ctrp - `ssp->sda`->srcu\_ctrs[0]) to drain to zero
at first, and then flip the ->srcu\_ctrp and wait for the count of the
other index.

Can block; must be called from process context.

Note that it is illegal to call [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") from the corresponding
SRCU read-side critical section; doing so will result in deadlock.
However, it is perfectly legal to call [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") on one
srcu\_struct from some other srcu\_struct’s read-side critical section,
as long as the resulting graph of srcu\_structs is acyclic.

There are memory-ordering constraints implied by [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu").
On systems with more than one CPU, when [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") returns,
each CPU is guaranteed to have executed a full memory barrier since
the end of its last corresponding SRCU read-side critical section
whose beginning preceded the call to [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu"). In addition,
each CPU having an SRCU read-side critical section that extends beyond
the return from [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") is guaranteed to have executed a
full memory barrier after the beginning of [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu") and before
the beginning of that SRCU read-side critical section. Note that these
guarantees include CPUs that are offline, idle, or executing in user mode,
as well as CPUs that are executing in the kernel.

Furthermore, if CPU A invoked [`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu"), which returned
to its caller on CPU B, then both CPU A and CPU B are guaranteed
to have executed a full memory barrier during the execution of
[`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu"). This guarantee applies even if CPU A and CPU B
are the same CPU, but again only if the system has more than one CPU.

Of course, these memory-ordering guarantees apply only when
[`synchronize_srcu()`](#c.synchronize_srcu "synchronize_srcu"), [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock"), and [`srcu_read_unlock()`](#c.srcu_read_unlock "srcu_read_unlock") are
passed the same srcu\_struct structure.

Implementation of these memory-ordering guarantees is similar to
that of [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu").

If SRCU is likely idle as determined by `srcu_should_expedite()`,
expedite the first request. This semantic was provided by Classic SRCU,
and is relied upon by its users, so TREE SRCU must also provide it.
Note that detecting idleness is heuristic and subject to both false
positives and negatives.

unsigned long get\_state\_synchronize\_srcu(struct srcu\_struct \*ssp)
:   Provide an end-of-grace-period cookie

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct to provide cookie for.

**Description**

This function returns a cookie that can be passed to
[`poll_state_synchronize_srcu()`](#c.poll_state_synchronize_srcu "poll_state_synchronize_srcu"), which will return true if a full grace
period has elapsed in the meantime. It is the caller’s responsibility
to make sure that grace period happens, for example, by invoking
[`call_srcu()`](#c.call_srcu "call_srcu") after return from [`get_state_synchronize_srcu()`](#c.get_state_synchronize_srcu "get_state_synchronize_srcu").

unsigned long start\_poll\_synchronize\_srcu(struct srcu\_struct \*ssp)
:   Provide cookie and start grace period

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct to provide cookie for.

**Description**

This function returns a cookie that can be passed to
[`poll_state_synchronize_srcu()`](#c.poll_state_synchronize_srcu "poll_state_synchronize_srcu"), which will return true if a full grace
period has elapsed in the meantime. Unlike [`get_state_synchronize_srcu()`](#c.get_state_synchronize_srcu "get_state_synchronize_srcu"),
this function also ensures that any needed SRCU grace period will be
started. This convenience does come at a cost in terms of CPU overhead.

bool poll\_state\_synchronize\_srcu(struct srcu\_struct \*ssp, unsigned long cookie)
:   Has cookie’s grace period ended?

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct to provide cookie for.

`unsigned long cookie`
:   Return value from [`get_state_synchronize_srcu()`](#c.get_state_synchronize_srcu "get_state_synchronize_srcu") or [`start_poll_synchronize_srcu()`](#c.start_poll_synchronize_srcu "start_poll_synchronize_srcu").

**Description**

This function takes the cookie that was returned from either
[`get_state_synchronize_srcu()`](#c.get_state_synchronize_srcu "get_state_synchronize_srcu") or [`start_poll_synchronize_srcu()`](#c.start_poll_synchronize_srcu "start_poll_synchronize_srcu"), and
returns **true** if an SRCU grace period elapsed since the time that the
cookie was created.

Because cookies are finite in size, wrapping/overflow is possible.
This is more pronounced on 32-bit systems where cookies are 32 bits,
where in theory wrapping could happen in about 14 hours assuming
25-microsecond expedited SRCU grace periods. However, a more likely
overflow lower bound is on the order of 24 days in the case of
one-millisecond SRCU grace periods. Of course, wrapping in a 64-bit
system requires geologic timespans, as in more than seven million years
even for expedited SRCU grace periods.

Wrapping/overflow is much more of an issue for CONFIG\_SMP=n systems
that also have CONFIG\_PREEMPTION=n, which selects Tiny SRCU. This uses
a 16-bit cookie, which rcutorture routinely wraps in a matter of a
few minutes. If this proves to be a problem, this counter will be
expanded to the same size as for Tree SRCU.

void srcu\_barrier(struct srcu\_struct \*ssp)
:   Wait until all in-flight [`call_srcu()`](#c.call_srcu "call_srcu") callbacks complete.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct on which to wait for in-flight callbacks.

void srcu\_expedite\_current(struct srcu\_struct \*ssp)
:   Expedite the current SRCU grace period

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct to expedite.

**Description**

Cause the current SRCU grace period to become expedited. The grace
period following the current one might also be expedited. If there is
no current grace period, one might be created. If the current grace
period is currently sleeping, that sleep will complete before expediting
will take effect.

unsigned long srcu\_batches\_completed(struct srcu\_struct \*ssp)
:   return batches completed.

**Parameters**

`struct srcu_struct *ssp`
:   srcu\_struct on which to report batch completion.

**Description**

Report the number of batches, correlated with, but not necessarily
precisely the same as, the number of grace periods that have elapsed.

void hlist\_bl\_del\_rcu(struct hlist\_bl\_node \*n)
:   deletes entry from hash list without re-initialization

**Parameters**

`struct hlist_bl_node *n`
:   the element to delete from the hash list.

**Note**

`hlist_bl_unhashed()` on entry does not return true after this,
the entry is in an undefined state. It is useful for RCU based
lockfree traversal.

In particular, it means that we can not poison the forward
pointers that may still be used for walking the hash list.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_bl_add_head_rcu()`](#c.hlist_bl_add_head_rcu "hlist_bl_add_head_rcu")
or [`hlist_bl_del_rcu()`](#c.hlist_bl_del_rcu "hlist_bl_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
`hlist_bl_for_each_entry()`.

void hlist\_bl\_add\_head\_rcu(struct hlist\_bl\_node \*n, struct hlist\_bl\_head \*h)

**Parameters**

`struct hlist_bl_node *n`
:   the element to add to the hash list.

`struct hlist_bl_head *h`
:   the list to add to.

**Description**

Adds the specified element to the specified hlist\_bl,
while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_bl_add_head_rcu()`](#c.hlist_bl_add_head_rcu "hlist_bl_add_head_rcu")
or [`hlist_bl_del_rcu()`](#c.hlist_bl_del_rcu "hlist_bl_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_bl_for_each_entry_rcu()`](#c.hlist_bl_for_each_entry_rcu "hlist_bl_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs. Regardless of the type of CPU, the
list-traversal primitive must be guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

hlist\_bl\_for\_each\_entry\_rcu

`hlist_bl_for_each_entry_rcu (tpos, pos, head, member)`

> iterate over rcu list of given type

**Parameters**

`tpos`
:   the type \* to use as a loop cursor.

`pos`
:   the `struct hlist_bl_node` to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the hlist\_bl\_node within the struct.

hlist\_bl\_for\_each\_entry\_continue\_rcu

`hlist_bl_for_each_entry_continue_rcu (tpos, pos, member)`

> continue iteration over list of given type

**Parameters**

`tpos`
:   the type \* to use as a loop cursor.

`pos`
:   the `struct hlist_bl_node` to use as a loop cursor.

`member`
:   the name of the hlist\_bl\_node within the struct.

**Description**

Continue to iterate over list of given type, continuing after
the current position which must have been in the list when the RCU read
lock was taken.
This would typically require either that you obtained the node from a
previous walk of the list in the same RCU read-side critical section, or
that you held some sort of non-RCU reference (such as a reference count)
to keep the node alive *and* in the list.

list\_for\_each\_rcu

`list_for_each_rcu (pos, head)`

> Iterate over a list in an RCU-safe fashion

**Parameters**

`pos`
:   the `struct list_head` to use as a loop cursor.

`head`
:   the head for your list.

list\_tail\_rcu

`list_tail_rcu (head)`

> returns the prev pointer of the head of the list

**Parameters**

`head`
:   the head of the list

**Note**

This should only be used with the list header, and even then
only if [`list_del()`](list.html#c.list_del "list_del") and similar primitives are not also used on the
list header.

void list\_add\_rcu(struct list\_head \*new, struct list\_head \*head)
:   add a new entry to rcu-protected list

**Parameters**

`struct list_head *new`
:   new entry to be added

`struct list_head *head`
:   list head to add it after

**Description**

Insert a new entry after the specified head.
This is good for implementing stacks.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu")
or [`list_del_rcu()`](#c.list_del_rcu "list_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`list_for_each_entry_rcu()`](#c.list_for_each_entry_rcu "list_for_each_entry_rcu").

void list\_add\_tail\_rcu(struct list\_head \*new, struct list\_head \*head)
:   add a new entry to rcu-protected list

**Parameters**

`struct list_head *new`
:   new entry to be added

`struct list_head *head`
:   list head to add it before

**Description**

Insert a new entry before the specified head.
This is useful for implementing queues.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`list_add_tail_rcu()`](#c.list_add_tail_rcu "list_add_tail_rcu")
or [`list_del_rcu()`](#c.list_del_rcu "list_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`list_for_each_entry_rcu()`](#c.list_for_each_entry_rcu "list_for_each_entry_rcu").

void list\_del\_rcu(struct list\_head \*entry)
:   deletes entry from list without re-initialization

**Parameters**

`struct list_head *entry`
:   the element to delete from the list.

**Note**

[`list_empty()`](list.html#c.list_empty "list_empty") on entry does not return true after this,
the entry is in an undefined state. It is useful for RCU based
lockfree traversal.

In particular, it means that we can not poison the forward
pointers that may still be used for walking the list.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`list_del_rcu()`](#c.list_del_rcu "list_del_rcu")
or [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`list_for_each_entry_rcu()`](#c.list_for_each_entry_rcu "list_for_each_entry_rcu").

Note that the caller is not permitted to immediately free
the newly deleted entry. Instead, either [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu")
or [`call_rcu()`](#c.call_rcu "call_rcu") must be used to defer freeing until an RCU
grace period has elapsed.

void list\_bidir\_del\_rcu(struct list\_head \*entry)
:   deletes entry from list without re-initialization

**Parameters**

`struct list_head *entry`
:   the element to delete from the list.

**Description**

In contrast to [`list_del_rcu()`](#c.list_del_rcu "list_del_rcu") doesn’t poison the prev pointer thus
allowing backwards traversal via `list_bidir_prev_rcu()`.

**Note**

[`list_empty()`](list.html#c.list_empty "list_empty") on entry does not return true after this because
the entry is in a special undefined state that permits RCU-based
lockfree reverse traversal. In particular this means that we can not
poison the forward and backwards pointers that may still be used for
walking the list.

The caller must take whatever precautions are necessary (such as
holding appropriate locks) to avoid racing with another list-mutation
primitive, such as [`list_bidir_del_rcu()`](#c.list_bidir_del_rcu "list_bidir_del_rcu") or [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu"), running on
this same list. However, it is perfectly legal to run concurrently
with the \_rcu list-traversal primitives, such as
[`list_for_each_entry_rcu()`](#c.list_for_each_entry_rcu "list_for_each_entry_rcu").

Note that [`list_del_rcu()`](#c.list_del_rcu "list_del_rcu") and [`list_bidir_del_rcu()`](#c.list_bidir_del_rcu "list_bidir_del_rcu") must not be used on
the same list.

Note that the caller is not permitted to immediately free
the newly deleted entry. Instead, either [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu")
or [`call_rcu()`](#c.call_rcu "call_rcu") must be used to defer freeing until an RCU
grace period has elapsed.

void hlist\_del\_init\_rcu(struct hlist\_node \*n)
:   deletes entry from hash list with re-initialization

**Parameters**

`struct hlist_node *n`
:   the element to delete from the hash list.

**Note**

`list_unhashed()` on the node return true after this. It is
useful for RCU based read lockfree traversal if the writer side
must know if the list entry is still hashed or already unhashed.

In particular, it means that we can not poison the forward pointers
that may still be used for walking the hash list and we can only
zero the pprev pointer so `list_unhashed()` will return true after
this.

The caller must take whatever precautions are necessary (such as
holding appropriate locks) to avoid racing with another
list-mutation primitive, such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu") or
[`hlist_del_rcu()`](#c.hlist_del_rcu "hlist_del_rcu"), running on this same list. However, it is
perfectly legal to run concurrently with the \_rcu list-traversal
primitives, such as [`hlist_for_each_entry_rcu()`](#c.hlist_for_each_entry_rcu "hlist_for_each_entry_rcu").

void list\_replace\_rcu(struct list\_head \*old, struct list\_head \*new)
:   replace old entry by new one

**Parameters**

`struct list_head *old`
:   the element to be replaced

`struct list_head *new`
:   the new element to insert

**Description**

The **old** entry will be replaced with the **new** entry atomically from
the perspective of concurrent readers. It is the caller’s responsibility
to synchronize with concurrent updaters, if any.

**Note**

**old** should not be empty.

void list\_splice\_rcu(struct list\_head \*list, struct list\_head \*head)
:   splice a non-RCU list into an RCU-protected list, designed for stacks.

**Parameters**

`struct list_head *list`
:   the non RCU-protected list to splice

`struct list_head *head`
:   the place in the existing RCU-protected list to splice

**Description**

The list pointed to by **head** can be RCU-read traversed concurrently with
this function.

void \_\_list\_splice\_init\_rcu(struct list\_head \*list, struct list\_head \*prev, struct list\_head \*next, void (\*sync)(void))
:   join an RCU-protected list into an existing list.

**Parameters**

`struct list_head *list`
:   the RCU-protected list to splice

`struct list_head *prev`
:   points to the last element of the existing list

`struct list_head *next`
:   points to the first element of the existing list

`void (*sync)(void)`
:   synchronize\_rcu, synchronize\_rcu\_expedited, ...

**Description**

The list pointed to by **prev** and **next** can be RCU-read traversed
concurrently with this function.

Note that this function blocks.

Important note: the caller must take whatever action is necessary to prevent
any other updates to the existing list. In principle, it is possible to
modify the list as soon as `sync()` begins execution. If this sort of thing
becomes necessary, an alternative version based on [`call_rcu()`](#c.call_rcu "call_rcu") could be
created. But only if -really- needed -- there is no shortage of RCU API
members.

void list\_splice\_init\_rcu(struct list\_head \*list, struct list\_head \*head, void (\*sync)(void))
:   splice an RCU-protected list into an existing list, designed for stacks.

**Parameters**

`struct list_head *list`
:   the RCU-protected list to splice

`struct list_head *head`
:   the place in the existing list to splice the first list into

`void (*sync)(void)`
:   synchronize\_rcu, synchronize\_rcu\_expedited, ...

void list\_splice\_tail\_init\_rcu(struct list\_head \*list, struct list\_head \*head, void (\*sync)(void))
:   splice an RCU-protected list into an existing list, designed for queues.

**Parameters**

`struct list_head *list`
:   the RCU-protected list to splice

`struct list_head *head`
:   the place in the existing list to splice the first list into

`void (*sync)(void)`
:   synchronize\_rcu, synchronize\_rcu\_expedited, ...

list\_entry\_rcu

`list_entry_rcu (ptr, type, member)`

> get the struct for this entry

**Parameters**

`ptr`
:   the `struct list_head` pointer.

`type`
:   the type of the `struct this` is embedded in.

`member`
:   the name of the list\_head within the struct.

**Description**

This primitive may safely run concurrently with the \_rcu list-mutation
primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu") as long as it’s guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

list\_first\_or\_null\_rcu

`list_first_or_null_rcu (ptr, type, member)`

> get the first element from a list

**Parameters**

`ptr`
:   the list head to take the element from.

`type`
:   the type of the `struct this` is embedded in.

`member`
:   the name of the list\_head within the struct.

**Description**

Note that if the list is empty, it returns NULL.

This primitive may safely run concurrently with the \_rcu list-mutation
primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu") as long as it’s guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

list\_next\_or\_null\_rcu

`list_next_or_null_rcu (head, ptr, type, member)`

> get the next element from a list

**Parameters**

`head`
:   the head for the list.

`ptr`
:   the list head to take the next element from.

`type`
:   the type of the `struct this` is embedded in.

`member`
:   the name of the list\_head within the struct.

**Description**

Note that if the ptr is at the end of the list, NULL is returned.

This primitive may safely run concurrently with the \_rcu list-mutation
primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu") as long as it’s guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

list\_for\_each\_entry\_rcu

`list_for_each_entry_rcu (pos, head, member, cond...)`

> iterate over rcu list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the list\_head within the struct.

`cond...`
:   optional lockdep expression if called from non-RCU protection.

**Description**

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

list\_for\_each\_entry\_srcu

`list_for_each_entry_srcu (pos, head, member, cond)`

> iterate over rcu list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the list\_head within the struct.

`cond`
:   lockdep expression for the lock required to traverse the list.

**Description**

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu")
as long as the traversal is guarded by [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock").
The lockdep expression [`srcu_read_lock_held()`](#c.srcu_read_lock_held "srcu_read_lock_held") can be passed as the
cond argument from read side.

list\_entry\_lockless

`list_entry_lockless (ptr, type, member)`

> get the struct for this entry

**Parameters**

`ptr`
:   the `struct list_head` pointer.

`type`
:   the type of the `struct this` is embedded in.

`member`
:   the name of the list\_head within the struct.

**Description**

This primitive may safely run concurrently with the \_rcu
list-mutation primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu"), but requires some
implicit RCU read-side guarding. One example is running within a special
exception-time environment where preemption is disabled and where lockdep
cannot be invoked. Another example is when items are added to the list,
but never deleted.

list\_for\_each\_entry\_lockless

`list_for_each_entry_lockless (pos, head, member)`

> iterate over rcu list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the list\_struct within the struct.

**Description**

This primitive may safely run concurrently with the \_rcu
list-mutation primitives such as [`list_add_rcu()`](#c.list_add_rcu "list_add_rcu"), but requires some
implicit RCU read-side guarding. One example is running within a special
exception-time environment where preemption is disabled and where lockdep
cannot be invoked. Another example is when items are added to the list,
but never deleted.

list\_for\_each\_entry\_continue\_rcu

`list_for_each_entry_continue_rcu (pos, head, member)`

> continue iteration over list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the list\_head within the struct.

**Description**

Continue to iterate over list of given type, continuing after
the current position which must have been in the list when the RCU read
lock was taken.
This would typically require either that you obtained the node from a
previous walk of the list in the same RCU read-side critical section, or
that you held some sort of non-RCU reference (such as a reference count)
to keep the node alive *and* in the list.

This iterator is similar to [`list_for_each_entry_from_rcu()`](#c.list_for_each_entry_from_rcu "list_for_each_entry_from_rcu") except
this starts after the given position and that one starts at the given
position.

list\_for\_each\_entry\_from\_rcu

`list_for_each_entry_from_rcu (pos, head, member)`

> iterate over a list from current point

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the list\_node within the struct.

**Description**

Iterate over the tail of a list starting from a given position,
which must have been in the list when the RCU read lock was taken.
This would typically require either that you obtained the node from a
previous walk of the list in the same RCU read-side critical section, or
that you held some sort of non-RCU reference (such as a reference count)
to keep the node alive *and* in the list.

This iterator is similar to [`list_for_each_entry_continue_rcu()`](#c.list_for_each_entry_continue_rcu "list_for_each_entry_continue_rcu") except
this starts from the given position and that one starts from the position
after the given position.

void hlist\_del\_rcu(struct hlist\_node \*n)
:   deletes entry from hash list without re-initialization

**Parameters**

`struct hlist_node *n`
:   the element to delete from the hash list.

**Note**

`list_unhashed()` on entry does not return true after this,
the entry is in an undefined state. It is useful for RCU based
lockfree traversal.

In particular, it means that we can not poison the forward
pointers that may still be used for walking the hash list.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
or [`hlist_del_rcu()`](#c.hlist_del_rcu "hlist_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_for_each_entry()`](list.html#c.hlist_for_each_entry "hlist_for_each_entry").

void hlist\_replace\_rcu(struct hlist\_node \*old, struct hlist\_node \*new)
:   replace old entry by new one

**Parameters**

`struct hlist_node *old`
:   the element to be replaced

`struct hlist_node *new`
:   the new element to insert

**Description**

The **old** entry will be replaced with the **new** entry atomically from
the perspective of concurrent readers. It is the caller’s responsibility
to synchronize with concurrent updaters, if any.

void hlists\_swap\_heads\_rcu(struct hlist\_head \*left, struct hlist\_head \*right)
:   swap the lists the hlist heads point to

**Parameters**

`struct hlist_head *left`
:   The hlist head on the left

`struct hlist_head *right`
:   The hlist head on the right

**Description**

The lists start out as [**left** ][node1 ... ] and
:   [**right** ][node2 ... ]

The lists end up as [**left** ][node2 ... ]
:   [**right** ][node1 ... ]

void hlist\_add\_head\_rcu(struct hlist\_node \*n, struct hlist\_head \*h)

**Parameters**

`struct hlist_node *n`
:   the element to add to the hash list.

`struct hlist_head *h`
:   the list to add to.

**Description**

Adds the specified element to the specified hlist,
while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
or [`hlist_del_rcu()`](#c.hlist_del_rcu "hlist_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_for_each_entry_rcu()`](#c.hlist_for_each_entry_rcu "hlist_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs. Regardless of the type of CPU, the
list-traversal primitive must be guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

void hlist\_add\_tail\_rcu(struct hlist\_node \*n, struct hlist\_head \*h)

**Parameters**

`struct hlist_node *n`
:   the element to add to the hash list.

`struct hlist_head *h`
:   the list to add to.

**Description**

Adds the specified element to the specified hlist,
while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
or [`hlist_del_rcu()`](#c.hlist_del_rcu "hlist_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_for_each_entry_rcu()`](#c.hlist_for_each_entry_rcu "hlist_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs. Regardless of the type of CPU, the
list-traversal primitive must be guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

void hlist\_add\_before\_rcu(struct hlist\_node \*n, struct hlist\_node \*next)

**Parameters**

`struct hlist_node *n`
:   the new element to add to the hash list.

`struct hlist_node *next`
:   the existing element to add the new element before.

**Description**

Adds the specified element to the specified hlist
before the specified node while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
or [`hlist_del_rcu()`](#c.hlist_del_rcu "hlist_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_for_each_entry_rcu()`](#c.hlist_for_each_entry_rcu "hlist_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs.

void hlist\_add\_behind\_rcu(struct hlist\_node \*n, struct hlist\_node \*prev)

**Parameters**

`struct hlist_node *n`
:   the new element to add to the hash list.

`struct hlist_node *prev`
:   the existing element to add the new element after.

**Description**

Adds the specified element to the specified hlist
after the specified node while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
or [`hlist_del_rcu()`](#c.hlist_del_rcu "hlist_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_for_each_entry_rcu()`](#c.hlist_for_each_entry_rcu "hlist_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs.

hlist\_for\_each\_entry\_rcu

`hlist_for_each_entry_rcu (pos, head, member, cond...)`

> iterate over rcu list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the hlist\_node within the struct.

`cond...`
:   optional lockdep expression if called from non-RCU protection.

**Description**

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

hlist\_for\_each\_entry\_srcu

`hlist_for_each_entry_srcu (pos, head, member, cond)`

> iterate over rcu list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the hlist\_node within the struct.

`cond`
:   lockdep expression for the lock required to traverse the list.

**Description**

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`srcu_read_lock()`](#c.srcu_read_lock "srcu_read_lock").
The lockdep expression [`srcu_read_lock_held()`](#c.srcu_read_lock_held "srcu_read_lock_held") can be passed as the
cond argument from read side.

hlist\_for\_each\_entry\_rcu\_notrace

`hlist_for_each_entry_rcu_notrace (pos, head, member)`

> iterate over rcu list of given type (for tracing)

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the hlist\_node within the struct.

**Description**

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

This is the same as [`hlist_for_each_entry_rcu()`](#c.hlist_for_each_entry_rcu "hlist_for_each_entry_rcu") except that it does
not do any RCU debugging or tracing.

hlist\_for\_each\_entry\_rcu\_bh

`hlist_for_each_entry_rcu_bh (pos, head, member)`

> iterate over rcu list of given type

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`head`
:   the head for your list.

`member`
:   the name of the hlist\_node within the struct.

**Description**

This list-traversal primitive may safely run concurrently with
the \_rcu list-mutation primitives such as [`hlist_add_head_rcu()`](#c.hlist_add_head_rcu "hlist_add_head_rcu")
as long as the traversal is guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

hlist\_for\_each\_entry\_continue\_rcu

`hlist_for_each_entry_continue_rcu (pos, member)`

> iterate over a hlist continuing after current point

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`member`
:   the name of the hlist\_node within the struct.

hlist\_for\_each\_entry\_continue\_rcu\_bh

`hlist_for_each_entry_continue_rcu_bh (pos, member)`

> iterate over a hlist continuing after current point

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`member`
:   the name of the hlist\_node within the struct.

hlist\_for\_each\_entry\_from\_rcu

`hlist_for_each_entry_from_rcu (pos, member)`

> iterate over a hlist continuing from current point

**Parameters**

`pos`
:   the type \* to use as a loop cursor.

`member`
:   the name of the hlist\_node within the struct.

void hlist\_nulls\_del\_init\_rcu(struct hlist\_nulls\_node \*n)
:   deletes entry from hash list with re-initialization

**Parameters**

`struct hlist_nulls_node *n`
:   the element to delete from the hash list.

**Note**

`hlist_nulls_unhashed()` on the node return true after this. It is
useful for RCU based read lockfree traversal if the writer side
must know if the list entry is still hashed or already unhashed.

In particular, it means that we can not poison the forward pointers
that may still be used for walking the hash list and we can only
zero the pprev pointer so `list_unhashed()` will return true after
this.

The caller must take whatever precautions are necessary (such as
holding appropriate locks) to avoid racing with another
list-mutation primitive, such as [`hlist_nulls_add_head_rcu()`](#c.hlist_nulls_add_head_rcu "hlist_nulls_add_head_rcu") or
[`hlist_nulls_del_rcu()`](#c.hlist_nulls_del_rcu "hlist_nulls_del_rcu"), running on this same list. However, it is
perfectly legal to run concurrently with the \_rcu list-traversal
primitives, such as [`hlist_nulls_for_each_entry_rcu()`](#c.hlist_nulls_for_each_entry_rcu "hlist_nulls_for_each_entry_rcu").

hlist\_nulls\_first\_rcu

`hlist_nulls_first_rcu (head)`

> returns the first element of the hash list.

**Parameters**

`head`
:   the head of the list.

hlist\_nulls\_next\_rcu

`hlist_nulls_next_rcu (node)`

> returns the element of the list after **node**.

**Parameters**

`node`
:   element of the list.

hlist\_nulls\_pprev\_rcu

`hlist_nulls_pprev_rcu (node)`

> returns the dereferenced pprev of **node**.

**Parameters**

`node`
:   element of the list.

void hlist\_nulls\_del\_rcu(struct hlist\_nulls\_node \*n)
:   deletes entry from hash list without re-initialization

**Parameters**

`struct hlist_nulls_node *n`
:   the element to delete from the hash list.

**Note**

`hlist_nulls_unhashed()` on entry does not return true after this,
the entry is in an undefined state. It is useful for RCU based
lockfree traversal.

In particular, it means that we can not poison the forward
pointers that may still be used for walking the hash list.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_nulls_add_head_rcu()`](#c.hlist_nulls_add_head_rcu "hlist_nulls_add_head_rcu")
or [`hlist_nulls_del_rcu()`](#c.hlist_nulls_del_rcu "hlist_nulls_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
`hlist_nulls_for_each_entry()`.

void hlist\_nulls\_add\_head\_rcu(struct hlist\_nulls\_node \*n, struct hlist\_nulls\_head \*h)

**Parameters**

`struct hlist_nulls_node *n`
:   the element to add to the hash list.

`struct hlist_nulls_head *h`
:   the list to add to.

**Description**

Adds the specified element to the specified hlist\_nulls,
while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_nulls_add_head_rcu()`](#c.hlist_nulls_add_head_rcu "hlist_nulls_add_head_rcu")
or [`hlist_nulls_del_rcu()`](#c.hlist_nulls_del_rcu "hlist_nulls_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_nulls_for_each_entry_rcu()`](#c.hlist_nulls_for_each_entry_rcu "hlist_nulls_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs. Regardless of the type of CPU, the
list-traversal primitive must be guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

void hlist\_nulls\_add\_tail\_rcu(struct hlist\_nulls\_node \*n, struct hlist\_nulls\_head \*h)

**Parameters**

`struct hlist_nulls_node *n`
:   the element to add to the hash list.

`struct hlist_nulls_head *h`
:   the list to add to.

**Description**

Adds the specified element to the specified hlist\_nulls,
while permitting racing traversals.

The caller must take whatever precautions are necessary
(such as holding appropriate locks) to avoid racing
with another list-mutation primitive, such as [`hlist_nulls_add_head_rcu()`](#c.hlist_nulls_add_head_rcu "hlist_nulls_add_head_rcu")
or [`hlist_nulls_del_rcu()`](#c.hlist_nulls_del_rcu "hlist_nulls_del_rcu"), running on this same list.
However, it is perfectly legal to run concurrently with
the \_rcu list-traversal primitives, such as
[`hlist_nulls_for_each_entry_rcu()`](#c.hlist_nulls_for_each_entry_rcu "hlist_nulls_for_each_entry_rcu"), used to prevent memory-consistency
problems on Alpha CPUs. Regardless of the type of CPU, the
list-traversal primitive must be guarded by [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

void hlist\_nulls\_replace\_rcu(struct hlist\_nulls\_node \*old, struct hlist\_nulls\_node \*new)
:   replace an old entry by a new one

**Parameters**

`struct hlist_nulls_node *old`
:   the element to be replaced

`struct hlist_nulls_node *new`
:   the new element to insert

**Description**

Replace the old entry with the new one in a RCU-protected hlist\_nulls, while
permitting racing traversals.

The caller must take whatever precautions are necessary (such as holding
appropriate locks) to avoid racing with another list-mutation primitive, such
as [`hlist_nulls_add_head_rcu()`](#c.hlist_nulls_add_head_rcu "hlist_nulls_add_head_rcu") or [`hlist_nulls_del_rcu()`](#c.hlist_nulls_del_rcu "hlist_nulls_del_rcu"), running on this same
list. However, it is perfectly legal to run concurrently with the \_rcu
list-traversal primitives, such as [`hlist_nulls_for_each_entry_rcu()`](#c.hlist_nulls_for_each_entry_rcu "hlist_nulls_for_each_entry_rcu").

void hlist\_nulls\_replace\_init\_rcu(struct hlist\_nulls\_node \*old, struct hlist\_nulls\_node \*new)
:   replace an old entry by a new one and initialize the old

**Parameters**

`struct hlist_nulls_node *old`
:   the element to be replaced

`struct hlist_nulls_node *new`
:   the new element to insert

**Description**

Replace the old entry with the new one in a RCU-protected hlist\_nulls, while
permitting racing traversals, and reinitialize the old entry.

**Note**

**old** must be hashed.

The caller must take whatever precautions are necessary (such as holding
appropriate locks) to avoid racing with another list-mutation primitive, such
as [`hlist_nulls_add_head_rcu()`](#c.hlist_nulls_add_head_rcu "hlist_nulls_add_head_rcu") or [`hlist_nulls_del_rcu()`](#c.hlist_nulls_del_rcu "hlist_nulls_del_rcu"), running on this same
list. However, it is perfectly legal to run concurrently with the \_rcu
list-traversal primitives, such as [`hlist_nulls_for_each_entry_rcu()`](#c.hlist_nulls_for_each_entry_rcu "hlist_nulls_for_each_entry_rcu").

hlist\_nulls\_for\_each\_entry\_rcu

`hlist_nulls_for_each_entry_rcu (tpos, pos, head, member)`

> iterate over rcu list of given type

**Parameters**

`tpos`
:   the type \* to use as a loop cursor.

`pos`
:   the `struct hlist_nulls_node` to use as a loop cursor.

`head`
:   the head of the list.

`member`
:   the name of the hlist\_nulls\_node within the struct.

**Description**

The `barrier()` is needed to make sure compiler doesn’t cache first element [1],
as this loop can be restarted [2]
[1] Documentation/memory-barriers.txt around line 1533
[2] [Using RCU hlist\_nulls to protect list and objects](../RCU/rculist_nulls.html) around line 146

hlist\_nulls\_for\_each\_entry\_safe

`hlist_nulls_for_each_entry_safe (tpos, pos, head, member)`

> iterate over list of given type safe against removal of list entry

**Parameters**

`tpos`
:   the type \* to use as a loop cursor.

`pos`
:   the `struct hlist_nulls_node` to use as a loop cursor.

`head`
:   the head of the list.

`member`
:   the name of the hlist\_nulls\_node within the struct.

bool rcu\_sync\_is\_idle(struct rcu\_sync \*rsp)
:   Are readers permitted to use their fastpaths?

**Parameters**

`struct rcu_sync *rsp`
:   Pointer to rcu\_sync structure to use for synchronization

**Description**

Returns true if readers are permitted to use their fastpaths. Must be
invoked within some flavor of RCU read-side critical section.

void rcu\_sync\_init(struct rcu\_sync \*rsp)
:   Initialize an rcu\_sync structure

**Parameters**

`struct rcu_sync *rsp`
:   Pointer to rcu\_sync structure to be initialized

void rcu\_sync\_func(struct rcu\_head \*rhp)
:   Callback function managing reader access to fastpath

**Parameters**

`struct rcu_head *rhp`
:   Pointer to rcu\_head in rcu\_sync structure to use for synchronization

**Description**

This function is passed to [`call_rcu()`](#c.call_rcu "call_rcu") function by [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter") and
[`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit"), so that it is invoked after a grace period following the
that invocation of enter/exit.

If it is called by [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter") it signals that all the readers were
switched onto slow path.

If it is called by [`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit") it takes action based on events that
have taken place in the meantime, so that closely spaced [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter")
and [`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit") pairs need not wait for a grace period.

If another [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter") is invoked before the grace period
ended, reset state to allow the next [`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit") to let the
readers back onto their fastpaths (after a grace period). If both
another [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter") and its matching [`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit") are invoked
before the grace period ended, re-invoke [`call_rcu()`](#c.call_rcu "call_rcu") on behalf of that
[`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit"). Otherwise, set all state back to idle so that readers
can again use their fastpaths.

void rcu\_sync\_enter(struct rcu\_sync \*rsp)
:   Force readers onto slowpath

**Parameters**

`struct rcu_sync *rsp`
:   Pointer to rcu\_sync structure to use for synchronization

**Description**

This function is used by updaters who need readers to make use of
a slowpath during the update. After this function returns, all
subsequent calls to [`rcu_sync_is_idle()`](#c.rcu_sync_is_idle "rcu_sync_is_idle") will return false, which
tells readers to stay off their fastpaths. A later call to
[`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit") re-enables reader fastpaths.

When called in isolation, [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter") must wait for a grace
period, however, closely spaced calls to [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter") can
optimize away the grace-period wait via a state machine implemented
by [`rcu_sync_enter()`](#c.rcu_sync_enter "rcu_sync_enter"), [`rcu_sync_exit()`](#c.rcu_sync_exit "rcu_sync_exit"), and [`rcu_sync_func()`](#c.rcu_sync_func "rcu_sync_func").

void rcu\_sync\_exit(struct rcu\_sync \*rsp)
:   Allow readers back onto fast path after grace period

**Parameters**

`struct rcu_sync *rsp`
:   Pointer to rcu\_sync structure to use for synchronization

**Description**

This function is used by updaters who have completed, and can therefore
now allow readers to make use of their fastpaths after a grace period
has elapsed. After this grace period has completed, all subsequent
calls to [`rcu_sync_is_idle()`](#c.rcu_sync_is_idle "rcu_sync_is_idle") will return true, which tells readers that
they can once again use their fastpaths.

void rcu\_sync\_dtor(struct rcu\_sync \*rsp)
:   Clean up an rcu\_sync structure

**Parameters**

`struct rcu_sync *rsp`
:   Pointer to rcu\_sync structure to be cleaned up

struct rcu\_tasks\_percpu
:   Per-CPU component of definition for a Tasks-RCU-like mechanism.

**Definition**:

```
struct rcu_tasks_percpu {
    struct rcu_segcblist cblist;
    raw_spinlock_t lock;
    unsigned long rtp_jiffies;
    unsigned long rtp_n_lock_retries;
    struct timer_list lazy_timer;
    unsigned int urgent_gp;
    struct work_struct rtp_work;
    struct irq_work rtp_irq_work;
    struct rcu_head barrier_q_head;
    struct list_head rtp_blkd_tasks;
    struct list_head rtp_exit_list;
    int cpu;
    int index;
    struct rcu_tasks *rtpp;
};
```

**Members**

`cblist`
:   Callback list.

`lock`
:   Lock protecting per-CPU callback list.

`rtp_jiffies`
:   Jiffies counter value for statistics.

`rtp_n_lock_retries`
:   Rough lock-contention statistic.

`lazy_timer`
:   Timer to unlazify callbacks.

`urgent_gp`
:   Number of additional non-lazy grace periods.

`rtp_work`
:   Work queue for invoking callbacks.

`rtp_irq_work`
:   IRQ work queue for deferred wakeups.

`barrier_q_head`
:   RCU callback for barrier operation.

`rtp_blkd_tasks`
:   List of tasks blocked as readers.

`rtp_exit_list`
:   List of tasks in the latter portion of `do_exit()`.

`cpu`
:   CPU number corresponding to this entry.

`index`
:   Index of this CPU in rtpcp\_array of the rcu\_tasks structure.

`rtpp`
:   Pointer to the rcu\_tasks structure.

struct rcu\_tasks
:   Definition for a Tasks-RCU-like mechanism.

**Definition**:

```
struct rcu_tasks {
    struct rcuwait cbs_wait;
    raw_spinlock_t cbs_gbl_lock;
    struct mutex tasks_gp_mutex;
    int gp_state;
    int gp_sleep;
    int init_fract;
    unsigned long gp_jiffies;
    unsigned long gp_start;
    unsigned long tasks_gp_seq;
    unsigned long n_ipis;
    unsigned long n_ipis_fails;
    struct task_struct *kthread_ptr;
    unsigned long lazy_jiffies;
    rcu_tasks_gp_func_t gp_func;
    pregp_func_t pregp_func;
    pertask_func_t pertask_func;
    postscan_func_t postscan_func;
    holdouts_func_t holdouts_func;
    postgp_func_t postgp_func;
    call_rcu_func_t call_func;
    unsigned int wait_state;
    struct rcu_tasks_percpu __percpu *rtpcpu;
    struct rcu_tasks_percpu **rtpcp_array;
    int percpu_enqueue_shift;
    int percpu_enqueue_lim;
    int percpu_dequeue_lim;
    unsigned long percpu_dequeue_gpseq;
    struct mutex barrier_q_mutex;
    atomic_t barrier_q_count;
    struct completion barrier_q_completion;
    unsigned long barrier_q_seq;
    unsigned long barrier_q_start;
    char *name;
    char *kname;
};
```

**Members**

`cbs_wait`
:   RCU wait allowing a new callback to get kthread’s attention.

`cbs_gbl_lock`
:   Lock protecting callback list.

`tasks_gp_mutex`
:   Mutex protecting grace period, needed during mid-boot dead zone.

`gp_state`
:   Grace period’s most recent state transition (debugging).

`gp_sleep`
:   Per-grace-period sleep to prevent CPU-bound looping.

`init_fract`
:   Initial backoff sleep interval.

`gp_jiffies`
:   Time of last **gp\_state** transition.

`gp_start`
:   Most recent grace-period start in jiffies.

`tasks_gp_seq`
:   Number of grace periods completed since boot in upper bits.

`n_ipis`
:   Number of IPIs sent to encourage grace periods to end.

`n_ipis_fails`
:   Number of IPI-send failures.

`kthread_ptr`
:   This flavor’s grace-period/callback-invocation kthread.

`lazy_jiffies`
:   Number of jiffies to allow callbacks to be lazy.

`gp_func`
:   This flavor’s grace-period-wait function.

`pregp_func`
:   This flavor’s pre-grace-period function (optional).

`pertask_func`
:   This flavor’s per-task scan function (optional).

`postscan_func`
:   This flavor’s post-task scan function (optional).

`holdouts_func`
:   This flavor’s holdout-list scan function (optional).

`postgp_func`
:   This flavor’s post-grace-period function (optional).

`call_func`
:   This flavor’s [`call_rcu()`](#c.call_rcu "call_rcu")-equivalent function.

`wait_state`
:   Task state for synchronous grace-period waits (default TASK\_UNINTERRUPTIBLE).

`rtpcpu`
:   This flavor’s rcu\_tasks\_percpu structure.

`rtpcp_array`
:   Array of pointers to rcu\_tasks\_percpu structure of CPUs in cpu\_possible\_mask.

`percpu_enqueue_shift`
:   Shift down CPU ID this much when enqueuing callbacks.

`percpu_enqueue_lim`
:   Number of per-CPU callback queues in use for enqueuing.

`percpu_dequeue_lim`
:   Number of per-CPU callback queues in use for dequeuing.

`percpu_dequeue_gpseq`
:   RCU grace-period number to propagate enqueue limit to dequeuers.

`barrier_q_mutex`
:   Serialize barrier operations.

`barrier_q_count`
:   Number of queues being waited on.

`barrier_q_completion`
:   Barrier wait/wakeup mechanism.

`barrier_q_seq`
:   Sequence number for barrier operations.

`barrier_q_start`
:   Most recent barrier start in jiffies.

`name`
:   This flavor’s textual name.

`kname`
:   This flavor’s kthread name.

void call\_rcu\_tasks(struct rcu\_head \*rhp, rcu\_callback\_t func)
:   Queue an RCU for invocation task-based grace period

**Parameters**

`struct rcu_head *rhp`
:   structure to be used for queueing the RCU updates.

`rcu_callback_t func`
:   actual callback function to be invoked after the grace period

**Description**

The callback function will be invoked some time after a full grace
period elapses, in other words after all currently executing RCU
read-side critical sections have completed. [`call_rcu_tasks()`](#c.call_rcu_tasks "call_rcu_tasks") assumes
that the read-side critical sections end at a voluntary context
switch (not a preemption!), [`cond_resched_tasks_rcu_qs()`](#c.cond_resched_tasks_rcu_qs "cond_resched_tasks_rcu_qs"), entry into idle,
or transition to usermode execution. As such, there are no read-side
primitives analogous to [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock") and [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock") because
this primitive is intended to determine that all tasks have passed
through a safe state, not so much for data-structure synchronization.

See the description of [`call_rcu()`](#c.call_rcu "call_rcu") for more detailed information on
memory ordering guarantees.

void synchronize\_rcu\_tasks(void)
:   wait until an rcu-tasks grace period has elapsed.

**Parameters**

`void`
:   no arguments

**Description**

Control will return to the caller some time after a full rcu-tasks
grace period has elapsed, in other words after all currently
executing rcu-tasks read-side critical sections have elapsed. These
read-side critical sections are delimited by calls to `schedule()`,
[`cond_resched_tasks_rcu_qs()`](#c.cond_resched_tasks_rcu_qs "cond_resched_tasks_rcu_qs"), idle execution, userspace execution, calls
to [`synchronize_rcu_tasks()`](#c.synchronize_rcu_tasks "synchronize_rcu_tasks"), and (in theory, anyway) `cond_resched()`.

This is a very specialized primitive, intended only for a few uses in
tracing and other situations requiring manipulation of function
preambles and profiling hooks. The [`synchronize_rcu_tasks()`](#c.synchronize_rcu_tasks "synchronize_rcu_tasks") function
is not (yet) intended for heavy use from multiple CPUs.

See the description of [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") for more detailed information
on memory ordering guarantees.

void rcu\_barrier\_tasks(void)
:   Wait for in-flight [`call_rcu_tasks()`](#c.call_rcu_tasks "call_rcu_tasks") callbacks.

**Parameters**

`void`
:   no arguments

**Description**

Although the current implementation is guaranteed to wait, it is not
obligated to, for example, if there are no pending callbacks.

void synchronize\_rcu\_tasks\_rude(void)
:   wait for a rude rcu-tasks grace period

**Parameters**

`void`
:   no arguments

**Description**

Control will return to the caller some time after a rude rcu-tasks
grace period has elapsed, in other words after all currently
executing rcu-tasks read-side critical sections have elapsed. These
read-side critical sections are delimited by calls to `schedule()`,
[`cond_resched_tasks_rcu_qs()`](#c.cond_resched_tasks_rcu_qs "cond_resched_tasks_rcu_qs"), userspace execution (which is a schedulable
context), and (in theory, anyway) `cond_resched()`.

This is a very specialized primitive, intended only for a few uses in
tracing and other situations requiring manipulation of function preambles
and profiling hooks. The [`synchronize_rcu_tasks_rude()`](#c.synchronize_rcu_tasks_rude "synchronize_rcu_tasks_rude") function is not
(yet) intended for heavy use from multiple CPUs.

See the description of [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") for more detailed information
on memory ordering guarantees.

void rcu\_cpu\_stall\_reset(void)
:   restart stall-warning timeout for current grace period

**Parameters**

`void`
:   no arguments

**Description**

To perform the reset request from the caller, disable stall detection until
3 fqs loops have passed. This is required to ensure a fresh jiffies is
loaded. It should be safe to do from the fqs loop as enough timer
interrupts and context switches should have passed.

The caller must disable hard irqs.

int rcu\_stall\_chain\_notifier\_register(struct notifier\_block \*n)
:   Add an RCU CPU stall notifier

**Parameters**

`struct notifier_block *n`
:   Entry to add.

**Description**

Adds an RCU CPU stall notifier to an atomic notifier chain.
The **action** passed to a notifier will be **RCU\_STALL\_NOTIFY\_NORM** or
friends. The **data** will be the duration of the stalled grace period,
in jiffies, coerced to a void\* pointer.

Returns 0 on success, `-EEXIST` on error.

int rcu\_stall\_chain\_notifier\_unregister(struct notifier\_block \*n)
:   Remove an RCU CPU stall notifier

**Parameters**

`struct notifier_block *n`
:   Entry to add.

**Description**

Removes an RCU CPU stall notifier from an atomic notifier chain.

Returns zero on success, `-ENOENT` on failure.

struct srcu\_ctr \_\_percpu \*rcu\_read\_lock\_tasks\_trace(void)
:   mark beginning of RCU-trace read-side critical section

**Parameters**

`void`
:   no arguments

**Description**

When [`synchronize_rcu_tasks_trace()`](#c.synchronize_rcu_tasks_trace "synchronize_rcu_tasks_trace") is invoked by one task, then that
task is guaranteed to block until all other tasks exit their read-side
critical sections. Similarly, if `call_rcu_trace()` is invoked on one
task while other tasks are within RCU read-side critical sections,
invocation of the corresponding RCU callback is deferred until after
the all the other tasks exit their critical sections.

For more details, please see the documentation for
[`srcu_read_lock_fast()`](#c.srcu_read_lock_fast "srcu_read_lock_fast"). For a description of how implicit RCU
readers provide the needed ordering for architectures defining the
ARCH\_WANTS\_NO\_INSTR Kconfig option (and thus promising never to trace
code where RCU is not watching), please see the `__srcu_read_lock_fast()`
(non-kerneldoc) header comment. Otherwise, the `smp_mb()` below provided
the needed ordering.

void rcu\_read\_unlock\_tasks\_trace(struct srcu\_ctr \_\_percpu \*scp)
:   mark end of RCU-trace read-side critical section

**Parameters**

`struct srcu_ctr __percpu *scp`
:   return value from corresponding [`rcu_read_lock_tasks_trace()`](#c.rcu_read_lock_tasks_trace "rcu_read_lock_tasks_trace").

**Description**

Pairs with the preceding call to [`rcu_read_lock_tasks_trace()`](#c.rcu_read_lock_tasks_trace "rcu_read_lock_tasks_trace") that
returned the value passed in via scp.

For more details, please see the documentation for [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock").
For memory-ordering information, please see the header comment for the
[`rcu_read_lock_tasks_trace()`](#c.rcu_read_lock_tasks_trace "rcu_read_lock_tasks_trace") function.

void rcu\_read\_lock\_trace(void)
:   mark beginning of RCU-trace read-side critical section

**Parameters**

`void`
:   no arguments

**Description**

When [`synchronize_rcu_tasks_trace()`](#c.synchronize_rcu_tasks_trace "synchronize_rcu_tasks_trace") is invoked by one task, then that
task is guaranteed to block until all other tasks exit their read-side
critical sections. Similarly, if `call_rcu_trace()` is invoked on one
task while other tasks are within RCU read-side critical sections,
invocation of the corresponding RCU callback is deferred until after
the all the other tasks exit their critical sections.

For more details, please see the documentation for [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock").

void rcu\_read\_unlock\_trace(void)
:   mark end of RCU-trace read-side critical section

**Parameters**

`void`
:   no arguments

**Description**

Pairs with a preceding call to [`rcu_read_lock_trace()`](#c.rcu_read_lock_trace "rcu_read_lock_trace"), and nesting is
allowed. Invoking a [`rcu_read_unlock_trace()`](#c.rcu_read_unlock_trace "rcu_read_unlock_trace") when there is no matching
[`rcu_read_lock_trace()`](#c.rcu_read_lock_trace "rcu_read_lock_trace") is verboten, and will result in lockdep complaints.

For more details, please see the documentation for [`rcu_read_unlock()`](#c.rcu_read_unlock "rcu_read_unlock").

void call\_rcu\_tasks\_trace(struct rcu\_head \*rhp, rcu\_callback\_t func)
:   Queue a callback trace task-based grace period

**Parameters**

`struct rcu_head *rhp`
:   structure to be used for queueing the RCU updates.

`rcu_callback_t func`
:   actual callback function to be invoked after the grace period

**Description**

The callback function will be invoked some time after a trace rcu-tasks
grace period elapses, in other words after all currently executing
trace rcu-tasks read-side critical sections have completed. These
read-side critical sections are delimited by calls to [`rcu_read_lock_trace()`](#c.rcu_read_lock_trace "rcu_read_lock_trace")
and [`rcu_read_unlock_trace()`](#c.rcu_read_unlock_trace "rcu_read_unlock_trace").

See the description of [`call_rcu()`](#c.call_rcu "call_rcu") for more detailed information on
memory ordering guarantees.

void synchronize\_rcu\_tasks\_trace(void)
:   wait for a trace rcu-tasks grace period

**Parameters**

`void`
:   no arguments

**Description**

Control will return to the caller some time after a trace rcu-tasks
grace period has elapsed, in other words after all currently executing
trace rcu-tasks read-side critical sections have elapsed. These read-side
critical sections are delimited by calls to [`rcu_read_lock_trace()`](#c.rcu_read_lock_trace "rcu_read_lock_trace")
and [`rcu_read_unlock_trace()`](#c.rcu_read_unlock_trace "rcu_read_unlock_trace").

This is a very specialized primitive, intended only for a few uses in
tracing and other situations requiring manipulation of function preambles
and profiling hooks. The [`synchronize_rcu_tasks_trace()`](#c.synchronize_rcu_tasks_trace "synchronize_rcu_tasks_trace") function is not
(yet) intended for heavy use from multiple CPUs.

See the description of [`synchronize_rcu()`](#c.synchronize_rcu "synchronize_rcu") for more detailed information
on memory ordering guarantees.

void rcu\_barrier\_tasks\_trace(void)
:   Wait for in-flight [`call_rcu_tasks_trace()`](#c.call_rcu_tasks_trace "call_rcu_tasks_trace") callbacks.

**Parameters**

`void`
:   no arguments

**Description**

Note that [`rcu_barrier_tasks_trace()`](#c.rcu_barrier_tasks_trace "rcu_barrier_tasks_trace") is not obligated to actually wait,
for example, if there are no pending callbacks.

void rcu\_tasks\_trace\_expedite\_current(void)
:   Expedite the current Tasks Trace RCU grace period

**Parameters**

`void`
:   no arguments

**Description**

Cause the current Tasks Trace RCU grace period to become expedited.
The grace period following the current one might also be expedited.
If there is no current grace period, one might be created. If the
current grace period is currently sleeping, that sleep will complete
before expediting will take effect.

synchronize\_rcu\_mult

`synchronize_rcu_mult (...)`

> Wait concurrently for multiple grace periods

**Parameters**

`...`
:   List of [`call_rcu()`](#c.call_rcu "call_rcu") functions for different grace periods to wait on

**Description**

This macro waits concurrently for multiple types of RCU grace periods.
For example, synchronize\_rcu\_mult(call\_rcu, call\_rcu\_tasks) would wait
on concurrent RCU and RCU-tasks grace periods. Waiting on a given SRCU
domain requires you to write a wrapper function for that SRCU domain’s
[`call_srcu()`](#c.call_srcu "call_srcu") function, with this wrapper supplying the pointer to the
corresponding srcu\_struct.

Note that [`call_rcu_hurry()`](#c.call_rcu_hurry "call_rcu_hurry") should be used instead of [`call_rcu()`](#c.call_rcu "call_rcu")
because in kernels built with CONFIG\_RCU\_LAZY=y the delay between the
invocation of [`call_rcu()`](#c.call_rcu "call_rcu") and that of the corresponding RCU callback
can be multiple seconds.

The first argument tells Tiny RCU’s `_wait_rcu_gp()` not to
bother waiting for RCU. The reason for this is because anywhere
[`synchronize_rcu_mult()`](#c.synchronize_rcu_mult "synchronize_rcu_mult") can be called is automatically already a full
grace period.

void rcuref\_init(rcuref\_t \*ref, unsigned int cnt)
:   Initialize a rcuref reference count with the given reference count

**Parameters**

`rcuref_t *ref`
:   Pointer to the reference count

`unsigned int cnt`
:   The initial reference count typically ‘1’

unsigned int rcuref\_read(rcuref\_t \*ref)
:   Read the number of held reference counts of a rcuref

**Parameters**

`rcuref_t *ref`
:   Pointer to the reference count

**Return**

The number of held references (0 ... N). The value 0 does not
indicate that it is safe to schedule the object, protected by this reference
counter, for deconstruction.
If you want to know if the reference counter has been marked DEAD (as
signaled by [`rcuref_put()`](#c.rcuref_put "rcuref_put")) please use `rcuread_is_dead()`.

bool rcuref\_is\_dead(rcuref\_t \*ref)
:   Check if the rcuref has been already marked dead

**Parameters**

`rcuref_t *ref`
:   Pointer to the reference count

**Return**

True if the object has been marked DEAD. This signals that a previous
invocation of [`rcuref_put()`](#c.rcuref_put "rcuref_put") returned true on this reference counter meaning
the protected object can safely be scheduled for deconstruction.
Otherwise, returns false.

bool rcuref\_get(rcuref\_t \*ref)
:   Acquire one reference on a rcuref reference count

**Parameters**

`rcuref_t *ref`
:   Pointer to the reference count

**Description**

Similar to [`atomic_inc_not_zero()`](../driver-api/basics.html#c.atomic_inc_not_zero "atomic_inc_not_zero") but saturates at RCUREF\_MAXREF.

Provides no memory ordering, it is assumed the caller has guaranteed the
object memory to be stable (RCU, etc.). It does provide a control dependency
and thereby orders future stores. See documentation in lib/rcuref.c

> True if a reference was successfully acquired

**Return**

False if the attempt to acquire a reference failed. This happens
when the last reference has been put already

bool rcuref\_put\_rcusafe(rcuref\_t \*ref)
:   * Release one reference for a rcuref reference count RCU safe

**Parameters**

`rcuref_t *ref`
:   Pointer to the reference count

**Description**

Provides release memory ordering, such that prior loads and stores are done
before, and provides an acquire ordering on success such that `free()`
must come after.

Can be invoked from contexts, which guarantee that no grace period can
happen which would free the object concurrently if the decrement drops
the last reference and the slowpath races against a concurrent `get()` and
`put()` pair. [`rcu_read_lock()`](#c.rcu_read_lock "rcu_read_lock")’ed and atomic contexts qualify.

> False if there are still active references or the `put()` raced
> with a concurrent `get()`/`put()` pair. Caller is not allowed to
> release the protected object.

**Return**

True if this was the last reference with no future references
possible. This signals the caller that it can safely release the
object which is protected by the reference counter.

bool rcuref\_put(rcuref\_t \*ref)
:   * Release one reference for a rcuref reference count

**Parameters**

`rcuref_t *ref`
:   Pointer to the reference count

**Description**

Can be invoked from any context.

Provides release memory ordering, such that prior loads and stores are done
before, and provides an acquire ordering on success such that `free()`
must come after.

> True if this was the last reference with no future references
> possible. This signals the caller that it can safely schedule the
> object, which is protected by the reference counter, for
> deconstruction.
>
> False if there are still active references or the `put()` raced
> with a concurrent `get()`/`put()` pair. Caller is not allowed to
> deconstruct the protected object.

bool same\_state\_synchronize\_rcu\_full(struct rcu\_gp\_oldstate \*rgosp1, struct rcu\_gp\_oldstate \*rgosp2)
:   Are two old-state values identical?

**Parameters**

`struct rcu_gp_oldstate *rgosp1`
:   First old-state value.

`struct rcu_gp_oldstate *rgosp2`
:   Second old-state value.

**Description**

The two old-state values must have been obtained from either
[`get_state_synchronize_rcu_full()`](#c.get_state_synchronize_rcu_full "get_state_synchronize_rcu_full"), [`start_poll_synchronize_rcu_full()`](#c.start_poll_synchronize_rcu_full "start_poll_synchronize_rcu_full"),
or [`get_completed_synchronize_rcu_full()`](#c.get_completed_synchronize_rcu_full "get_completed_synchronize_rcu_full"). Returns **true** if the two
values are identical and **false** otherwise. This allows structures
whose lifetimes are tracked by old-state values to push these values
to a list header, allowing those structures to be slightly smaller.

Note that equality is judged on a bitwise basis, so that an
**rcu\_gp\_oldstate** structure with an already-completed state in one field
will compare not-equal to a structure with an already-completed state
in the other field. After all, the **rcu\_gp\_oldstate** structure is opaque
so how did such a situation come to pass in the first place?
