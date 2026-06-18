# SHA-3 Algorithm Collection

> 출처(원문): https://docs.kernel.org/crypto/sha3.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [SHA-3 Algorithm Collection](#id3)

## [Overview](#id4)

The SHA-3 family of algorithms, as specified in NIST FIPS-202 [[1]](#id2), contains six
algorithms based on the Keccak sponge function. The differences between them
are: the “rate” (how much of the state buffer gets updated with new data between
invocations of the Keccak function and analogous to the “block size”), what
domain separation suffix gets appended to the input data, and how much output
data is extracted at the end. The Keccak sponge function is designed such that
arbitrary amounts of output can be obtained for certain algorithms.

Four digest algorithms are provided:

> * SHA3-224
> * SHA3-256
> * SHA3-384
> * SHA3-512

Additionally, two Extendable-Output Functions (XOFs) are provided:

> * SHAKE128
> * SHAKE256

The SHA-3 library API supports all six of these algorithms. The four digest
algorithms are also supported by the crypto\_shash and crypto\_ahash APIs.

This document describes the SHA-3 library API.

## [Digests](#id5)

The following functions compute SHA-3 digests:

```
void sha3_224(const u8 *in, size_t in_len, u8 out[SHA3_224_DIGEST_SIZE]);
void sha3_256(const u8 *in, size_t in_len, u8 out[SHA3_256_DIGEST_SIZE]);
void sha3_384(const u8 *in, size_t in_len, u8 out[SHA3_384_DIGEST_SIZE]);
void sha3_512(const u8 *in, size_t in_len, u8 out[SHA3_512_DIGEST_SIZE]);
```

For users that need to pass in data incrementally, an incremental API is also
provided. The incremental API uses the following struct:

```
struct sha3_ctx { ... };
```

Initialization is done with one of:

```
void sha3_224_init(struct sha3_ctx *ctx);
void sha3_256_init(struct sha3_ctx *ctx);
void sha3_384_init(struct sha3_ctx *ctx);
void sha3_512_init(struct sha3_ctx *ctx);
```

Input data is then added with any number of calls to:

```
void sha3_update(struct sha3_ctx *ctx, const u8 *in, size_t in_len);
```

Finally, the digest is generated using:

```
void sha3_final(struct sha3_ctx *ctx, u8 *out);
```

which also zeroizes the context. The length of the digest is determined by the
initialization function that was called.

## [Extendable-Output Functions](#id6)

The following functions compute the SHA-3 extendable-output functions (XOFs):

```
void shake128(const u8 *in, size_t in_len, u8 *out, size_t out_len);
void shake256(const u8 *in, size_t in_len, u8 *out, size_t out_len);
```

For users that need to provide the input data incrementally and/or receive the
output data incrementally, an incremental API is also provided. The incremental
API uses the following struct:

```
struct shake_ctx { ... };
```

Initialization is done with one of:

```
void shake128_init(struct shake_ctx *ctx);
void shake256_init(struct shake_ctx *ctx);
```

Input data is then added with any number of calls to:

```
void shake_update(struct shake_ctx *ctx, const u8 *in, size_t in_len);
```

Finally, the output data is extracted with any number of calls to:

```
void shake_squeeze(struct shake_ctx *ctx, u8 *out, size_t out_len);
```

and telling it how much data should be extracted. Note that performing multiple
squeezes, with the output laid consecutively in a buffer, gets exactly the same
output as doing a single squeeze for the combined amount over the same buffer.

More input data cannot be added after squeezing has started.

Once all the desired output has been extracted, zeroize the context:

```
void shake_zeroize_ctx(struct shake_ctx *ctx);
```

## [Testing](#id7)

To test the SHA-3 code, use sha3\_kunit (CONFIG\_CRYPTO\_LIB\_SHA3\_KUNIT\_TEST).

Since the SHA-3 algorithms are FIPS-approved, when the kernel is booted in FIPS
mode the SHA-3 library also performs a simple self-test. This is purely to meet
a FIPS requirement. Normal testing done by kernel developers and integrators
should use the much more comprehensive KUnit test suite instead.

## [References](#id8)

[[1](#id1)]

<https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf>

## [API Function Reference](#id9)

struct sha3\_ctx
:   Context for SHA3-224, SHA3-256, SHA3-384, or SHA3-512

**Definition**:

```
struct sha3_ctx {
    struct __sha3_ctx ctx;
};
```

**Members**

`ctx`
:   private

void sha3\_zeroize\_ctx(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx)
:   Zeroize a SHA-3 context

**Parameters**

`struct sha3_ctx *ctx`
:   The context to zeroize

**Description**

This is already called by [`sha3_final()`](#c.sha3_final "sha3_final"). Call this explicitly when abandoning
a context without calling [`sha3_final()`](#c.sha3_final "sha3_final").

struct shake\_ctx
:   Context for SHAKE128 or SHAKE256

**Definition**:

```
struct shake_ctx {
    struct __sha3_ctx ctx;
};
```

**Members**

`ctx`
:   private

void shake\_zeroize\_ctx(struct [shake\_ctx](#c.shake_ctx "shake_ctx") \*ctx)
:   Zeroize a SHAKE context

**Parameters**

`struct shake_ctx *ctx`
:   The context to zeroize

**Description**

Call this after the last squeeze.

void sha3\_224\_init(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx)
:   Initialize a context for SHA3-224

**Parameters**

`struct sha3_ctx *ctx`
:   The context to initialize

**Description**

This begins a new SHA3-224 message digest computation.

**Context**

Any context.

void sha3\_256\_init(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx)
:   Initialize a context for SHA3-256

**Parameters**

`struct sha3_ctx *ctx`
:   The context to initialize

**Description**

This begins a new SHA3-256 message digest computation.

**Context**

Any context.

void sha3\_384\_init(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx)
:   Initialize a context for SHA3-384

**Parameters**

`struct sha3_ctx *ctx`
:   The context to initialize

**Description**

This begins a new SHA3-384 message digest computation.

**Context**

Any context.

void sha3\_512\_init(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx)
:   Initialize a context for SHA3-512

**Parameters**

`struct sha3_ctx *ctx`
:   The context to initialize

**Description**

This begins a new SHA3-512 message digest computation.

**Context**

Any context.

void sha3\_update(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx, const u8 \*in, size\_t in\_len)
:   Update a SHA-3 digest context with input data

**Parameters**

`struct sha3_ctx *ctx`
:   The context to update; must have been initialized

`const u8 *in`
:   The input data

`size_t in_len`
:   Length of the input data in bytes

**Description**

This can be called any number of times to add data to a SHA3-224, SHA3-256,
SHA3-384, or SHA3-512 digest (depending on which init function was called).

**Context**

Any context.

void sha3\_final(struct [sha3\_ctx](#c.sha3_ctx "sha3_ctx") \*ctx, u8 \*out)
:   Finish computing a SHA-3 message digest

**Parameters**

`struct sha3_ctx *ctx`
:   The context to finalize; must have been initialized

`u8 *out`
:   (output) The resulting SHA3-224, SHA3-256, SHA3-384, or SHA3-512
    message digest, matching the init function that was called. Note that
    the size differs for each one; see SHA3\_\*\_DIGEST\_SIZE.

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void shake128\_init(struct [shake\_ctx](#c.shake_ctx "shake_ctx") \*ctx)
:   Initialize a context for SHAKE128

**Parameters**

`struct shake_ctx *ctx`
:   The context to initialize

**Description**

This begins a new SHAKE128 extendable-output function (XOF) computation.

**Context**

Any context.

void shake256\_init(struct [shake\_ctx](#c.shake_ctx "shake_ctx") \*ctx)
:   Initialize a context for SHAKE256

**Parameters**

`struct shake_ctx *ctx`
:   The context to initialize

**Description**

This begins a new SHAKE256 extendable-output function (XOF) computation.

**Context**

Any context.

void shake\_update(struct [shake\_ctx](#c.shake_ctx "shake_ctx") \*ctx, const u8 \*in, size\_t in\_len)
:   Update a SHAKE context with input data

**Parameters**

`struct shake_ctx *ctx`
:   The context to update; must have been initialized

`const u8 *in`
:   The input data

`size_t in_len`
:   Length of the input data in bytes

**Description**

This can be called any number of times to add more input data to SHAKE128 or
SHAKE256. This cannot be called after squeezing has begun.

**Context**

Any context.

void shake\_squeeze(struct [shake\_ctx](#c.shake_ctx "shake_ctx") \*ctx, u8 \*out, size\_t out\_len)
:   Generate output from SHAKE128 or SHAKE256

**Parameters**

`struct shake_ctx *ctx`
:   The context to squeeze; must have been initialized

`u8 *out`
:   Where to write the resulting output data

`size_t out_len`
:   The amount of data to extract to **out** in bytes

**Description**

This may be called multiple times. A number of consecutive squeezes laid
end-to-end will yield the same output as one big squeeze generating the same
total amount of output. More input cannot be provided after squeezing has
begun. After the last squeeze, call [`shake_zeroize_ctx()`](#c.shake_zeroize_ctx "shake_zeroize_ctx").

**Context**

Any context.

void sha3\_224(const u8 \*in, size\_t in\_len, u8 out[SHA3\_224\_DIGEST\_SIZE])
:   Compute SHA3-224 digest in one shot

**Parameters**

`const u8 *in`
:   The input data to be digested

`size_t in_len`
:   Length of the input data in bytes

`u8 out[SHA3_224_DIGEST_SIZE]`
:   The buffer into which the digest will be stored

**Description**

Convenience function that computes a SHA3-224 digest. Use this instead of
the incremental API if you’re able to provide all the input at once.

**Context**

Any context.

void sha3\_256(const u8 \*in, size\_t in\_len, u8 out[SHA3\_256\_DIGEST\_SIZE])
:   Compute SHA3-256 digest in one shot

**Parameters**

`const u8 *in`
:   The input data to be digested

`size_t in_len`
:   Length of the input data in bytes

`u8 out[SHA3_256_DIGEST_SIZE]`
:   The buffer into which the digest will be stored

**Description**

Convenience function that computes a SHA3-256 digest. Use this instead of
the incremental API if you’re able to provide all the input at once.

**Context**

Any context.

void sha3\_384(const u8 \*in, size\_t in\_len, u8 out[SHA3\_384\_DIGEST\_SIZE])
:   Compute SHA3-384 digest in one shot

**Parameters**

`const u8 *in`
:   The input data to be digested

`size_t in_len`
:   Length of the input data in bytes

`u8 out[SHA3_384_DIGEST_SIZE]`
:   The buffer into which the digest will be stored

**Description**

Convenience function that computes a SHA3-384 digest. Use this instead of
the incremental API if you’re able to provide all the input at once.

**Context**

Any context.

void sha3\_512(const u8 \*in, size\_t in\_len, u8 out[SHA3\_512\_DIGEST\_SIZE])
:   Compute SHA3-512 digest in one shot

**Parameters**

`const u8 *in`
:   The input data to be digested

`size_t in_len`
:   Length of the input data in bytes

`u8 out[SHA3_512_DIGEST_SIZE]`
:   The buffer into which the digest will be stored

**Description**

Convenience function that computes a SHA3-512 digest. Use this instead of
the incremental API if you’re able to provide all the input at once.

**Context**

Any context.

void shake128(const u8 \*in, size\_t in\_len, u8 \*out, size\_t out\_len)
:   Compute SHAKE128 in one shot

**Parameters**

`const u8 *in`
:   The input data to be used

`size_t in_len`
:   Length of the input data in bytes

`u8 *out`
:   The buffer into which the output will be stored

`size_t out_len`
:   Length of the output to produce in bytes

**Description**

Convenience function that computes SHAKE128 in one shot. Use this instead of
the incremental API if you’re able to provide all the input at once as well
as receive all the output at once. All output lengths are supported.

**Context**

Any context.

void shake256(const u8 \*in, size\_t in\_len, u8 \*out, size\_t out\_len)
:   Compute SHAKE256 in one shot

**Parameters**

`const u8 *in`
:   The input data to be used

`size_t in_len`
:   Length of the input data in bytes

`u8 *out`
:   The buffer into which the output will be stored

`size_t out_len`
:   Length of the output to produce in bytes

**Description**

Convenience function that computes SHAKE256 in one shot. Use this instead of
the incremental API if you’re able to provide all the input at once as well
as receive all the output at once. All output lengths are supported.

**Context**

Any context.
