# Block ciphers

> 출처(원문): https://docs.kernel.org/crypto/libcrypto-blockcipher.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Block ciphers

## AES

Support for the AES block cipher.

struct aes\_enckey
:   An AES key prepared for encryption

**Definition**:

```
struct aes_enckey {
    u32 len;
    u32 nrounds;
    u32 padding[2];
    union aes_enckey_arch k;
};
```

**Members**

`len`
:   Key length in bytes: 16 for AES-128, 24 for AES-192, 32 for AES-256.

`nrounds`
:   Number of rounds: 10 for AES-128, 12 for AES-192, 14 for AES-256.
    This is ‘6 + **len** / 4’ and is cached so that AES implementations
    that need it don’t have to recompute it for each en/decryption.

`padding`
:   Padding to make offsetof(**k**) be a multiple of 16, so that aligning
    this `struct to` a 16-byte boundary results in **k** also being 16-byte
    aligned. Users aren’t required to align this `struct to` 16 bytes,
    but it may slightly improve performance.

`k`
:   This typically contains the AES round keys as an array of ‘**nrounds** + 1’
    groups of four u32 words. However, architecture-specific implementations
    of AES may store something else here, e.g. just the raw key if it’s all
    they need.

**Description**

Note that this `struct is` about half the size of [`struct aes_key`](#c.aes_key "aes_key"). This is
separate from [`struct aes_key`](#c.aes_key "aes_key") so that modes that need only AES encryption
(e.g. AES-GCM, AES-CTR, AES-CMAC, tweak key in AES-XTS) don’t incur the time
and space overhead of computing and caching the decryption round keys.

Note that there’s no decryption-only equivalent (i.e. “`struct aes_deckey`”),
since (a) it’s rare that modes need decryption-only, and (b) some AES
implementations use the same **k** for both encryption and decryption, either
always or conditionally; in the latter case both **k** and **inv\_k** are needed.

struct aes\_key
:   An AES key prepared for encryption and decryption

**Definition**:

```
struct aes_key {
    struct aes_enckey;
    union aes_invkey_arch inv_k;
};
```

**Members**

`aes_enckey`
:   Common fields and the key prepared for encryption

`inv_k`
:   This generally contains the round keys for the AES Equivalent
    Inverse Cipher, as an array of ‘**nrounds** + 1’ groups of four u32
    words. However, architecture-specific implementations of AES may
    store something else here. For example, they may leave this field
    uninitialized if they use **k** for both encryption and decryption.

int aes\_expandkey(struct crypto\_aes\_ctx \*ctx, const u8 \*in\_key, unsigned int key\_len)
:   Expands the AES key as described in FIPS-197

**Parameters**

`struct crypto_aes_ctx *ctx`
:   The location where the computed key will be stored.

`const u8 *in_key`
:   The supplied key.

`unsigned int key_len`
:   The length of the supplied key.

**Description**

Returns 0 on success. The function fails only if an invalid key size (or
pointer) is supplied.
The expanded key size is 240 bytes (max of 14 rounds with a unique 16 bytes
key schedule plus a 16 bytes key which is used before the first round).
The decryption key is prepared for the “Equivalent Inverse Cipher” as
described in FIPS-197. The first slot (16 bytes) of each key (enc or dec) is
for the initial combination, the second slot for the first round and so on.

int aes\_preparekey(struct [aes\_key](#c.aes_key "aes_key") \*key, const u8 \*in\_key, size\_t key\_len)
:   Prepare an AES key for encryption and decryption

**Parameters**

`struct aes_key *key`
:   (output) The key structure to initialize

`const u8 *in_key`
:   The raw AES key

`size_t key_len`
:   Length of the raw key in bytes. Should be either AES\_KEYSIZE\_128,
    AES\_KEYSIZE\_192, or AES\_KEYSIZE\_256.

**Description**

This prepares an AES key for both the encryption and decryption directions of
the block cipher. Typically this involves expanding the raw key into both
the standard round keys and the Equivalent Inverse Cipher round keys, but
some architecture-specific implementations don’t do the full expansion here.

The caller is responsible for zeroizing both the [`struct aes_key`](#c.aes_key "aes_key") and the raw
key once they are no longer needed.

If you don’t need decryption support, use [`aes_prepareenckey()`](#c.aes_prepareenckey "aes_prepareenckey") instead.

**Return**

0 on success or -EINVAL if the given key length is invalid. No other
errors are possible, so callers that always pass a valid key length
don’t need to check for errors.

**Context**

Any context.

int aes\_prepareenckey(struct [aes\_enckey](#c.aes_enckey "aes_enckey") \*key, const u8 \*in\_key, size\_t key\_len)
:   Prepare an AES key for encryption-only

**Parameters**

`struct aes_enckey *key`
:   (output) The key structure to initialize

`const u8 *in_key`
:   The raw AES key

`size_t key_len`
:   Length of the raw key in bytes. Should be either AES\_KEYSIZE\_128,
    AES\_KEYSIZE\_192, or AES\_KEYSIZE\_256.

**Description**

This prepares an AES key for only the encryption direction of the block
cipher. Typically this involves expanding the raw key into only the standard
round keys, resulting in a `struct about` half the size of [`struct aes_key`](#c.aes_key "aes_key").

The caller is responsible for zeroizing both the [`struct aes_enckey`](#c.aes_enckey "aes_enckey") and the
raw key once they are no longer needed.

Note that while the resulting prepared key supports only AES encryption, it
can still be used for decrypting in a mode of operation that uses AES in only
the encryption (forward) direction, for example counter mode.

**Return**

0 on success or -EINVAL if the given key length is invalid. No other
errors are possible, so callers that always pass a valid key length
don’t need to check for errors.

**Context**

Any context.

void aes\_encrypt(aes\_encrypt\_arg key, u8 out[static AES\_BLOCK\_SIZE], const u8 in[static AES\_BLOCK\_SIZE])
:   Encrypt a single AES block

**Parameters**

`aes_encrypt_arg key`
:   The AES key, as a pointer to either an encryption-only key
    ([`struct aes_enckey`](#c.aes_enckey "aes_enckey")) or a full, bidirectional key ([`struct aes_key`](#c.aes_key "aes_key")).

`u8 out[static AES_BLOCK_SIZE]`
:   Buffer to store the ciphertext block

`const u8 in[static AES_BLOCK_SIZE]`
:   Buffer containing the plaintext block

**Context**

Any context.

void aes\_decrypt(const struct [aes\_key](#c.aes_key "aes_key") \*key, u8 out[static AES\_BLOCK\_SIZE], const u8 in[static AES\_BLOCK\_SIZE])
:   Decrypt a single AES block

**Parameters**

`const struct aes_key *key`
:   The AES key, previously initialized by [`aes_preparekey()`](#c.aes_preparekey "aes_preparekey")

`u8 out[static AES_BLOCK_SIZE]`
:   Buffer to store the plaintext block

`const u8 in[static AES_BLOCK_SIZE]`
:   Buffer containing the ciphertext block

**Context**

Any context.

## DES

Support for the DES block cipher. This algorithm is obsolete and is supported
only for backwards compatibility.

int des\_expand\_key(struct des\_ctx \*ctx, const u8 \*key, unsigned int keylen)
:   Expand a DES input key into a key schedule

**Parameters**

`struct des_ctx *ctx`
:   the key schedule

`const u8 *key`
:   buffer containing the input key

`unsigned int keylen`
:   size of the buffer contents

**Return**

0 on success, -EINVAL if the input key is rejected and -ENOKEY if
the key is accepted but has been found to be weak.

int des3\_ede\_expand\_key(struct des3\_ede\_ctx \*ctx, const u8 \*key, unsigned int keylen)
:   Expand a triple DES input key into a key schedule

**Parameters**

`struct des3_ede_ctx *ctx`
:   the key schedule

`const u8 *key`
:   buffer containing the input key

`unsigned int keylen`
:   size of the buffer contents

**Return**

0 on success, -EINVAL if the input key is rejected and -ENOKEY if
the key is accepted but has been found to be weak. Note that weak keys will
be rejected (and -EINVAL will be returned) when running in FIPS mode.
