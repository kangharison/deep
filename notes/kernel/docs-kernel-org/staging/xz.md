# XZ data compression in Linux

> 출처(원문): https://docs.kernel.org/staging/xz.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# XZ data compression in Linux

## Introduction

XZ is a general purpose data compression format with high compression
ratio. The XZ decompressor in Linux is called XZ Embedded. It supports
the LZMA2 filter and optionally also Branch/Call/Jump (BCJ) filters
for executable code. CRC32 is supported for integrity checking.

See the [XZ Embedded](https://tukaani.org/xz/embedded.html) home page for the latest version which includes
a few optional extra features that aren’t required in the Linux kernel
and information about using the code outside the Linux kernel.

For userspace, [XZ Utils](https://tukaani.org/xz/) provide a zlib-like compression library
and a gzip-like command line tool.

## XZ related components in the kernel

The xz\_dec module provides XZ decompressor with single-call (buffer
to buffer) and multi-call (stateful) APIs in include/linux/xz.h.

For decompressing the kernel image, initramfs, and initrd, there
is a wrapper function in lib/decompress\_unxz.c. Its API is the
same as in other decompress\_\*.c files, which is defined in
include/linux/decompress/generic.h.

For kernel makefiles, three commands are provided for use with
`$(call if_changed)`. They require the xz tool from XZ Utils.

* `$(call if_changed,xzkern)` is for compressing the kernel image.
  It runs the script scripts/xz\_wrap.sh which uses arch-optimized
  options and a big LZMA2 dictionary.
* `$(call if_changed,xzkern_with_size)` is like `xzkern` above but
  this also appends a four-byte trailer containing the uncompressed size
  of the file. The trailer is needed by the boot code on some archs.
* Other things can be compressed with `$(call if_needed,xzmisc)`
  which will use no BCJ filter and 1 MiB LZMA2 dictionary.

## Notes on compression options

Since the XZ Embedded supports only streams with CRC32 or no integrity
check, make sure that you don’t use some other integrity check type
when encoding files that are supposed to be decoded by the kernel.
With liblzma from XZ Utils, you need to use either `LZMA_CHECK_CRC32`
or `LZMA_CHECK_NONE` when encoding. With the `xz` command line tool,
use `--check=crc32` or `--check=none` to override the default
`--check=crc64`.

Using CRC32 is strongly recommended unless there is some other layer
which will verify the integrity of the uncompressed data anyway.
Double checking the integrity would probably be waste of CPU cycles.
Note that the headers will always have a CRC32 which will be validated
by the decoder; you can only change the integrity check type (or
disable it) for the actual uncompressed data.

In userspace, LZMA2 is typically used with dictionary sizes of several
megabytes. The decoder needs to have the dictionary in RAM:

* In multi-call mode the dictionary is allocated as part of the
  decoder state. The reasonable maximum dictionary size for in-kernel
  use will depend on the target hardware: a few megabytes is fine for
  desktop systems while 64 KiB to 1 MiB might be more appropriate on
  some embedded systems.
* In single-call mode the output buffer is used as the dictionary
  buffer. That is, the size of the dictionary doesn’t affect the
  decompressor memory usage at all. Only the base data structures
  are allocated which take a little less than 30 KiB of memory.
  For the best compression, the dictionary should be at least
  as big as the uncompressed data. A notable example of single-call
  mode is decompressing the kernel itself (except on PowerPC).

The compression presets in XZ Utils may not be optimal when creating
files for the kernel, so don’t hesitate to use custom settings to,
for example, set the dictionary size. Also, xz may produce a smaller
file in single-threaded mode so setting that explicitly is recommended.
Example:

```
xz --threads=1 --check=crc32 --lzma2=dict=512KiB inputfile
```

## xz\_dec API

This is available with `#include <linux/xz.h>`.

enum xz\_mode
:   Operation mode

**Constants**

`XZ_SINGLE`
:   Single-call mode. This uses less RAM than
    multi-call modes, because the LZMA2
    dictionary doesn’t need to be allocated as
    part of the decoder state. All required data
    structures are allocated at initialization,
    so [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") cannot return XZ\_MEM\_ERROR.

`XZ_PREALLOC`
:   Multi-call mode with preallocated LZMA2
    dictionary buffer. All data structures are
    allocated at initialization, so [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run")
    cannot return XZ\_MEM\_ERROR.

`XZ_DYNALLOC`
:   Multi-call mode. The LZMA2 dictionary is
    allocated once the required size has been
    parsed from the stream headers. If the
    allocation fails, [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") will return
    XZ\_MEM\_ERROR.

**Description**

It is possible to enable support only for a subset of the above
modes at compile time by defining XZ\_DEC\_SINGLE, XZ\_DEC\_PREALLOC,
or XZ\_DEC\_DYNALLOC. The xz\_dec kernel module is always compiled
with support for all operation modes, but the preboot code may
be built with fewer features to minimize code size.

enum xz\_ret
:   Return codes

**Constants**

`XZ_OK`
:   Everything is OK so far. More input or more
    output space is required to continue. This
    return code is possible only in multi-call mode
    (XZ\_PREALLOC or XZ\_DYNALLOC).

`XZ_STREAM_END`
:   Operation finished successfully.

`XZ_UNSUPPORTED_CHECK`
:   Integrity check type is not supported. Decoding
    is still possible in multi-call mode by simply
    calling [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") again.
    Note that this return value is used only if
    XZ\_DEC\_ANY\_CHECK was defined at build time,
    which is not used in the kernel. Unsupported
    check types return XZ\_OPTIONS\_ERROR if
    XZ\_DEC\_ANY\_CHECK was not defined at build time.

`XZ_MEM_ERROR`
:   Allocating memory failed. This return code is
    possible only if the decoder was initialized
    with XZ\_DYNALLOC. The amount of memory that was
    tried to be allocated was no more than the
    dict\_max argument given to [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init").

`XZ_MEMLIMIT_ERROR`
:   A bigger LZMA2 dictionary would be needed than
    allowed by the dict\_max argument given to
    [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init"). This return value is possible
    only in multi-call mode (XZ\_PREALLOC or
    XZ\_DYNALLOC); the single-call mode (XZ\_SINGLE)
    ignores the dict\_max argument.

`XZ_FORMAT_ERROR`
:   File format was not recognized (wrong magic
    bytes).

`XZ_OPTIONS_ERROR`
:   This implementation doesn’t support the requested
    compression options. In the decoder this means
    that the header CRC32 matches, but the header
    itself specifies something that we don’t support.

`XZ_DATA_ERROR`
:   Compressed data is corrupt.

`XZ_BUF_ERROR`
:   Cannot make any progress. Details are slightly
    different between multi-call and single-call
    mode; more information below.

**Description**

In multi-call mode, XZ\_BUF\_ERROR is returned when two consecutive calls
to XZ code cannot consume any input and cannot produce any new output.
This happens when there is no new input available, or the output buffer
is full while at least one output byte is still pending. Assuming your
code is not buggy, you can get this error only when decoding a compressed
stream that is truncated or otherwise corrupt.

In single-call mode, XZ\_BUF\_ERROR is returned only when the output buffer
is too small or the compressed input is corrupt in a way that makes the
decoder produce more output than the caller expected. When it is
(relatively) clear that the compressed input is truncated, XZ\_DATA\_ERROR
is used instead of XZ\_BUF\_ERROR.

struct xz\_buf
:   Passing input and output buffers to XZ code

**Definition**:

```
struct xz_buf {
    const uint8_t *in;
    size_t in_pos;
    size_t in_size;
    uint8_t *out;
    size_t out_pos;
    size_t out_size;
};
```

**Members**

`in`
:   Beginning of the input buffer. This may be NULL if and only
    if in\_pos is equal to in\_size.

`in_pos`
:   Current position in the input buffer. This must not exceed
    in\_size.

`in_size`
:   Size of the input buffer

`out`
:   Beginning of the output buffer. This may be NULL if and only
    if out\_pos is equal to out\_size.

`out_pos`
:   Current position in the output buffer. This must not exceed
    out\_size.

`out_size`
:   Size of the output buffer

**Description**

Only the contents of the output buffer from out[out\_pos] onward, and
the variables in\_pos and out\_pos are modified by the XZ code.

struct xz\_dec \*xz\_dec\_init(enum [xz\_mode](#c.xz_mode "xz_mode") mode, uint32\_t dict\_max)
:   Allocate and initialize a XZ decoder state

**Parameters**

`enum xz_mode mode`
:   Operation mode

`uint32_t dict_max`
:   Maximum size of the LZMA2 dictionary (history buffer) for
    multi-call decoding. This is ignored in single-call mode
    (mode == XZ\_SINGLE). LZMA2 dictionary is always 2^n bytes
    or 2^n + 2^(n-1) bytes (the latter sizes are less common
    in practice), so other values for dict\_max don’t make sense.
    In the kernel, dictionary sizes of 64 KiB, 128 KiB, 256 KiB,
    512 KiB, and 1 MiB are probably the only reasonable values,
    except for kernel and initramfs images where a bigger
    dictionary can be fine and useful.

**Description**

Single-call mode (XZ\_SINGLE): [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") decodes the whole stream at
once. The caller must provide enough output space or the decoding will
fail. The output space is used as the dictionary buffer, which is why
there is no need to allocate the dictionary as part of the decoder’s
internal state.

Because the output buffer is used as the workspace, streams encoded using
a big dictionary are not a problem in single-call mode. It is enough that
the output buffer is big enough to hold the actual uncompressed data; it
can be smaller than the dictionary size stored in the stream headers.

Multi-call mode with preallocated dictionary (XZ\_PREALLOC): dict\_max bytes
of memory is preallocated for the LZMA2 dictionary. This way there is no
risk that [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") could run out of memory, since [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") will
never allocate any memory. Instead, if the preallocated dictionary is too
small for decoding the given input stream, [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") will return
XZ\_MEMLIMIT\_ERROR. Thus, it is important to know what kind of data will be
decoded to avoid allocating excessive amount of memory for the dictionary.

Multi-call mode with dynamically allocated dictionary (XZ\_DYNALLOC):
dict\_max specifies the maximum allowed dictionary size that [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run")
may allocate once it has parsed the dictionary size from the stream
headers. This way excessive allocations can be avoided while still
limiting the maximum memory usage to a sane value to prevent running the
system out of memory when decompressing streams from untrusted sources.

On success, [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init") returns a pointer to `struct xz_dec`, which is
ready to be used with [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run"). If memory allocation fails,
[`xz_dec_init()`](#c.xz_dec_init "xz_dec_init") returns NULL.

enum [xz\_ret](#c.xz_ret "xz_ret") xz\_dec\_run(struct xz\_dec \*s, struct [xz\_buf](#c.xz_buf "xz_buf") \*b)
:   Run the XZ decoder

**Parameters**

`struct xz_dec *s`
:   Decoder state allocated using [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init")

`struct xz_buf *b`
:   Input and output buffers

**Description**

The possible return values depend on build options and operation mode.
See [`enum xz_ret`](#c.xz_ret "xz_ret") for details.

Note that if an error occurs in single-call mode (return value is not
XZ\_STREAM\_END), b->in\_pos and b->out\_pos are not modified and the
contents of the output buffer from b->out[b->out\_pos] onward are
undefined. This is true even after XZ\_BUF\_ERROR, because with some filter
chains, there may be a second pass over the output buffer, and this pass
cannot be properly done if the output buffer is truncated. Thus, you
cannot give the single-call decoder a too small buffer and then expect to
get that amount valid data from the beginning of the stream. You must use
the multi-call decoder if you don’t want to uncompress the whole stream.

void xz\_dec\_reset(struct xz\_dec \*s)
:   Reset an already allocated decoder state

**Parameters**

`struct xz_dec *s`
:   Decoder state allocated using [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init")

**Description**

This function can be used to reset the multi-call decoder state without
freeing and reallocating memory with [`xz_dec_end()`](#c.xz_dec_end "xz_dec_end") and [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init").

In single-call mode, [`xz_dec_reset()`](#c.xz_dec_reset "xz_dec_reset") is always called in the beginning of
[`xz_dec_run()`](#c.xz_dec_run "xz_dec_run"). Thus, explicit call to [`xz_dec_reset()`](#c.xz_dec_reset "xz_dec_reset") is useful only in
multi-call mode.

void xz\_dec\_end(struct xz\_dec \*s)
:   Free the memory allocated for the decoder state

**Parameters**

`struct xz_dec *s`
:   Decoder state allocated using [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init"). If s is NULL,
    this function does nothing.

**MicroLZMA decompressor**

This MicroLZMA header format was created for use in EROFS but may be used
by others too. **In most cases one needs the XZ APIs above instead.**

The compressed format supported by this decoder is a raw LZMA stream
whose first byte (always 0x00) has been replaced with bitwise-negation
of the LZMA properties (lc/lp/pb) byte. For example, if lc/lp/pb is
3/0/2, the first byte is 0xA2. This way the first byte can never be 0x00.
Just like with LZMA2, lc + lp <= 4 must be true. The LZMA end-of-stream
marker must not be used. The unused values are reserved for future use.

struct xz\_dec\_microlzma \*xz\_dec\_microlzma\_alloc(enum [xz\_mode](#c.xz_mode "xz_mode") mode, uint32\_t dict\_size)
:   Allocate memory for the MicroLZMA decoder

**Parameters**

`enum xz_mode mode`
:   XZ\_SINGLE or XZ\_PREALLOC

`uint32_t dict_size`
:   LZMA dictionary size. This must be at least 4 KiB and
    at most 3 GiB.

**Description**

In contrast to [`xz_dec_init()`](#c.xz_dec_init "xz_dec_init"), this function only allocates the memory
and remembers the dictionary size. [`xz_dec_microlzma_reset()`](#c.xz_dec_microlzma_reset "xz_dec_microlzma_reset") must be used
before calling [`xz_dec_microlzma_run()`](#c.xz_dec_microlzma_run "xz_dec_microlzma_run").

The amount of allocated memory is a little less than 30 KiB with XZ\_SINGLE.
With XZ\_PREALLOC also a dictionary buffer of dict\_size bytes is allocated.

On success, [`xz_dec_microlzma_alloc()`](#c.xz_dec_microlzma_alloc "xz_dec_microlzma_alloc") returns a pointer to
`struct xz_dec_microlzma`. If memory allocation fails or
dict\_size is invalid, NULL is returned.

void xz\_dec\_microlzma\_reset(struct xz\_dec\_microlzma \*s, uint32\_t comp\_size, uint32\_t uncomp\_size, int uncomp\_size\_is\_exact)
:   Reset the MicroLZMA decoder state

**Parameters**

`struct xz_dec_microlzma *s`
:   Decoder state allocated using [`xz_dec_microlzma_alloc()`](#c.xz_dec_microlzma_alloc "xz_dec_microlzma_alloc")

`uint32_t comp_size`
:   Compressed size of the input stream

`uint32_t uncomp_size`
:   Uncompressed size of the input stream. A value smaller
    than the real uncompressed size of the input stream can
    be specified if uncomp\_size\_is\_exact is set to false.
    uncomp\_size can never be set to a value larger than the
    expected real uncompressed size because it would eventually
    result in XZ\_DATA\_ERROR.

`int uncomp_size_is_exact`
:   This is an int instead of bool to avoid
    requiring stdbool.h. This should normally be set to true.
    When this is set to false, error detection is weaker.

enum [xz\_ret](#c.xz_ret "xz_ret") xz\_dec\_microlzma\_run(struct xz\_dec\_microlzma \*s, struct [xz\_buf](#c.xz_buf "xz_buf") \*b)
:   Run the MicroLZMA decoder

**Parameters**

`struct xz_dec_microlzma *s`
:   Decoder state initialized using [`xz_dec_microlzma_reset()`](#c.xz_dec_microlzma_reset "xz_dec_microlzma_reset")

`struct xz_buf *b`
:   Input and output buffers

**Description**

This works similarly to [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run") with a few important differences.
Only the differences are documented here.

The only possible return values are XZ\_OK, XZ\_STREAM\_END, and
XZ\_DATA\_ERROR. This function cannot return XZ\_BUF\_ERROR: if no progress
is possible due to lack of input data or output space, this function will
keep returning XZ\_OK. Thus, the calling code must be written so that it
will eventually provide input and output space matching (or exceeding)
comp\_size and uncomp\_size arguments given to [`xz_dec_microlzma_reset()`](#c.xz_dec_microlzma_reset "xz_dec_microlzma_reset").
If the caller cannot do this (for example, if the input file is truncated
or otherwise corrupt), the caller must detect this error by itself to
avoid an infinite loop.

If the compressed data seems to be corrupt, XZ\_DATA\_ERROR is returned.
This can happen also when incorrect dictionary, uncompressed, or
compressed sizes have been specified.

With XZ\_PREALLOC only: As an extra feature, b->out may be NULL to skip over
uncompressed data. This way the caller doesn’t need to provide a temporary
output buffer for the bytes that will be ignored.

With XZ\_SINGLE only: In contrast to [`xz_dec_run()`](#c.xz_dec_run "xz_dec_run"), the return value XZ\_OK
is also possible and thus XZ\_SINGLE is actually a limited multi-call mode.
After XZ\_OK the bytes decoded so far may be read from the output buffer.
It is possible to continue decoding but the variables b->out and b->out\_pos
MUST NOT be changed by the caller. Increasing the value of b->out\_size is
allowed to make more output space available; one doesn’t need to provide
space for the whole uncompressed data on the first call. The input buffer
may be changed normally like with XZ\_PREALLOC. This way input data can be
provided from non-contiguous memory.

void xz\_dec\_microlzma\_end(struct xz\_dec\_microlzma \*s)
:   Free the memory allocated for the decoder state

**Parameters**

`struct xz_dec_microlzma *s`
:   Decoder state allocated using [`xz_dec_microlzma_alloc()`](#c.xz_dec_microlzma_alloc "xz_dec_microlzma_alloc").
    If s is NULL, this function does nothing.
