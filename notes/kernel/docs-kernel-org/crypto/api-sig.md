# Asymmetric Signature

> 출처(원문): https://docs.kernel.org/crypto/api-sig.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Asymmetric Signature

## Asymmetric Signature Algorithm Definitions

struct sig\_alg
:   generic public key signature algorithm

**Definition**:

```
struct sig_alg {
    int (*sign)(struct crypto_sig *tfm, const void *src, unsigned int slen, void *dst, unsigned int dlen);
    int (*verify)(struct crypto_sig *tfm, const void *src, unsigned int slen, const void *digest, unsigned int dlen);
    int (*set_pub_key)(struct crypto_sig *tfm, const void *key, unsigned int keylen);
    int (*set_priv_key)(struct crypto_sig *tfm, const void *key, unsigned int keylen);
    unsigned int (*key_size)(struct crypto_sig *tfm);
    unsigned int (*digest_size)(struct crypto_sig *tfm);
    unsigned int (*max_size)(struct crypto_sig *tfm);
    int (*init)(struct crypto_sig *tfm);
    void (*exit)(struct crypto_sig *tfm);
    struct crypto_alg base;
};
```

**Members**

`sign`
:   Function performs a sign operation as defined by public key
    algorithm. On success, the signature size is returned.
    Optional.

`verify`
:   Function performs a complete verify operation as defined by
    public key algorithm, returning verification status. Optional.

`set_pub_key`
:   Function invokes the algorithm specific set public key
    function, which knows how to decode and interpret
    the BER encoded public key and parameters. Mandatory.

`set_priv_key`
:   Function invokes the algorithm specific set private key
    function, which knows how to decode and interpret
    the BER encoded private key and parameters. Optional.

`key_size`
:   Function returns key size. Mandatory.

`digest_size`
:   Function returns maximum digest size. Optional.

`max_size`
:   Function returns maximum signature size. Optional.

`init`
:   Initialize the cryptographic transformation object.
    This function is used to initialize the cryptographic
    transformation object. This function is called only once at
    the instantiation time, right after the transformation context
    was allocated. In case the cryptographic hardware has some
    special requirements which need to be handled by software, this
    function shall check for the precise requirement of the
    transformation and put any software fallbacks in place.

`exit`
:   Deinitialize the cryptographic transformation object. This is a
    counterpart to **init**, used to remove various changes set in
    **init**.

`base`
:   Common crypto API algorithm data structure

## Asymmetric Signature API

The Public Key Signature API is used with the algorithms of type
CRYPTO\_ALG\_TYPE\_SIG (listed as type “sig” in /proc/crypto)

struct crypto\_sig \*crypto\_alloc\_sig(const char \*alg\_name, u32 type, u32 mask)
:   allocate signature tfm handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    signing algorithm e.g. “ecdsa”

`u32 type`
:   specifies the type of the algorithm

`u32 mask`
:   specifies the mask for the algorithm

**Description**

Allocate a handle for public key signature algorithm. The returned `struct
crypto_sig` is the handle that is required for any subsequent
API invocation for signature operations.

**Return**

allocated handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_sig(struct crypto\_sig \*tfm)
:   free signature tfm handle

**Parameters**

`struct crypto_sig *tfm`
:   signature tfm handle allocated with [`crypto_alloc_sig()`](#c.crypto_alloc_sig "crypto_alloc_sig")

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

unsigned int crypto\_sig\_keysize(struct crypto\_sig \*tfm)
:   Get key size

**Parameters**

`struct crypto_sig *tfm`
:   signature tfm handle allocated with [`crypto_alloc_sig()`](#c.crypto_alloc_sig "crypto_alloc_sig")

**Description**

Function returns the key size in bits.
Function assumes that the key is already set in the transformation. If this
function is called without a setkey or with a failed setkey, you may end up
in a NULL dereference.

unsigned int crypto\_sig\_digestsize(struct crypto\_sig \*tfm)
:   Get maximum digest size

**Parameters**

`struct crypto_sig *tfm`
:   signature tfm handle allocated with [`crypto_alloc_sig()`](#c.crypto_alloc_sig "crypto_alloc_sig")

**Description**

Function returns the maximum digest size in bytes.
Function assumes that the key is already set in the transformation. If this
function is called without a setkey or with a failed setkey, you may end up
in a NULL dereference.

unsigned int crypto\_sig\_maxsize(struct crypto\_sig \*tfm)
:   Get maximum signature size

**Parameters**

`struct crypto_sig *tfm`
:   signature tfm handle allocated with [`crypto_alloc_sig()`](#c.crypto_alloc_sig "crypto_alloc_sig")

**Description**

Function returns the maximum signature size in bytes.
Function assumes that the key is already set in the transformation. If this
function is called without a setkey or with a failed setkey, you may end up
in a NULL dereference.

int crypto\_sig\_sign(struct crypto\_sig \*tfm, const void \*src, unsigned int slen, void \*dst, unsigned int dlen)
:   Invoke signing operation

**Parameters**

`struct crypto_sig *tfm`
:   signature tfm handle allocated with [`crypto_alloc_sig()`](#c.crypto_alloc_sig "crypto_alloc_sig")

`const void *src`
:   source buffer

`unsigned int slen`
:   source length

`void *dst`
:   destination obuffer

`unsigned int dlen`
:   destination length

**Description**

Function invokes the specific signing operation for a given algorithm

**Return**

signature size on success; error code in case of error

int crypto\_sig\_verify(struct crypto\_sig \*tfm, const void \*src, unsigned int slen, const void \*digest, unsigned int dlen)
:   Invoke signature verification

**Parameters**

`struct crypto_sig *tfm`
:   signature tfm handle allocated with [`crypto_alloc_sig()`](#c.crypto_alloc_sig "crypto_alloc_sig")

`const void *src`
:   source buffer

`unsigned int slen`
:   source length

`const void *digest`
:   digest

`unsigned int dlen`
:   digest length

**Description**

Function invokes the specific signature verification operation
for a given algorithm.

**Return**

zero on verification success; error code in case of error.

int crypto\_sig\_set\_pubkey(struct crypto\_sig \*tfm, const void \*key, unsigned int keylen)
:   Invoke set public key operation

**Parameters**

`struct crypto_sig *tfm`
:   tfm handle

`const void *key`
:   BER encoded public key, algo OID, paramlen, BER encoded
    parameters

`unsigned int keylen`
:   length of the key (not including other data)

**Description**

Function invokes the algorithm specific set key function, which knows
how to decode and interpret the encoded key and parameters

**Return**

zero on success; error code in case of error

int crypto\_sig\_set\_privkey(struct crypto\_sig \*tfm, const void \*key, unsigned int keylen)
:   Invoke set private key operation

**Parameters**

`struct crypto_sig *tfm`
:   tfm handle

`const void *key`
:   BER encoded private key, algo OID, paramlen, BER encoded
    parameters

`unsigned int keylen`
:   length of the key (not including other data)

**Description**

Function invokes the algorithm specific set key function, which knows
how to decode and interpret the encoded key and parameters

**Return**

zero on success; error code in case of error
