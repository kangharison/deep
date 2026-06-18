# Generic parser

> 출처(원문): https://docs.kernel.org/core-api/parser.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Generic parser

## Overview

The generic parser is a simple parser for parsing mount options,
filesystem options, driver options, subsystem options, etc.

## Parser API

int match\_token(char \*s, const match\_table\_t table, substring\_t args[])
:   Find a token (and optional args) in a string

**Parameters**

`char *s`
:   the string to examine for token/argument pairs

`const match_table_t table`
:   match\_table\_t describing the set of allowed option tokens and the
    arguments that may be associated with them. Must be terminated with a
    [`struct match_token`](#c.match_token "match_token") whose pattern is set to the NULL pointer.

`substring_t args[]`
:   array of `MAX_OPT_ARGS` `substring_t` elements. Used to return match
    locations.

**Description**

Detects which if any of a set of token strings has been passed
to it. Tokens can include up to `MAX_OPT_ARGS` instances of basic c-style
format identifiers which will be taken into account when matching the
tokens, and whose locations will be returned in the **args** array.

int match\_int(substring\_t \*s, int \*result)
:   scan a decimal representation of an integer from a substring\_t

**Parameters**

`substring_t *s`
:   substring\_t to be scanned

`int *result`
:   resulting integer on success

**Description**

Attempts to parse the `substring_t` **s** as a decimal integer.

**Return**

On success, sets **result** to the integer represented by the string
and returns 0. Returns -EINVAL or -ERANGE on failure.

int match\_uint(substring\_t \*s, unsigned int \*result)
:   scan a decimal representation of an integer from a substring\_t

**Parameters**

`substring_t *s`
:   substring\_t to be scanned

`unsigned int *result`
:   resulting integer on success

**Description**

Attempts to parse the `substring_t` **s** as a decimal integer.

**Return**

On success, sets **result** to the integer represented by the string
and returns 0. Returns -EINVAL or -ERANGE on failure.

int match\_u64(substring\_t \*s, u64 \*result)
:   scan a decimal representation of a u64 from a substring\_t

**Parameters**

`substring_t *s`
:   substring\_t to be scanned

`u64 *result`
:   resulting unsigned long long on success

**Description**

Attempts to parse the `substring_t` **s** as a long decimal
integer.

**Return**

On success, sets **result** to the integer represented by the string
and returns 0. Returns -EINVAL or -ERANGE on failure.

int match\_octal(substring\_t \*s, int \*result)
:   scan an octal representation of an integer from a substring\_t

**Parameters**

`substring_t *s`
:   substring\_t to be scanned

`int *result`
:   resulting integer on success

**Description**

Attempts to parse the `substring_t` **s** as an octal integer.

**Return**

On success, sets **result** to the integer represented by the string
and returns 0. Returns -EINVAL or -ERANGE on failure.

int match\_hex(substring\_t \*s, int \*result)
:   scan a hex representation of an integer from a substring\_t

**Parameters**

`substring_t *s`
:   substring\_t to be scanned

`int *result`
:   resulting integer on success

**Description**

Attempts to parse the `substring_t` **s** as a hexadecimal integer.

**Return**

On success, sets **result** to the integer represented by the string
and returns 0. Returns -EINVAL or -ERANGE on failure.

bool match\_wildcard(const char \*pattern, const char \*str)
:   parse if a string matches given wildcard pattern

**Parameters**

`const char *pattern`
:   wildcard pattern

`const char *str`
:   the string to be parsed

**Description**

Parse the string **str** to check if matches wildcard
pattern **pattern**. The pattern may contain two types of wildcards:

* ‘\*’ - matches zero or more characters
* ‘?’ - matches one character

**Return**

If the **str** matches the **pattern**, return true, else return false.

size\_t match\_strlcpy(char \*dest, const substring\_t \*src, size\_t size)
:   Copy the characters from a substring\_t to a sized buffer

**Parameters**

`char *dest`
:   where to copy to

`const substring_t *src`
:   `substring_t` to copy

`size_t size`
:   size of destination buffer

**Description**

Copy the characters in `substring_t` **src** to the
c-style string **dest**. Copy no more than **size** - 1 characters, plus
the terminating NUL.

**Return**

length of **src**.

char \*match\_strdup(const substring\_t \*s)
:   allocate a new string with the contents of a substring\_t

**Parameters**

`const substring_t *s`
:   `substring_t` to copy

**Description**

Allocates and returns a string filled with the contents of
the `substring_t` **s**. The caller is responsible for freeing the returned
string with [`kfree()`](mm-api.html#c.kfree "kfree").

**Return**

the address of the newly allocated NUL-terminated string or
`NULL` on error.
