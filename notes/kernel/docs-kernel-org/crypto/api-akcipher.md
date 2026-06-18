# Asymmetric Cipher

> 출처(원문): https://docs.kernel.org/crypto/api-akcipher.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Asymmetric Cipher

## Asymmetric Cipher Algorithm Definitions

struct akcipher\_request
:   public key cipher request

**Definition**:

```
struct akcipher_request {
    struct crypto_async_request base;
    struct scatterlist *src;
    struct scatterlist *dst;
    unsigned int src_len;
    unsigned int dst_len;
    void *__ctx[];
};
```

**Members**

`base`
:   Common attributes for async crypto requests

`src`
:   Source data

`dst`
:   Destination data

`src_len`
:   Size of the input buffer

`dst_len`
:   Size of **dst** buffer
    It needs to be at least as big as the expected result
    depending on the operation.
    After operation it will be updated with the actual size of the
    result.
    In case of error where the dst sgl size was insufficient,
    it will be updated to the size required for the operation.

`__ctx`
:   Start of private context data

struct akcipher\_alg
:   generic public key cipher algorithm

**Definition**:

```
struct akcipher_alg {
    int (*encrypt)(struct akcipher_request *req);
    int (*decrypt)(struct akcipher_request *req);
    int (*set_pub_key)(struct crypto_akcipher *tfm, const void *key, unsigned int keylen);
    int (*set_priv_key)(struct crypto_akcipher *tfm, const void *key, unsigned int keylen);
    unsigned int (*max_size)(struct crypto_akcipher *tfm);
    int (*init)(struct crypto_akcipher *tfm);
    void (*exit)(struct crypto_akcipher *tfm);
    struct crypto_alg base;
};
```

**Members**

`encrypt`
:   Function performs an encrypt operation as defined by public key
    algorithm. In case of error, where the dst\_len was insufficient,
    the req->dst\_len will be updated to the size required for the
    operation

`decrypt`
:   Function performs a decrypt operation as defined by public key
    algorithm. In case of error, where the dst\_len was insufficient,
    the req->dst\_len will be updated to the size required for the
    operation

`set_pub_key`
:   Function invokes the algorithm specific set public key
    function, which knows how to decode and interpret
    the BER encoded public key and parameters

`set_priv_key`
:   Function invokes the algorithm specific set private key
    function, which knows how to decode and interpret
    the BER encoded private key and parameters

`max_size`
:   Function returns dest buffer size required for a given key.

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

## Asymmetric Cipher API

The Public Key Cipher API is used with the algorithms of type
CRYPTO\_ALG\_TYPE\_AKCIPHER (listed as type “akcipher” in /proc/crypto)

struct crypto\_akcipher \*crypto\_alloc\_akcipher(const char \*alg\_name, u32 type, u32 mask)
:   allocate AKCIPHER tfm handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    public key algorithm e.g. “rsa”

`u32 type`
:   specifies the type of the algorithm

`u32 mask`
:   specifies the mask for the algorithm

**Description**

Allocate a handle for public key algorithm. The returned `struct
crypto_akcipher` is the handle that is required for any subsequent
API invocation for the public key operations.

**Return**

allocated handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_akcipher(struct crypto\_akcipher \*tfm)
:   free AKCIPHER tfm handle

**Parameters**

`struct crypto_akcipher *tfm`
:   AKCIPHER tfm handle allocated with [`crypto_alloc_akcipher()`](#c.crypto_alloc_akcipher "crypto_alloc_akcipher")

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

unsigned int crypto\_akcipher\_maxsize(struct crypto\_akcipher \*tfm)
:   Get len for output buffer

**Parameters**

`struct crypto_akcipher *tfm`
:   AKCIPHER tfm handle allocated with [`crypto_alloc_akcipher()`](#c.crypto_alloc_akcipher "crypto_alloc_akcipher")

**Description**

Function returns the dest buffer size required for a given key.
Function assumes that the key is already set in the transformation. If this
function is called without a setkey or with a failed setkey, you will end up
in a NULL dereference.

int crypto\_akcipher\_encrypt(struct [akcipher\_request](#c.akcipher_request "akcipher_request") \*req)
:   Invoke public key encrypt operation

**Parameters**

`struct akcipher_request *req`
:   asymmetric key request

**Description**

Function invokes the specific public key encrypt operation for a given
public key algorithm

**Return**

zero on success; error code in case of error

int crypto\_akcipher\_decrypt(struct [akcipher\_request](#c.akcipher_request "akcipher_request") \*req)
:   Invoke public key decrypt operation

**Parameters**

`struct akcipher_request *req`
:   asymmetric key request

**Description**

Function invokes the specific public key decrypt operation for a given
public key algorithm

**Return**

zero on success; error code in case of error

int crypto\_akcipher\_set\_pub\_key(struct crypto\_akcipher \*tfm, const void \*key, unsigned int keylen)
:   Invoke set public key operation

**Parameters**

`struct crypto_akcipher *tfm`
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

int crypto\_akcipher\_set\_priv\_key(struct crypto\_akcipher \*tfm, const void \*key, unsigned int keylen)
:   Invoke set private key operation

**Parameters**

`struct crypto_akcipher *tfm`
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

## Asymmetric Cipher Request Handle

struct [akcipher\_request](#c.akcipher_request "akcipher_request") \*akcipher\_request\_alloc(struct crypto\_akcipher \*tfm, gfp\_t gfp)
:   allocates public key request

**Parameters**

`struct crypto_akcipher *tfm`
:   AKCIPHER tfm handle allocated with [`crypto_alloc_akcipher()`](#c.crypto_alloc_akcipher "crypto_alloc_akcipher")

`gfp_t gfp`
:   allocation flags

**Return**

allocated handle in case of success or NULL in case of an error.

void akcipher\_request\_free(struct [akcipher\_request](#c.akcipher_request "akcipher_request") \*req)
:   zeroize and free public key request

**Parameters**

`struct akcipher_request *req`
:   request to free

void akcipher\_request\_set\_callback(struct [akcipher\_request](#c.akcipher_request "akcipher_request") \*req, u32 flgs, crypto\_completion\_t cmpl, void \*data)
:   Sets an asynchronous callback.

**Parameters**

`struct akcipher_request *req`
:   request that the callback will be set for

`u32 flgs`
:   specify for instance if the operation may backlog

`crypto_completion_t cmpl`
:   callback which will be called

`void *data`
:   private data used by the caller

**Description**

Callback will be called when an asynchronous operation on a given
request is finished.

void akcipher\_request\_set\_crypt(struct [akcipher\_request](#c.akcipher_request "akcipher_request") \*req, struct scatterlist \*src, struct scatterlist \*dst, unsigned int src\_len, unsigned int dst\_len)
:   Sets request parameters

**Parameters**

`struct akcipher_request *req`
:   public key request

`struct scatterlist *src`
:   ptr to input scatter list

`struct scatterlist *dst`
:   ptr to output scatter list

`unsigned int src_len`
:   size of the src input scatter list to be processed

`unsigned int dst_len`
:   size of the dst output scatter list

**Description**

Sets parameters required by crypto operation
