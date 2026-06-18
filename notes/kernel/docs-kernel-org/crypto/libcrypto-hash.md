# Hash functions, MACs, and XOFs

> 출처(원문): https://docs.kernel.org/crypto/libcrypto-hash.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Hash functions, MACs, and XOFs

## AES-CMAC and AES-XCBC-MAC

Support for the AES-CMAC and AES-XCBC-MAC message authentication codes.

struct aes\_cmac\_key
:   Prepared key for AES-CMAC or AES-XCBC-MAC

**Definition**:

```
struct aes_cmac_key {
    struct aes_enckey aes;
    union {
        u8 b[AES_BLOCK_SIZE];
        __be64 w[2];
    } k_final[2];
};
```

**Members**

`aes`
:   The AES key for cipher block chaining

`k_final`
:   Finalization subkeys for the final block.
    k\_final[0] (CMAC K1, XCBC-MAC K2) is used if it’s a full block.
    k\_final[1] (CMAC K2, XCBC-MAC K3) is used if it’s a partial block.

struct aes\_cmac\_ctx
:   Context for computing an AES-CMAC or AES-XCBC-MAC value

**Definition**:

```
struct aes_cmac_ctx {
    const struct aes_cmac_key *key;
    size_t partial_len;
    u8 h[AES_BLOCK_SIZE];
};
```

**Members**

`key`
:   Pointer to the key struct. A pointer is used rather than a copy of the
    struct, since the key `struct size` may be large. It is assumed that the
    key lives at least as long as the context.

`partial_len`
:   Number of bytes that have been XOR’ed into **h** since the last
    AES encryption. This is 0 if no data has been processed yet,
    or between 1 and AES\_BLOCK\_SIZE inclusive otherwise.

`h`
:   The current chaining value

int aes\_cmac\_preparekey(struct [aes\_cmac\_key](#c.aes_cmac_key "aes_cmac_key") \*key, const u8 \*in\_key, size\_t key\_len)
:   Prepare a key for AES-CMAC

**Parameters**

`struct aes_cmac_key *key`
:   (output) The key `struct to` initialize

`const u8 *in_key`
:   The raw AES key

`size_t key_len`
:   Length of the raw key in bytes. The supported values are
    AES\_KEYSIZE\_128, AES\_KEYSIZE\_192, and AES\_KEYSIZE\_256.

**Context**

Any context.

**Return**

0 on success or -EINVAL if the given key length is invalid. No other
errors are possible, so callers that always pass a valid key length
don’t need to check for errors.

void aes\_xcbcmac\_preparekey(struct [aes\_cmac\_key](#c.aes_cmac_key "aes_cmac_key") \*key, const u8 in\_key[static AES\_KEYSIZE\_128])
:   Prepare a key for AES-XCBC-MAC

**Parameters**

`struct aes_cmac_key *key`
:   (output) The key `struct to` initialize

`const u8 in_key[static AES_KEYSIZE_128]`
:   The raw key. As per the AES-XCBC-MAC specification (RFC 3566), this
    is 128 bits, matching the internal use of AES-128.

**Description**

AES-XCBC-MAC and AES-CMAC are the same except for the key preparation. After
that step, AES-XCBC-MAC is supported via the aes\_cmac\_\* functions.

New users should use AES-CMAC instead of AES-XCBC-MAC.

**Context**

Any context.

void aes\_cmac\_init(struct [aes\_cmac\_ctx](#c.aes_cmac_ctx "aes_cmac_ctx") \*ctx, const struct [aes\_cmac\_key](#c.aes_cmac_key "aes_cmac_key") \*key)
:   Start computing an AES-CMAC or AES-XCBC-MAC value

**Parameters**

`struct aes_cmac_ctx *ctx`
:   (output) The context to initialize

`const struct aes_cmac_key *key`
:   The key to use. Note that a pointer to the key is saved in the
    context, so the key must live at least as long as the context.

**Description**

This supports both AES-CMAC and AES-XCBC-MAC. Which one is done depends on
whether [`aes_cmac_preparekey()`](#c.aes_cmac_preparekey "aes_cmac_preparekey") or [`aes_xcbcmac_preparekey()`](#c.aes_xcbcmac_preparekey "aes_xcbcmac_preparekey") was called.

void aes\_cmac\_update(struct [aes\_cmac\_ctx](#c.aes_cmac_ctx "aes_cmac_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an AES-CMAC or AES-XCBC-MAC context with more data

**Parameters**

`struct aes_cmac_ctx *ctx`
:   The context to update; must have been initialized

`const u8 *data`
:   The message data

`size_t data_len`
:   The data length in bytes. Doesn’t need to be block-aligned.

**Description**

This can be called any number of times.

**Context**

Any context.

void aes\_cmac\_final(struct [aes\_cmac\_ctx](#c.aes_cmac_ctx "aes_cmac_ctx") \*ctx, u8 out[static AES\_BLOCK\_SIZE])
:   Finish computing an AES-CMAC or AES-XCBC-MAC value

**Parameters**

`struct aes_cmac_ctx *ctx`
:   The context to finalize; must have been initialized

`u8 out[static AES_BLOCK_SIZE]`
:   (output) The resulting MAC

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void aes\_cmac(const struct [aes\_cmac\_key](#c.aes_cmac_key "aes_cmac_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static AES\_BLOCK\_SIZE])
:   Compute AES-CMAC or AES-XCBC-MAC in one shot

**Parameters**

`const struct aes_cmac_key *key`
:   The key to use

`const u8 *data`
:   The message data

`size_t data_len`
:   The data length in bytes

`u8 out[static AES_BLOCK_SIZE]`
:   (output) The resulting AES-CMAC or AES-XCBC-MAC value

**Description**

This supports both AES-CMAC and AES-XCBC-MAC. Which one is done depends on
whether [`aes_cmac_preparekey()`](#c.aes_cmac_preparekey "aes_cmac_preparekey") or [`aes_xcbcmac_preparekey()`](#c.aes_xcbcmac_preparekey "aes_xcbcmac_preparekey") was called.

**Context**

Any context.

## BLAKE2b

Support for the BLAKE2b cryptographic hash function.

struct blake2b\_ctx
:   Context for hashing a message with BLAKE2b

**Definition**:

```
struct blake2b_ctx {
    u64 h[8];
    u64 t[2];
    u64 f[2];
    u8 buf[BLAKE2B_BLOCK_SIZE];
    unsigned int buflen;
    unsigned int outlen;
};
```

**Members**

`h`
:   compression function state

`t`
:   block counter

`f`
:   finalization indicator

`buf`
:   partial block buffer; ‘buflen’ bytes are valid

`buflen`
:   number of bytes buffered in **buf**

`outlen`
:   length of output hash value in bytes, at most BLAKE2B\_HASH\_SIZE

void blake2b\_init(struct [blake2b\_ctx](#c.blake2b_ctx "blake2b_ctx") \*ctx, size\_t outlen)
:   Initialize a BLAKE2b context for a new message (unkeyed)

**Parameters**

`struct blake2b_ctx *ctx`
:   the context to initialize

`size_t outlen`
:   length of output hash value in bytes, at most BLAKE2B\_HASH\_SIZE

**Context**

Any context.

void blake2b\_init\_key(struct [blake2b\_ctx](#c.blake2b_ctx "blake2b_ctx") \*ctx, size\_t outlen, const void \*key, size\_t keylen)
:   Initialize a BLAKE2b context for a new message (keyed)

**Parameters**

`struct blake2b_ctx *ctx`
:   the context to initialize

`size_t outlen`
:   length of output hash value in bytes, at most BLAKE2B\_HASH\_SIZE

`const void *key`
:   the key

`size_t keylen`
:   the key length in bytes, at most BLAKE2B\_KEY\_SIZE

**Context**

Any context.

void blake2b\_update(struct [blake2b\_ctx](#c.blake2b_ctx "blake2b_ctx") \*ctx, const u8 \*in, size\_t inlen)
:   Update a BLAKE2b context with message data

**Parameters**

`struct blake2b_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *in`
:   the message data

`size_t inlen`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void blake2b\_final(struct [blake2b\_ctx](#c.blake2b_ctx "blake2b_ctx") \*ctx, u8 \*out)
:   Finish computing a BLAKE2b hash

**Parameters**

`struct blake2b_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 *out`
:   (output) the resulting BLAKE2b hash. Its length will be equal to the
    **outlen** that was passed to [`blake2b_init()`](#c.blake2b_init "blake2b_init") or [`blake2b_init_key()`](#c.blake2b_init_key "blake2b_init_key").

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void blake2b(const u8 \*key, size\_t keylen, const u8 \*in, size\_t inlen, u8 \*out, size\_t outlen)
:   Compute BLAKE2b hash in one shot

**Parameters**

`const u8 *key`
:   the key, or NULL for an unkeyed hash

`size_t keylen`
:   the key length in bytes (at most BLAKE2B\_KEY\_SIZE), or 0 for an
    unkeyed hash

`const u8 *in`
:   the message data

`size_t inlen`
:   the data length in bytes

`u8 *out`
:   (output) the resulting BLAKE2b hash, with length **outlen**

`size_t outlen`
:   length of output hash value in bytes, at most BLAKE2B\_HASH\_SIZE

**Context**

Any context.

## BLAKE2s

Support for the BLAKE2s cryptographic hash function.

struct blake2s\_ctx
:   Context for hashing a message with BLAKE2s

**Definition**:

```
struct blake2s_ctx {
    u32 h[8];
    u32 t[2];
    u32 f[2];
    u8 buf[BLAKE2S_BLOCK_SIZE];
    unsigned int buflen;
    unsigned int outlen;
};
```

**Members**

`h`
:   compression function state

`t`
:   block counter

`f`
:   finalization indicator

`buf`
:   partial block buffer; ‘buflen’ bytes are valid

`buflen`
:   number of bytes buffered in **buf**

`outlen`
:   length of output hash value in bytes, at most BLAKE2S\_HASH\_SIZE

void blake2s\_init(struct [blake2s\_ctx](#c.blake2s_ctx "blake2s_ctx") \*ctx, size\_t outlen)
:   Initialize a BLAKE2s context for a new message (unkeyed)

**Parameters**

`struct blake2s_ctx *ctx`
:   the context to initialize

`size_t outlen`
:   length of output hash value in bytes, at most BLAKE2S\_HASH\_SIZE

**Context**

Any context.

void blake2s\_init\_key(struct [blake2s\_ctx](#c.blake2s_ctx "blake2s_ctx") \*ctx, size\_t outlen, const void \*key, size\_t keylen)
:   Initialize a BLAKE2s context for a new message (keyed)

**Parameters**

`struct blake2s_ctx *ctx`
:   the context to initialize

`size_t outlen`
:   length of output hash value in bytes, at most BLAKE2S\_HASH\_SIZE

`const void *key`
:   the key

`size_t keylen`
:   the key length in bytes, at most BLAKE2S\_KEY\_SIZE

**Context**

Any context.

void blake2s\_update(struct [blake2s\_ctx](#c.blake2s_ctx "blake2s_ctx") \*ctx, const u8 \*in, size\_t inlen)
:   Update a BLAKE2s context with message data

**Parameters**

`struct blake2s_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *in`
:   the message data

`size_t inlen`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void blake2s\_final(struct [blake2s\_ctx](#c.blake2s_ctx "blake2s_ctx") \*ctx, u8 \*out)
:   Finish computing a BLAKE2s hash

**Parameters**

`struct blake2s_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 *out`
:   (output) the resulting BLAKE2s hash. Its length will be equal to the
    **outlen** that was passed to [`blake2s_init()`](#c.blake2s_init "blake2s_init") or [`blake2s_init_key()`](#c.blake2s_init_key "blake2s_init_key").

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void blake2s(const u8 \*key, size\_t keylen, const u8 \*in, size\_t inlen, u8 \*out, size\_t outlen)
:   Compute BLAKE2s hash in one shot

**Parameters**

`const u8 *key`
:   the key, or NULL for an unkeyed hash

`size_t keylen`
:   the key length in bytes (at most BLAKE2S\_KEY\_SIZE), or 0 for an
    unkeyed hash

`const u8 *in`
:   the message data

`size_t inlen`
:   the data length in bytes

`u8 *out`
:   (output) the resulting BLAKE2s hash, with length **outlen**

`size_t outlen`
:   length of output hash value in bytes, at most BLAKE2S\_HASH\_SIZE

**Context**

Any context.

## GHASH and POLYVAL

Support for the GHASH and POLYVAL universal hash functions. These algorithms
are used only as internal components of other algorithms.

struct polyval\_elem
:   An element of the POLYVAL finite field

**Definition**:

```
struct polyval_elem {
    union {
        u8 bytes[POLYVAL_BLOCK_SIZE];
        struct {
            __le64 lo;
            __le64 hi;
        };
    };
};
```

**Members**

`{unnamed_union}`
:   anonymous

`bytes`
:   View of the element as a byte array (unioned with **lo** and **hi**)

`{unnamed_struct}`
:   anonymous

`lo`
:   The low 64 terms of the element’s polynomial

`hi`
:   The high 64 terms of the element’s polynomial

**Description**

This represents an element of the finite field GF(2^128), using the POLYVAL
convention: little-endian byte order and natural bit order.

struct ghash\_key
:   Prepared key for GHASH

**Definition**:

```
struct ghash_key {
#if defined(CONFIG_CRYPTO_LIB_GF128HASH_ARCH) && defined(CONFIG_PPC64);
    u64 htable[4][2];
#elif defined(CONFIG_CRYPTO_LIB_GF128HASH_ARCH) &&         (defined(CONFIG_RISCV) || defined(CONFIG_S390));
    u8 h_raw[GHASH_BLOCK_SIZE];
#endif;
    struct polyval_elem h;
};
```

**Members**

`htable`
:   GHASH key format used by the POWER8 assembly code

`h_raw`
:   The hash key H, in GHASH format

`h`
:   The hash key H, in POLYVAL format

**Description**

Use [`ghash_preparekey()`](#c.ghash_preparekey "ghash_preparekey") to initialize this.

struct polyval\_key
:   Prepared key for POLYVAL

**Definition**:

```
struct polyval_key {
#if defined(CONFIG_CRYPTO_LIB_GF128HASH_ARCH) &&         (defined(CONFIG_ARM64) || defined(CONFIG_X86));
    struct polyval_elem h_powers[8];
#else;
    struct polyval_elem h;
#endif;
};
```

**Members**

`h_powers`
:   Powers of the hash key H^8 through H^1

`h`
:   The hash key H

**Description**

This may contain just the raw key H, or it may contain precomputed key
powers, depending on the platform’s POLYVAL implementation. Use
[`polyval_preparekey()`](#c.polyval_preparekey "polyval_preparekey") to initialize this.

By H^i we mean H^(i-1) \* H \* x^-128, with base case H^1 = H. I.e. the
exponentiation repeats the POLYVAL dot operation, with its “extra” x^-128.

struct ghash\_ctx
:   Context for computing a GHASH value

**Definition**:

```
struct ghash_ctx {
    const struct ghash_key *key;
    struct polyval_elem acc;
    size_t partial;
};
```

**Members**

`key`
:   Pointer to the prepared GHASH key. The user of the API is
    responsible for ensuring that the key lives as long as the context.

`acc`
:   The accumulator. It is stored in POLYVAL format rather than GHASH
    format, since most implementations want it in POLYVAL format.

`partial`
:   Number of data bytes processed so far modulo GHASH\_BLOCK\_SIZE

struct polyval\_ctx
:   Context for computing a POLYVAL value

**Definition**:

```
struct polyval_ctx {
    const struct polyval_key *key;
    struct polyval_elem acc;
    size_t partial;
};
```

**Members**

`key`
:   Pointer to the prepared POLYVAL key. The user of the API is
    responsible for ensuring that the key lives as long as the context.

`acc`
:   The accumulator

`partial`
:   Number of data bytes processed so far modulo POLYVAL\_BLOCK\_SIZE

void ghash\_preparekey(struct [ghash\_key](#c.ghash_key "ghash_key") \*key, const u8 raw\_key[GHASH\_BLOCK\_SIZE])
:   Prepare a GHASH key

**Parameters**

`struct ghash_key *key`
:   (output) The key structure to initialize

`const u8 raw_key[GHASH_BLOCK_SIZE]`
:   The raw hash key

**Description**

Initialize a GHASH key structure from a raw key.

**Context**

Any context.

void polyval\_preparekey(struct [polyval\_key](#c.polyval_key "polyval_key") \*key, const u8 raw\_key[POLYVAL\_BLOCK\_SIZE])
:   Prepare a POLYVAL key

**Parameters**

`struct polyval_key *key`
:   (output) The key structure to initialize

`const u8 raw_key[POLYVAL_BLOCK_SIZE]`
:   The raw hash key

**Description**

Initialize a POLYVAL key structure from a raw key. This may be a simple
copy, or it may involve precomputing powers of the key, depending on the
platform’s POLYVAL implementation.

**Context**

Any context.

void ghash\_init(struct [ghash\_ctx](#c.ghash_ctx "ghash_ctx") \*ctx, const struct [ghash\_key](#c.ghash_key "ghash_key") \*key)
:   Initialize a GHASH context for a new message

**Parameters**

`struct ghash_ctx *ctx`
:   The context to initialize

`const struct ghash_key *key`
:   The key to use. Note that a pointer to the key is saved in the
    context, so the key must live at least as long as the context.

void polyval\_init(struct [polyval\_ctx](#c.polyval_ctx "polyval_ctx") \*ctx, const struct [polyval\_key](#c.polyval_key "polyval_key") \*key)
:   Initialize a POLYVAL context for a new message

**Parameters**

`struct polyval_ctx *ctx`
:   The context to initialize

`const struct polyval_key *key`
:   The key to use. Note that a pointer to the key is saved in the
    context, so the key must live at least as long as the context.

void polyval\_import\_blkaligned(struct [polyval\_ctx](#c.polyval_ctx "polyval_ctx") \*ctx, const struct [polyval\_key](#c.polyval_key "polyval_key") \*key, const struct [polyval\_elem](#c.polyval_elem "polyval_elem") \*acc)
:   Import a POLYVAL accumulator value

**Parameters**

`struct polyval_ctx *ctx`
:   The context to initialize

`const struct polyval_key *key`
:   The key to import. Note that a pointer to the key is saved in the
    context, so the key must live at least as long as the context.

`const struct polyval_elem *acc`
:   The accumulator value to import.

**Description**

This imports an accumulator that was saved by [`polyval_export_blkaligned()`](#c.polyval_export_blkaligned "polyval_export_blkaligned").
The same key must be used.

void polyval\_export\_blkaligned(const struct [polyval\_ctx](#c.polyval_ctx "polyval_ctx") \*ctx, struct [polyval\_elem](#c.polyval_elem "polyval_elem") \*acc)
:   Export a POLYVAL accumulator value

**Parameters**

`const struct polyval_ctx *ctx`
:   The context to export the accumulator value from

`struct polyval_elem *acc`
:   (output) The exported accumulator value

**Description**

This exports the accumulator from a POLYVAL context. The number of data
bytes processed so far must be a multiple of POLYVAL\_BLOCK\_SIZE.

void ghash\_update(struct [ghash\_ctx](#c.ghash_ctx "ghash_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a GHASH context with message data

**Parameters**

`struct ghash_ctx *ctx`
:   The context to update; must have been initialized

`const u8 *data`
:   The message data

`size_t len`
:   The data length in bytes. Doesn’t need to be block-aligned.

**Description**

This can be called any number of times.

**Context**

Any context.

void polyval\_update(struct [polyval\_ctx](#c.polyval_ctx "polyval_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a POLYVAL context with message data

**Parameters**

`struct polyval_ctx *ctx`
:   The context to update; must have been initialized

`const u8 *data`
:   The message data

`size_t len`
:   The data length in bytes. Doesn’t need to be block-aligned.

**Description**

This can be called any number of times.

**Context**

Any context.

void ghash\_final(struct [ghash\_ctx](#c.ghash_ctx "ghash_ctx") \*ctx, u8 out[GHASH\_BLOCK\_SIZE])
:   Finish computing a GHASH value

**Parameters**

`struct ghash_ctx *ctx`
:   The context to finalize

`u8 out[GHASH_BLOCK_SIZE]`
:   The output value

**Description**

If the total data length isn’t a multiple of GHASH\_BLOCK\_SIZE, then the
final block is automatically zero-padded.

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void polyval\_final(struct [polyval\_ctx](#c.polyval_ctx "polyval_ctx") \*ctx, u8 out[POLYVAL\_BLOCK\_SIZE])
:   Finish computing a POLYVAL value

**Parameters**

`struct polyval_ctx *ctx`
:   The context to finalize

`u8 out[POLYVAL_BLOCK_SIZE]`
:   The output value

**Description**

If the total data length isn’t a multiple of POLYVAL\_BLOCK\_SIZE, then the
final block is automatically zero-padded.

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void ghash(const struct [ghash\_key](#c.ghash_key "ghash_key") \*key, const u8 \*data, size\_t len, u8 out[GHASH\_BLOCK\_SIZE])
:   Compute a GHASH value

**Parameters**

`const struct ghash_key *key`
:   The prepared key

`const u8 *data`
:   The message data

`size_t len`
:   The data length in bytes. Doesn’t need to be block-aligned.

`u8 out[GHASH_BLOCK_SIZE]`
:   The output value

**Context**

Any context.

void polyval(const struct [polyval\_key](#c.polyval_key "polyval_key") \*key, const u8 \*data, size\_t len, u8 out[POLYVAL\_BLOCK\_SIZE])
:   Compute a POLYVAL value

**Parameters**

`const struct polyval_key *key`
:   The prepared key

`const u8 *data`
:   The message data

`size_t len`
:   The data length in bytes. Doesn’t need to be block-aligned.

`u8 out[POLYVAL_BLOCK_SIZE]`
:   The output value

**Context**

Any context.

## MD5

Support for the MD5 cryptographic hash function and HMAC-MD5. This algorithm is
obsolete and is supported only for backwards compatibility.

struct md5\_ctx
:   Context for hashing a message with MD5

**Definition**:

```
struct md5_ctx {
    struct md5_block_state state;
    u64 bytecount;
    u8 buf[MD5_BLOCK_SIZE];
};
```

**Members**

`state`
:   the compression function state

`bytecount`
:   number of bytes processed so far

`buf`
:   partial block buffer; bytecount % MD5\_BLOCK\_SIZE bytes are valid

void md5\_init(struct [md5\_ctx](#c.md5_ctx "md5_ctx") \*ctx)
:   Initialize an MD5 context for a new message

**Parameters**

`struct md5_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`md5()`](#c.md5 "md5") instead.

**Context**

Any context.

void md5\_update(struct [md5\_ctx](#c.md5_ctx "md5_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update an MD5 context with message data

**Parameters**

`struct md5_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void md5\_final(struct [md5\_ctx](#c.md5_ctx "md5_ctx") \*ctx, u8 out[static MD5\_DIGEST\_SIZE])
:   Finish computing an MD5 message digest

**Parameters**

`struct md5_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static MD5_DIGEST_SIZE]`
:   (output) the resulting MD5 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void md5(const u8 \*data, size\_t len, u8 out[static MD5\_DIGEST\_SIZE])
:   Compute MD5 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static MD5_DIGEST_SIZE]`
:   (output) the resulting MD5 message digest

**Context**

Any context.

struct hmac\_md5\_key
:   Prepared key for HMAC-MD5

**Definition**:

```
struct hmac_md5_key {
    struct md5_block_state istate;
    struct md5_block_state ostate;
};
```

**Members**

`istate`
:   private

`ostate`
:   private

struct hmac\_md5\_ctx
:   Context for computing HMAC-MD5 of a message

**Definition**:

```
struct hmac_md5_ctx {
    struct md5_ctx hash_ctx;
    struct md5_block_state ostate;
};
```

**Members**

`hash_ctx`
:   private

`ostate`
:   private

void hmac\_md5\_preparekey(struct [hmac\_md5\_key](#c.hmac_md5_key "hmac_md5_key") \*key, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Prepare a key for HMAC-MD5

**Parameters**

`struct hmac_md5_key *key`
:   (output) the key structure to initialize

`const u8 *raw_key`
:   the raw HMAC-MD5 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Note**

the caller is responsible for zeroizing both the [`struct hmac_md5_key`](#c.hmac_md5_key "hmac_md5_key")
and the raw key once they are no longer needed.

**Context**

Any context.

void hmac\_md5\_init(struct [hmac\_md5\_ctx](#c.hmac_md5_ctx "hmac_md5_ctx") \*ctx, const struct [hmac\_md5\_key](#c.hmac_md5_key "hmac_md5_key") \*key)
:   Initialize an HMAC-MD5 context for a new message

**Parameters**

`struct hmac_md5_ctx *ctx`
:   (output) the HMAC context to initialize

`const struct hmac_md5_key *key`
:   the prepared HMAC key

**Description**

If you don’t need incremental computation, consider [`hmac_md5()`](#c.hmac_md5 "hmac_md5") instead.

**Context**

Any context.

void hmac\_md5\_init\_usingrawkey(struct [hmac\_md5\_ctx](#c.hmac_md5_ctx "hmac_md5_ctx") \*ctx, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Initialize an HMAC-MD5 context for a new message, using a raw key

**Parameters**

`struct hmac_md5_ctx *ctx`
:   (output) the HMAC context to initialize

`const u8 *raw_key`
:   the raw HMAC-MD5 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Description**

If you don’t need incremental computation, consider [`hmac_md5_usingrawkey()`](#c.hmac_md5_usingrawkey "hmac_md5_usingrawkey")
instead.

**Context**

Any context.

void hmac\_md5\_update(struct [hmac\_md5\_ctx](#c.hmac_md5_ctx "hmac_md5_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an HMAC-MD5 context with message data

**Parameters**

`struct hmac_md5_ctx *ctx`
:   the HMAC context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void hmac\_md5\_final(struct [hmac\_md5\_ctx](#c.hmac_md5_ctx "hmac_md5_ctx") \*ctx, u8 out[static MD5\_DIGEST\_SIZE])
:   Finish computing an HMAC-MD5 value

**Parameters**

`struct hmac_md5_ctx *ctx`
:   the HMAC context to finalize; must have been initialized

`u8 out[static MD5_DIGEST_SIZE]`
:   (output) the resulting HMAC-MD5 value

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void hmac\_md5(const struct [hmac\_md5\_key](#c.hmac_md5_key "hmac_md5_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static MD5\_DIGEST\_SIZE])
:   Compute HMAC-MD5 in one shot, using a prepared key

**Parameters**

`const struct hmac_md5_key *key`
:   the prepared HMAC key

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static MD5_DIGEST_SIZE]`
:   (output) the resulting HMAC-MD5 value

**Description**

If you’re using the key only once, consider using [`hmac_md5_usingrawkey()`](#c.hmac_md5_usingrawkey "hmac_md5_usingrawkey").

**Context**

Any context.

void hmac\_md5\_usingrawkey(const u8 \*raw\_key, size\_t raw\_key\_len, const u8 \*data, size\_t data\_len, u8 out[static MD5\_DIGEST\_SIZE])
:   Compute HMAC-MD5 in one shot, using a raw key

**Parameters**

`const u8 *raw_key`
:   the raw HMAC-MD5 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static MD5_DIGEST_SIZE]`
:   (output) the resulting HMAC-MD5 value

**Description**

If you’re using the key multiple times, prefer to use [`hmac_md5_preparekey()`](#c.hmac_md5_preparekey "hmac_md5_preparekey")
followed by multiple calls to [`hmac_md5()`](#c.hmac_md5 "hmac_md5") instead.

**Context**

Any context.

## NH

Support for the NH universal hash function. This algorithm is used only as an
internal component of other algorithms.

void nh(const u32 \*key, const u8 \*message, size\_t message\_len, \_\_le64 hash[NH\_NUM\_PASSES])
:   NH hash function for Adiantum

**Parameters**

`const u32 *key`
:   The key. **message\_len** + 48 bytes of it are used. This is NH\_KEY\_BYTES
    if **message\_len** has its maximum length of NH\_MESSAGE\_BYTES.

`const u8 *message`
:   The message

`size_t message_len`
:   The message length in bytes. Must be a multiple of 16
    (NH\_MESSAGE\_UNIT) and at most 1024 (NH\_MESSAGE\_BYTES).

`__le64 hash[NH_NUM_PASSES]`
:   (output) The resulting hash value

**Note**

the pseudocode for NH in the Adiantum paper iterates over 1024-byte
segments of the message, computes a 32-byte hash for each, and returns all
the hashes concatenated together. In contrast, this function just hashes one
segment and returns one hash. It’s the caller’s responsibility to call this
function for each 1024-byte segment and collect all the hashes.

**Context**

Any context.

## Poly1305

Support for the Poly1305 universal hash function. This algorithm is used only
as an internal component of other algorithms.

## SHA-1

Support for the SHA-1 cryptographic hash function and HMAC-SHA1. This algorithm
is obsolete and is supported only for backwards compatibility.

struct sha1\_ctx
:   Context for hashing a message with SHA-1

**Definition**:

```
struct sha1_ctx {
    struct sha1_block_state state;
    u64 bytecount;
    u8 buf[SHA1_BLOCK_SIZE];
};
```

**Members**

`state`
:   the compression function state

`bytecount`
:   number of bytes processed so far

`buf`
:   partial block buffer; bytecount % SHA1\_BLOCK\_SIZE bytes are valid

void sha1\_init(struct [sha1\_ctx](#c.sha1_ctx "sha1_ctx") \*ctx)
:   Initialize a SHA-1 context for a new message

**Parameters**

`struct sha1_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`sha1()`](#c.sha1 "sha1") instead.

**Context**

Any context.

void sha1\_update(struct [sha1\_ctx](#c.sha1_ctx "sha1_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a SHA-1 context with message data

**Parameters**

`struct sha1_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void sha1\_final(struct [sha1\_ctx](#c.sha1_ctx "sha1_ctx") \*ctx, u8 out[static SHA1\_DIGEST\_SIZE])
:   Finish computing a SHA-1 message digest

**Parameters**

`struct sha1_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static SHA1_DIGEST_SIZE]`
:   (output) the resulting SHA-1 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void sha1(const u8 \*data, size\_t len, u8 out[static SHA1\_DIGEST\_SIZE])
:   Compute SHA-1 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static SHA1_DIGEST_SIZE]`
:   (output) the resulting SHA-1 message digest

**Context**

Any context.

struct hmac\_sha1\_key
:   Prepared key for HMAC-SHA1

**Definition**:

```
struct hmac_sha1_key {
    struct sha1_block_state istate;
    struct sha1_block_state ostate;
};
```

**Members**

`istate`
:   private

`ostate`
:   private

struct hmac\_sha1\_ctx
:   Context for computing HMAC-SHA1 of a message

**Definition**:

```
struct hmac_sha1_ctx {
    struct sha1_ctx sha_ctx;
    struct sha1_block_state ostate;
};
```

**Members**

`sha_ctx`
:   private

`ostate`
:   private

void hmac\_sha1\_preparekey(struct [hmac\_sha1\_key](#c.hmac_sha1_key "hmac_sha1_key") \*key, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Prepare a key for HMAC-SHA1

**Parameters**

`struct hmac_sha1_key *key`
:   (output) the key structure to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA1 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Note**

the caller is responsible for zeroizing both the [`struct hmac_sha1_key`](#c.hmac_sha1_key "hmac_sha1_key")
and the raw key once they are no longer needed.

**Context**

Any context.

void hmac\_sha1\_init(struct [hmac\_sha1\_ctx](#c.hmac_sha1_ctx "hmac_sha1_ctx") \*ctx, const struct [hmac\_sha1\_key](#c.hmac_sha1_key "hmac_sha1_key") \*key)
:   Initialize an HMAC-SHA1 context for a new message

**Parameters**

`struct hmac_sha1_ctx *ctx`
:   (output) the HMAC context to initialize

`const struct hmac_sha1_key *key`
:   the prepared HMAC key

**Description**

If you don’t need incremental computation, consider [`hmac_sha1()`](#c.hmac_sha1 "hmac_sha1") instead.

**Context**

Any context.

void hmac\_sha1\_init\_usingrawkey(struct [hmac\_sha1\_ctx](#c.hmac_sha1_ctx "hmac_sha1_ctx") \*ctx, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Initialize an HMAC-SHA1 context for a new message, using a raw key

**Parameters**

`struct hmac_sha1_ctx *ctx`
:   (output) the HMAC context to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA1 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Description**

If you don’t need incremental computation, consider [`hmac_sha1_usingrawkey()`](#c.hmac_sha1_usingrawkey "hmac_sha1_usingrawkey")
instead.

**Context**

Any context.

void hmac\_sha1\_update(struct [hmac\_sha1\_ctx](#c.hmac_sha1_ctx "hmac_sha1_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an HMAC-SHA1 context with message data

**Parameters**

`struct hmac_sha1_ctx *ctx`
:   the HMAC context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void hmac\_sha1\_final(struct [hmac\_sha1\_ctx](#c.hmac_sha1_ctx "hmac_sha1_ctx") \*ctx, u8 out[static SHA1\_DIGEST\_SIZE])
:   Finish computing an HMAC-SHA1 value

**Parameters**

`struct hmac_sha1_ctx *ctx`
:   the HMAC context to finalize; must have been initialized

`u8 out[static SHA1_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA1 value

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void hmac\_sha1(const struct [hmac\_sha1\_key](#c.hmac_sha1_key "hmac_sha1_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static SHA1\_DIGEST\_SIZE])
:   Compute HMAC-SHA1 in one shot, using a prepared key

**Parameters**

`const struct hmac_sha1_key *key`
:   the prepared HMAC key

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA1_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA1 value

**Description**

If you’re using the key only once, consider using [`hmac_sha1_usingrawkey()`](#c.hmac_sha1_usingrawkey "hmac_sha1_usingrawkey").

**Context**

Any context.

void hmac\_sha1\_usingrawkey(const u8 \*raw\_key, size\_t raw\_key\_len, const u8 \*data, size\_t data\_len, u8 out[static SHA1\_DIGEST\_SIZE])
:   Compute HMAC-SHA1 in one shot, using a raw key

**Parameters**

`const u8 *raw_key`
:   the raw HMAC-SHA1 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA1_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA1 value

**Description**

If you’re using the key multiple times, prefer to use [`hmac_sha1_preparekey()`](#c.hmac_sha1_preparekey "hmac_sha1_preparekey")
followed by multiple calls to [`hmac_sha1()`](#c.hmac_sha1 "hmac_sha1") instead.

**Context**

Any context.

## SHA-2

Support for the SHA-2 family of cryptographic hash functions, including SHA-224,
SHA-256, SHA-384, and SHA-512. This also includes their corresponding HMACs:
HMAC-SHA224, HMAC-SHA256, HMAC-SHA384, and HMAC-SHA512.

struct sha224\_ctx
:   Context for hashing a message with SHA-224

**Definition**:

```
struct sha224_ctx {
    struct __sha256_ctx ctx;
};
```

**Members**

`ctx`
:   private

void sha224\_init(struct [sha224\_ctx](#c.sha224_ctx "sha224_ctx") \*ctx)
:   Initialize a SHA-224 context for a new message

**Parameters**

`struct sha224_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`sha224()`](#c.sha224 "sha224") instead.

**Context**

Any context.

void sha224\_update(struct [sha224\_ctx](#c.sha224_ctx "sha224_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a SHA-224 context with message data

**Parameters**

`struct sha224_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void sha224\_final(struct [sha224\_ctx](#c.sha224_ctx "sha224_ctx") \*ctx, u8 out[static SHA224\_DIGEST\_SIZE])
:   Finish computing a SHA-224 message digest

**Parameters**

`struct sha224_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static SHA224_DIGEST_SIZE]`
:   (output) the resulting SHA-224 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void sha224(const u8 \*data, size\_t len, u8 out[static SHA224\_DIGEST\_SIZE])
:   Compute SHA-224 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static SHA224_DIGEST_SIZE]`
:   (output) the resulting SHA-224 message digest

**Context**

Any context.

struct hmac\_sha224\_key
:   Prepared key for HMAC-SHA224

**Definition**:

```
struct hmac_sha224_key {
    struct __hmac_sha256_key key;
};
```

**Members**

`key`
:   private

struct hmac\_sha224\_ctx
:   Context for computing HMAC-SHA224 of a message

**Definition**:

```
struct hmac_sha224_ctx {
    struct __hmac_sha256_ctx ctx;
};
```

**Members**

`ctx`
:   private

void hmac\_sha224\_preparekey(struct [hmac\_sha224\_key](#c.hmac_sha224_key "hmac_sha224_key") \*key, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Prepare a key for HMAC-SHA224

**Parameters**

`struct hmac_sha224_key *key`
:   (output) the key structure to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA224 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Note**

the caller is responsible for zeroizing both the [`struct hmac_sha224_key`](#c.hmac_sha224_key "hmac_sha224_key")
and the raw key once they are no longer needed.

**Context**

Any context.

void hmac\_sha224\_init(struct [hmac\_sha224\_ctx](#c.hmac_sha224_ctx "hmac_sha224_ctx") \*ctx, const struct [hmac\_sha224\_key](#c.hmac_sha224_key "hmac_sha224_key") \*key)
:   Initialize an HMAC-SHA224 context for a new message

**Parameters**

`struct hmac_sha224_ctx *ctx`
:   (output) the HMAC context to initialize

`const struct hmac_sha224_key *key`
:   the prepared HMAC key

**Description**

If you don’t need incremental computation, consider [`hmac_sha224()`](#c.hmac_sha224 "hmac_sha224") instead.

**Context**

Any context.

void hmac\_sha224\_init\_usingrawkey(struct [hmac\_sha224\_ctx](#c.hmac_sha224_ctx "hmac_sha224_ctx") \*ctx, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Initialize an HMAC-SHA224 context for a new message, using a raw key

**Parameters**

`struct hmac_sha224_ctx *ctx`
:   (output) the HMAC context to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA224 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Description**

If you don’t need incremental computation, consider [`hmac_sha224_usingrawkey()`](#c.hmac_sha224_usingrawkey "hmac_sha224_usingrawkey")
instead.

**Context**

Any context.

void hmac\_sha224\_update(struct [hmac\_sha224\_ctx](#c.hmac_sha224_ctx "hmac_sha224_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an HMAC-SHA224 context with message data

**Parameters**

`struct hmac_sha224_ctx *ctx`
:   the HMAC context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void hmac\_sha224\_final(struct [hmac\_sha224\_ctx](#c.hmac_sha224_ctx "hmac_sha224_ctx") \*ctx, u8 out[static SHA224\_DIGEST\_SIZE])
:   Finish computing an HMAC-SHA224 value

**Parameters**

`struct hmac_sha224_ctx *ctx`
:   the HMAC context to finalize; must have been initialized

`u8 out[static SHA224_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA224 value

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void hmac\_sha224(const struct [hmac\_sha224\_key](#c.hmac_sha224_key "hmac_sha224_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static SHA224\_DIGEST\_SIZE])
:   Compute HMAC-SHA224 in one shot, using a prepared key

**Parameters**

`const struct hmac_sha224_key *key`
:   the prepared HMAC key

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA224_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA224 value

**Description**

If you’re using the key only once, consider using [`hmac_sha224_usingrawkey()`](#c.hmac_sha224_usingrawkey "hmac_sha224_usingrawkey").

**Context**

Any context.

void hmac\_sha224\_usingrawkey(const u8 \*raw\_key, size\_t raw\_key\_len, const u8 \*data, size\_t data\_len, u8 out[static SHA224\_DIGEST\_SIZE])
:   Compute HMAC-SHA224 in one shot, using a raw key

**Parameters**

`const u8 *raw_key`
:   the raw HMAC-SHA224 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA224_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA224 value

**Description**

If you’re using the key multiple times, prefer to use
[`hmac_sha224_preparekey()`](#c.hmac_sha224_preparekey "hmac_sha224_preparekey") followed by multiple calls to [`hmac_sha224()`](#c.hmac_sha224 "hmac_sha224") instead.

**Context**

Any context.

struct sha256\_ctx
:   Context for hashing a message with SHA-256

**Definition**:

```
struct sha256_ctx {
    struct __sha256_ctx ctx;
};
```

**Members**

`ctx`
:   private

void sha256\_init(struct [sha256\_ctx](#c.sha256_ctx "sha256_ctx") \*ctx)
:   Initialize a SHA-256 context for a new message

**Parameters**

`struct sha256_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`sha256()`](#c.sha256 "sha256") instead.

**Context**

Any context.

void sha256\_update(struct [sha256\_ctx](#c.sha256_ctx "sha256_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a SHA-256 context with message data

**Parameters**

`struct sha256_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void sha256\_final(struct [sha256\_ctx](#c.sha256_ctx "sha256_ctx") \*ctx, u8 out[static SHA256\_DIGEST\_SIZE])
:   Finish computing a SHA-256 message digest

**Parameters**

`struct sha256_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static SHA256_DIGEST_SIZE]`
:   (output) the resulting SHA-256 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void sha256(const u8 \*data, size\_t len, u8 out[static SHA256\_DIGEST\_SIZE])
:   Compute SHA-256 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static SHA256_DIGEST_SIZE]`
:   (output) the resulting SHA-256 message digest

**Context**

Any context.

void sha256\_finup\_2x(const struct [sha256\_ctx](#c.sha256_ctx "sha256_ctx") \*ctx, const u8 \*data1, const u8 \*data2, size\_t len, u8 out1[static SHA256\_DIGEST\_SIZE], u8 out2[static SHA256\_DIGEST\_SIZE])
:   Compute two SHA-256 digests from a common initial context. On some CPUs, this is faster than sequentially computing each digest.

**Parameters**

`const struct sha256_ctx *ctx`
:   an optional initial context, which may have already processed data. If
    NULL, a default initial context is used (equivalent to [`sha256_init()`](#c.sha256_init "sha256_init")).

`const u8 *data1`
:   data for the first message

`const u8 *data2`
:   data for the second message

`size_t len`
:   the length of each of **data1** and **data2**, in bytes

`u8 out1[static SHA256_DIGEST_SIZE]`
:   (output) the first SHA-256 message digest

`u8 out2[static SHA256_DIGEST_SIZE]`
:   (output) the second SHA-256 message digest

**Context**

Any context.

bool sha256\_finup\_2x\_is\_optimized(void)
:   Check if [`sha256_finup_2x()`](#c.sha256_finup_2x "sha256_finup_2x") is using a real interleaved implementation, as opposed to a sequential fallback

**Parameters**

`void`
:   no arguments

**Return**

true if optimized

**Context**

Any context.

struct hmac\_sha256\_key
:   Prepared key for HMAC-SHA256

**Definition**:

```
struct hmac_sha256_key {
    struct __hmac_sha256_key key;
};
```

**Members**

`key`
:   private

struct hmac\_sha256\_ctx
:   Context for computing HMAC-SHA256 of a message

**Definition**:

```
struct hmac_sha256_ctx {
    struct __hmac_sha256_ctx ctx;
};
```

**Members**

`ctx`
:   private

void hmac\_sha256\_preparekey(struct [hmac\_sha256\_key](#c.hmac_sha256_key "hmac_sha256_key") \*key, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Prepare a key for HMAC-SHA256

**Parameters**

`struct hmac_sha256_key *key`
:   (output) the key structure to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA256 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Note**

the caller is responsible for zeroizing both the [`struct hmac_sha256_key`](#c.hmac_sha256_key "hmac_sha256_key")
and the raw key once they are no longer needed.

**Context**

Any context.

void hmac\_sha256\_init(struct [hmac\_sha256\_ctx](#c.hmac_sha256_ctx "hmac_sha256_ctx") \*ctx, const struct [hmac\_sha256\_key](#c.hmac_sha256_key "hmac_sha256_key") \*key)
:   Initialize an HMAC-SHA256 context for a new message

**Parameters**

`struct hmac_sha256_ctx *ctx`
:   (output) the HMAC context to initialize

`const struct hmac_sha256_key *key`
:   the prepared HMAC key

**Description**

If you don’t need incremental computation, consider [`hmac_sha256()`](#c.hmac_sha256 "hmac_sha256") instead.

**Context**

Any context.

void hmac\_sha256\_init\_usingrawkey(struct [hmac\_sha256\_ctx](#c.hmac_sha256_ctx "hmac_sha256_ctx") \*ctx, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Initialize an HMAC-SHA256 context for a new message, using a raw key

**Parameters**

`struct hmac_sha256_ctx *ctx`
:   (output) the HMAC context to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA256 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Description**

If you don’t need incremental computation, consider [`hmac_sha256_usingrawkey()`](#c.hmac_sha256_usingrawkey "hmac_sha256_usingrawkey")
instead.

**Context**

Any context.

void hmac\_sha256\_update(struct [hmac\_sha256\_ctx](#c.hmac_sha256_ctx "hmac_sha256_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an HMAC-SHA256 context with message data

**Parameters**

`struct hmac_sha256_ctx *ctx`
:   the HMAC context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void hmac\_sha256\_final(struct [hmac\_sha256\_ctx](#c.hmac_sha256_ctx "hmac_sha256_ctx") \*ctx, u8 out[static SHA256\_DIGEST\_SIZE])
:   Finish computing an HMAC-SHA256 value

**Parameters**

`struct hmac_sha256_ctx *ctx`
:   the HMAC context to finalize; must have been initialized

`u8 out[static SHA256_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA256 value

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void hmac\_sha256(const struct [hmac\_sha256\_key](#c.hmac_sha256_key "hmac_sha256_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static SHA256\_DIGEST\_SIZE])
:   Compute HMAC-SHA256 in one shot, using a prepared key

**Parameters**

`const struct hmac_sha256_key *key`
:   the prepared HMAC key

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA256_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA256 value

**Description**

If you’re using the key only once, consider using [`hmac_sha256_usingrawkey()`](#c.hmac_sha256_usingrawkey "hmac_sha256_usingrawkey").

**Context**

Any context.

void hmac\_sha256\_usingrawkey(const u8 \*raw\_key, size\_t raw\_key\_len, const u8 \*data, size\_t data\_len, u8 out[static SHA256\_DIGEST\_SIZE])
:   Compute HMAC-SHA256 in one shot, using a raw key

**Parameters**

`const u8 *raw_key`
:   the raw HMAC-SHA256 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA256_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA256 value

**Description**

If you’re using the key multiple times, prefer to use
[`hmac_sha256_preparekey()`](#c.hmac_sha256_preparekey "hmac_sha256_preparekey") followed by multiple calls to [`hmac_sha256()`](#c.hmac_sha256 "hmac_sha256") instead.

**Context**

Any context.

struct sha384\_ctx
:   Context for hashing a message with SHA-384

**Definition**:

```
struct sha384_ctx {
    struct __sha512_ctx ctx;
};
```

**Members**

`ctx`
:   private

void sha384\_init(struct [sha384\_ctx](#c.sha384_ctx "sha384_ctx") \*ctx)
:   Initialize a SHA-384 context for a new message

**Parameters**

`struct sha384_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`sha384()`](#c.sha384 "sha384") instead.

**Context**

Any context.

void sha384\_update(struct [sha384\_ctx](#c.sha384_ctx "sha384_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a SHA-384 context with message data

**Parameters**

`struct sha384_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void sha384\_final(struct [sha384\_ctx](#c.sha384_ctx "sha384_ctx") \*ctx, u8 out[static SHA384\_DIGEST\_SIZE])
:   Finish computing a SHA-384 message digest

**Parameters**

`struct sha384_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static SHA384_DIGEST_SIZE]`
:   (output) the resulting SHA-384 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void sha384(const u8 \*data, size\_t len, u8 out[static SHA384\_DIGEST\_SIZE])
:   Compute SHA-384 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static SHA384_DIGEST_SIZE]`
:   (output) the resulting SHA-384 message digest

**Context**

Any context.

struct hmac\_sha384\_key
:   Prepared key for HMAC-SHA384

**Definition**:

```
struct hmac_sha384_key {
    struct __hmac_sha512_key key;
};
```

**Members**

`key`
:   private

struct hmac\_sha384\_ctx
:   Context for computing HMAC-SHA384 of a message

**Definition**:

```
struct hmac_sha384_ctx {
    struct __hmac_sha512_ctx ctx;
};
```

**Members**

`ctx`
:   private

void hmac\_sha384\_preparekey(struct [hmac\_sha384\_key](#c.hmac_sha384_key "hmac_sha384_key") \*key, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Prepare a key for HMAC-SHA384

**Parameters**

`struct hmac_sha384_key *key`
:   (output) the key structure to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA384 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Note**

the caller is responsible for zeroizing both the [`struct hmac_sha384_key`](#c.hmac_sha384_key "hmac_sha384_key")
and the raw key once they are no longer needed.

**Context**

Any context.

void hmac\_sha384\_init(struct [hmac\_sha384\_ctx](#c.hmac_sha384_ctx "hmac_sha384_ctx") \*ctx, const struct [hmac\_sha384\_key](#c.hmac_sha384_key "hmac_sha384_key") \*key)
:   Initialize an HMAC-SHA384 context for a new message

**Parameters**

`struct hmac_sha384_ctx *ctx`
:   (output) the HMAC context to initialize

`const struct hmac_sha384_key *key`
:   the prepared HMAC key

**Description**

If you don’t need incremental computation, consider [`hmac_sha384()`](#c.hmac_sha384 "hmac_sha384") instead.

**Context**

Any context.

void hmac\_sha384\_init\_usingrawkey(struct [hmac\_sha384\_ctx](#c.hmac_sha384_ctx "hmac_sha384_ctx") \*ctx, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Initialize an HMAC-SHA384 context for a new message, using a raw key

**Parameters**

`struct hmac_sha384_ctx *ctx`
:   (output) the HMAC context to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA384 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Description**

If you don’t need incremental computation, consider [`hmac_sha384_usingrawkey()`](#c.hmac_sha384_usingrawkey "hmac_sha384_usingrawkey")
instead.

**Context**

Any context.

void hmac\_sha384\_update(struct [hmac\_sha384\_ctx](#c.hmac_sha384_ctx "hmac_sha384_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an HMAC-SHA384 context with message data

**Parameters**

`struct hmac_sha384_ctx *ctx`
:   the HMAC context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void hmac\_sha384\_final(struct [hmac\_sha384\_ctx](#c.hmac_sha384_ctx "hmac_sha384_ctx") \*ctx, u8 out[static SHA384\_DIGEST\_SIZE])
:   Finish computing an HMAC-SHA384 value

**Parameters**

`struct hmac_sha384_ctx *ctx`
:   the HMAC context to finalize; must have been initialized

`u8 out[static SHA384_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA384 value

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void hmac\_sha384(const struct [hmac\_sha384\_key](#c.hmac_sha384_key "hmac_sha384_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static SHA384\_DIGEST\_SIZE])
:   Compute HMAC-SHA384 in one shot, using a prepared key

**Parameters**

`const struct hmac_sha384_key *key`
:   the prepared HMAC key

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA384_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA384 value

**Description**

If you’re using the key only once, consider using [`hmac_sha384_usingrawkey()`](#c.hmac_sha384_usingrawkey "hmac_sha384_usingrawkey").

**Context**

Any context.

void hmac\_sha384\_usingrawkey(const u8 \*raw\_key, size\_t raw\_key\_len, const u8 \*data, size\_t data\_len, u8 out[static SHA384\_DIGEST\_SIZE])
:   Compute HMAC-SHA384 in one shot, using a raw key

**Parameters**

`const u8 *raw_key`
:   the raw HMAC-SHA384 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA384_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA384 value

**Description**

If you’re using the key multiple times, prefer to use
[`hmac_sha384_preparekey()`](#c.hmac_sha384_preparekey "hmac_sha384_preparekey") followed by multiple calls to [`hmac_sha384()`](#c.hmac_sha384 "hmac_sha384") instead.

**Context**

Any context.

struct sha512\_ctx
:   Context for hashing a message with SHA-512

**Definition**:

```
struct sha512_ctx {
    struct __sha512_ctx ctx;
};
```

**Members**

`ctx`
:   private

void sha512\_init(struct [sha512\_ctx](#c.sha512_ctx "sha512_ctx") \*ctx)
:   Initialize a SHA-512 context for a new message

**Parameters**

`struct sha512_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`sha512()`](#c.sha512 "sha512") instead.

**Context**

Any context.

void sha512\_update(struct [sha512\_ctx](#c.sha512_ctx "sha512_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update a SHA-512 context with message data

**Parameters**

`struct sha512_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void sha512\_final(struct [sha512\_ctx](#c.sha512_ctx "sha512_ctx") \*ctx, u8 out[static SHA512\_DIGEST\_SIZE])
:   Finish computing a SHA-512 message digest

**Parameters**

`struct sha512_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static SHA512_DIGEST_SIZE]`
:   (output) the resulting SHA-512 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void sha512(const u8 \*data, size\_t len, u8 out[static SHA512\_DIGEST\_SIZE])
:   Compute SHA-512 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static SHA512_DIGEST_SIZE]`
:   (output) the resulting SHA-512 message digest

**Context**

Any context.

struct hmac\_sha512\_key
:   Prepared key for HMAC-SHA512

**Definition**:

```
struct hmac_sha512_key {
    struct __hmac_sha512_key key;
};
```

**Members**

`key`
:   private

struct hmac\_sha512\_ctx
:   Context for computing HMAC-SHA512 of a message

**Definition**:

```
struct hmac_sha512_ctx {
    struct __hmac_sha512_ctx ctx;
};
```

**Members**

`ctx`
:   private

void hmac\_sha512\_preparekey(struct [hmac\_sha512\_key](#c.hmac_sha512_key "hmac_sha512_key") \*key, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Prepare a key for HMAC-SHA512

**Parameters**

`struct hmac_sha512_key *key`
:   (output) the key structure to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA512 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Note**

the caller is responsible for zeroizing both the [`struct hmac_sha512_key`](#c.hmac_sha512_key "hmac_sha512_key")
and the raw key once they are no longer needed.

**Context**

Any context.

void hmac\_sha512\_init(struct [hmac\_sha512\_ctx](#c.hmac_sha512_ctx "hmac_sha512_ctx") \*ctx, const struct [hmac\_sha512\_key](#c.hmac_sha512_key "hmac_sha512_key") \*key)
:   Initialize an HMAC-SHA512 context for a new message

**Parameters**

`struct hmac_sha512_ctx *ctx`
:   (output) the HMAC context to initialize

`const struct hmac_sha512_key *key`
:   the prepared HMAC key

**Description**

If you don’t need incremental computation, consider [`hmac_sha512()`](#c.hmac_sha512 "hmac_sha512") instead.

**Context**

Any context.

void hmac\_sha512\_init\_usingrawkey(struct [hmac\_sha512\_ctx](#c.hmac_sha512_ctx "hmac_sha512_ctx") \*ctx, const u8 \*raw\_key, size\_t raw\_key\_len)
:   Initialize an HMAC-SHA512 context for a new message, using a raw key

**Parameters**

`struct hmac_sha512_ctx *ctx`
:   (output) the HMAC context to initialize

`const u8 *raw_key`
:   the raw HMAC-SHA512 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

**Description**

If you don’t need incremental computation, consider [`hmac_sha512_usingrawkey()`](#c.hmac_sha512_usingrawkey "hmac_sha512_usingrawkey")
instead.

**Context**

Any context.

void hmac\_sha512\_update(struct [hmac\_sha512\_ctx](#c.hmac_sha512_ctx "hmac_sha512_ctx") \*ctx, const u8 \*data, size\_t data\_len)
:   Update an HMAC-SHA512 context with message data

**Parameters**

`struct hmac_sha512_ctx *ctx`
:   the HMAC context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void hmac\_sha512\_final(struct [hmac\_sha512\_ctx](#c.hmac_sha512_ctx "hmac_sha512_ctx") \*ctx, u8 out[static SHA512\_DIGEST\_SIZE])
:   Finish computing an HMAC-SHA512 value

**Parameters**

`struct hmac_sha512_ctx *ctx`
:   the HMAC context to finalize; must have been initialized

`u8 out[static SHA512_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA512 value

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void hmac\_sha512(const struct [hmac\_sha512\_key](#c.hmac_sha512_key "hmac_sha512_key") \*key, const u8 \*data, size\_t data\_len, u8 out[static SHA512\_DIGEST\_SIZE])
:   Compute HMAC-SHA512 in one shot, using a prepared key

**Parameters**

`const struct hmac_sha512_key *key`
:   the prepared HMAC key

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA512_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA512 value

**Description**

If you’re using the key only once, consider using [`hmac_sha512_usingrawkey()`](#c.hmac_sha512_usingrawkey "hmac_sha512_usingrawkey").

**Context**

Any context.

void hmac\_sha512\_usingrawkey(const u8 \*raw\_key, size\_t raw\_key\_len, const u8 \*data, size\_t data\_len, u8 out[static SHA512\_DIGEST\_SIZE])
:   Compute HMAC-SHA512 in one shot, using a raw key

**Parameters**

`const u8 *raw_key`
:   the raw HMAC-SHA512 key

`size_t raw_key_len`
:   the key length in bytes. All key lengths are supported.

`const u8 *data`
:   the message data

`size_t data_len`
:   the data length in bytes

`u8 out[static SHA512_DIGEST_SIZE]`
:   (output) the resulting HMAC-SHA512 value

**Description**

If you’re using the key multiple times, prefer to use
[`hmac_sha512_preparekey()`](#c.hmac_sha512_preparekey "hmac_sha512_preparekey") followed by multiple calls to [`hmac_sha512()`](#c.hmac_sha512 "hmac_sha512") instead.

**Context**

Any context.

## SHA-3

The SHA-3 functions are documented in [SHA-3 Algorithm Collection](sha3.html#sha3).

## SM3

Support for the SM3 cryptographic hash function.

struct sm3\_ctx
:   Context for hashing a message with SM3

**Definition**:

```
struct sm3_ctx {
    struct sm3_block_state state;
    u64 bytecount;
    u8 buf[SM3_BLOCK_SIZE];
};
```

**Members**

`state`
:   the compression function state

`bytecount`
:   number of bytes processed so far

`buf`
:   partial block buffer; bytecount % SM3\_BLOCK\_SIZE bytes are valid

void sm3\_init(struct [sm3\_ctx](#c.sm3_ctx "sm3_ctx") \*ctx)
:   Initialize an SM3 context for a new message

**Parameters**

`struct sm3_ctx *ctx`
:   the context to initialize

**Description**

If you don’t need incremental computation, consider [`sm3()`](#c.sm3 "sm3") instead.

**Context**

Any context.

void sm3\_update(struct [sm3\_ctx](#c.sm3_ctx "sm3_ctx") \*ctx, const u8 \*data, size\_t len)
:   Update an SM3 context with message data

**Parameters**

`struct sm3_ctx *ctx`
:   the context to update; must have been initialized

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

**Description**

This can be called any number of times.

**Context**

Any context.

void sm3\_final(struct [sm3\_ctx](#c.sm3_ctx "sm3_ctx") \*ctx, u8 out[static SM3\_DIGEST\_SIZE])
:   Finish computing an SM3 message digest

**Parameters**

`struct sm3_ctx *ctx`
:   the context to finalize; must have been initialized

`u8 out[static SM3_DIGEST_SIZE]`
:   (output) the resulting SM3 message digest

**Description**

After finishing, this zeroizes **ctx**. So the caller does not need to do it.

**Context**

Any context.

void sm3(const u8 \*data, size\_t len, u8 out[static SM3\_DIGEST\_SIZE])
:   Compute SM3 message digest in one shot

**Parameters**

`const u8 *data`
:   the message data

`size_t len`
:   the data length in bytes

`u8 out[static SM3_DIGEST_SIZE]`
:   (output) the resulting SM3 message digest

**Context**

Any context.
