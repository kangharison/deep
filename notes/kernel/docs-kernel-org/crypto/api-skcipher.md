# Symmetric Key Cipher

> 출처(원문): https://docs.kernel.org/crypto/api-skcipher.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Symmetric Key Cipher

## Block Cipher Algorithm Definitions

These data structures define modular crypto algorithm implementations,
managed via `crypto_register_alg()` and `crypto_unregister_alg()`.

struct cipher\_alg
:   single-block symmetric ciphers definition

**Definition**:

```
struct cipher_alg {
    unsigned int cia_min_keysize;
    unsigned int cia_max_keysize;
    int (*cia_setkey)(struct crypto_tfm *tfm, const u8 *key, unsigned int keylen);
    void (*cia_encrypt)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);
    void (*cia_decrypt)(struct crypto_tfm *tfm, u8 *dst, const u8 *src);
};
```

**Members**

`cia_min_keysize`
:   Minimum key size supported by the transformation. This is
    the smallest key length supported by this transformation
    algorithm. This must be set to one of the pre-defined
    values as this is not hardware specific. Possible values
    for this field can be found via git grep “\_MIN\_KEY\_SIZE”
    include/crypto/

`cia_max_keysize`
:   Maximum key size supported by the transformation. This is
    the largest key length supported by this transformation
    algorithm. This must be set to one of the pre-defined values
    as this is not hardware specific. Possible values for this
    field can be found via git grep “\_MAX\_KEY\_SIZE”
    include/crypto/

`cia_setkey`
:   Set key for the transformation. This function is used to either
    program a supplied key into the hardware or store the key in the
    transformation context for programming it later. Note that this
    function does modify the transformation context. This function
    can be called multiple times during the existence of the
    transformation object, so one must make sure the key is properly
    reprogrammed into the hardware. This function is also
    responsible for checking the key length for validity.

`cia_encrypt`
:   Encrypt a single block. This function is used to encrypt a
    single block of data, which must be **cra\_blocksize** big. This
    always operates on a full **cra\_blocksize** and it is not possible
    to encrypt a block of smaller size. The supplied buffers must
    therefore also be at least of **cra\_blocksize** size. Both the
    input and output buffers are always aligned to **cra\_alignmask**.
    In case either of the input or output buffer supplied by user
    of the crypto API is not aligned to **cra\_alignmask**, the crypto
    API will re-align the buffers. The re-alignment means that a
    new buffer will be allocated, the data will be copied into the
    new buffer, then the processing will happen on the new buffer,
    then the data will be copied back into the original buffer and
    finally the new buffer will be freed. In case a software
    fallback was put in place in the **cra\_init** call, this function
    might need to use the fallback if the algorithm doesn’t support
    all of the key sizes. In case the key was stored in
    transformation context, the key might need to be re-programmed
    into the hardware in this function. This function shall not
    modify the transformation context, as this function may be
    called in parallel with the same transformation object.

`cia_decrypt`
:   Decrypt a single block. This is a reverse counterpart to
    **cia\_encrypt**, and the conditions are exactly the same.

**Description**

All fields are mandatory and must be filled.

struct crypto\_alg
:   definition of a cryptograpic cipher algorithm

**Definition**:

```
struct crypto_alg {
    struct list_head cra_list;
    struct list_head cra_users;
    u32 cra_flags;
    unsigned int cra_blocksize;
    unsigned int cra_ctxsize;
    unsigned int cra_alignmask;
    unsigned int cra_reqsize;
    int cra_priority;
    refcount_t cra_refcnt;
    char cra_name[CRYPTO_MAX_ALG_NAME];
    char cra_driver_name[CRYPTO_MAX_ALG_NAME];
    const struct crypto_type *cra_type;
    union {
        struct cipher_alg cipher;
    } cra_u;
    int (*cra_init)(struct crypto_tfm *tfm);
    void (*cra_exit)(struct crypto_tfm *tfm);
    void (*cra_destroy)(struct crypto_alg *alg);
    struct module *cra_module;
};
```

**Members**

`cra_list`
:   internally used

`cra_users`
:   internally used

`cra_flags`
:   Flags describing this transformation. See include/linux/crypto.h
    CRYPTO\_ALG\_\* flags for the flags which go in here. Those are
    used for fine-tuning the description of the transformation
    algorithm.

`cra_blocksize`
:   Minimum block size of this transformation. The size in bytes
    of the smallest possible unit which can be transformed with
    this algorithm. The users must respect this value.
    In case of HASH transformation, it is possible for a smaller
    block than **cra\_blocksize** to be passed to the crypto API for
    transformation, in case of any other transformation type, an
    error will be returned upon any attempt to transform smaller
    than **cra\_blocksize** chunks.

`cra_ctxsize`
:   Size of the operational context of the transformation. This
    value informs the kernel crypto API about the memory size
    needed to be allocated for the transformation context.

`cra_alignmask`
:   For cipher, skcipher, lskcipher, and aead algorithms this is
    1 less than the alignment, in bytes, that the algorithm
    implementation requires for input and output buffers. When
    the crypto API is invoked with buffers that are not aligned
    to this alignment, the crypto API automatically utilizes
    appropriately aligned temporary buffers to comply with what
    the algorithm needs. (For scatterlists this happens only if
    the algorithm uses the skcipher\_walk helper functions.) This
    misalignment handling carries a performance penalty, so it is
    preferred that algorithms do not set a nonzero alignmask.
    Also, crypto API users may wish to allocate buffers aligned
    to the alignmask of the algorithm being used, in order to
    avoid the API having to realign them. Note: the alignmask is
    not supported for hash algorithms and is always 0 for them.

`cra_reqsize`
:   Size of the request context for this algorithm.

`cra_priority`
:   Priority of this transformation implementation. In case
    multiple transformations with same **cra\_name** are available to
    the Crypto API, the kernel will use the one with highest
    **cra\_priority**.

`cra_refcnt`
:   internally used

`cra_name`
:   Generic name (usable by multiple implementations) of the
    transformation algorithm. This is the name of the transformation
    itself. This field is used by the kernel when looking up the
    providers of particular transformation.

`cra_driver_name`
:   Unique name of the transformation provider. This is the
    name of the provider of the transformation. This can be any
    arbitrary value, but in the usual case, this contains the
    name of the chip or provider and the name of the
    transformation algorithm.

`cra_type`
:   Type of the cryptographic transformation. This is a pointer to
    `struct crypto_type`, which implements callbacks common for all
    transformation types. There are multiple options, such as
    `crypto_skcipher_type`, `crypto_ahash_type`, `crypto_rng_type`.
    This field might be empty. In that case, there are no common
    callbacks. This is the case for: cipher.

`cra_u`
:   Callbacks implementing the transformation. This is a `union of`
    multiple structures. Depending on the type of transformation selected
    by **cra\_type** and **cra\_flags** above, the associated structure must be
    filled with callbacks. This field might be empty. This is the case
    for ahash, shash.

`cra_u.cipher`
:   Union member which contains a single-block symmetric cipher
    definition. See **struct** **cipher\_alg**.

`cra_init`
:   Deprecated, do not use.

`cra_exit`
:   Deprecated, do not use.

`cra_destroy`
:   internally used

`cra_module`
:   Owner of this transformation implementation. Set to THIS\_MODULE

**Description**

The [`struct crypto_alg`](#c.crypto_alg "crypto_alg") describes a generic Crypto API algorithm and is common
for all of the transformations. Any variable not documented here shall not
be used by a cipher implementation as it is internal to the Crypto API.

## Symmetric Key Cipher API

Symmetric key cipher API is used with the ciphers of type
CRYPTO\_ALG\_TYPE\_SKCIPHER (listed as type “skcipher” in /proc/crypto).

Asynchronous cipher operations imply that the function invocation for a
cipher request returns immediately before the completion of the operation.
The cipher request is scheduled as a separate kernel thread and therefore
load-balanced on the different CPUs via the process scheduler. To allow
the kernel crypto API to inform the caller about the completion of a cipher
request, the caller must provide a callback function. That function is
invoked with the cipher handle when the request completes.

To support the asynchronous operation, additional information than just the
cipher handle must be supplied to the kernel crypto API. That additional
information is given by filling in the skcipher\_request data structure.

For the symmetric key cipher API, the state is maintained with the tfm
cipher handle. A single tfm can be used across multiple calls and in
parallel. For asynchronous block cipher calls, context data supplied and
only used by the caller can be referenced the request data structure in
addition to the IV used for the cipher request. The maintenance of such
state information would be important for a crypto driver implementer to
have, because when calling the callback function upon completion of the
cipher operation, that callback function may need some information about
which operation just finished if it invoked multiple in parallel. This
state information is unused by the kernel crypto API.

struct crypto\_skcipher \*crypto\_alloc\_skcipher(const char \*alg\_name, u32 type, u32 mask)
:   allocate symmetric key cipher handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    skcipher cipher

`u32 type`
:   specifies the type of the cipher

`u32 mask`
:   specifies the mask for the cipher

**Description**

Allocate a cipher handle for an skcipher. The returned `struct
crypto_skcipher` is the cipher handle that is required for any subsequent
API invocation for that skcipher.

**Return**

allocated cipher handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_skcipher(struct crypto\_skcipher \*tfm)
:   zeroize and free cipher handle

**Parameters**

`struct crypto_skcipher *tfm`
:   cipher handle to be freed

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

int crypto\_has\_skcipher(const char \*alg\_name, u32 type, u32 mask)
:   Search for the availability of an skcipher.

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    skcipher

`u32 type`
:   specifies the type of the skcipher

`u32 mask`
:   specifies the mask for the skcipher

**Return**

true when the skcipher is known to the kernel crypto API; false
otherwise

unsigned int crypto\_skcipher\_ivsize(struct crypto\_skcipher \*tfm)
:   obtain IV size

**Parameters**

`struct crypto_skcipher *tfm`
:   cipher handle

**Description**

The size of the IV for the skcipher referenced by the cipher handle is
returned. This IV size may be zero if the cipher does not need an IV.

**Return**

IV size in bytes

unsigned int crypto\_skcipher\_blocksize(struct crypto\_skcipher \*tfm)
:   obtain block size of cipher

**Parameters**

`struct crypto_skcipher *tfm`
:   cipher handle

**Description**

The block size for the skcipher referenced with the cipher handle is
returned. The caller may use that information to allocate appropriate
memory for the data returned by the encryption or decryption operation

**Return**

block size of cipher

int crypto\_skcipher\_setkey(struct crypto\_skcipher \*tfm, const u8 \*key, unsigned int keylen)
:   set key for cipher

**Parameters**

`struct crypto_skcipher *tfm`
:   cipher handle

`const u8 *key`
:   buffer holding the key

`unsigned int keylen`
:   length of the key in bytes

**Description**

The caller provided key is set for the skcipher referenced by the cipher
handle.

Note, the key length determines the cipher type. Many block ciphers implement
different cipher modes depending on the key size, such as AES-128 vs AES-192
vs. AES-256. When providing a 16 byte key for an AES cipher handle, AES-128
is performed.

**Return**

0 if the setting of the key was successful; < 0 if an error occurred

struct crypto\_skcipher \*crypto\_skcipher\_reqtfm(struct skcipher\_request \*req)
:   obtain cipher handle from request

**Parameters**

`struct skcipher_request *req`
:   skcipher\_request out of which the cipher handle is to be obtained

**Description**

Return the crypto\_skcipher handle when furnishing an skcipher\_request
data structure.

**Return**

crypto\_skcipher handle

int crypto\_skcipher\_encrypt(struct skcipher\_request \*req)
:   encrypt plaintext

**Parameters**

`struct skcipher_request *req`
:   reference to the skcipher\_request handle that holds all information
    needed to perform the cipher operation

**Description**

Encrypt plaintext data using the skcipher\_request handle. That data
structure and how it is filled with data is discussed with the
skcipher\_request\_\* functions.

**Return**

0 if the cipher operation was successful; < 0 if an error occurred

int crypto\_skcipher\_decrypt(struct skcipher\_request \*req)
:   decrypt ciphertext

**Parameters**

`struct skcipher_request *req`
:   reference to the skcipher\_request handle that holds all information
    needed to perform the cipher operation

**Description**

Decrypt ciphertext data using the skcipher\_request handle. That data
structure and how it is filled with data is discussed with the
skcipher\_request\_\* functions.

**Return**

0 if the cipher operation was successful; < 0 if an error occurred

## Symmetric Key Cipher Request Handle

The skcipher\_request data structure contains all pointers to data
required for the symmetric key cipher operation. This includes the cipher
handle (which can be used by multiple skcipher\_request instances), pointer
to plaintext and ciphertext, asynchronous callback function, etc. It acts
as a handle to the skcipher\_request\_\* API calls in a similar way as
skcipher handle to the crypto\_skcipher\_\* API calls.

unsigned int crypto\_skcipher\_reqsize(struct crypto\_skcipher \*tfm)
:   obtain size of the request data structure

**Parameters**

`struct crypto_skcipher *tfm`
:   cipher handle

**Return**

number of bytes

void skcipher\_request\_set\_tfm(struct skcipher\_request \*req, struct crypto\_skcipher \*tfm)
:   update cipher handle reference in request

**Parameters**

`struct skcipher_request *req`
:   request handle to be modified

`struct crypto_skcipher *tfm`
:   cipher handle that shall be added to the request handle

**Description**

Allow the caller to replace the existing skcipher handle in the request
data structure with a different one.

struct skcipher\_request \*skcipher\_request\_alloc(struct crypto\_skcipher \*tfm, gfp\_t gfp)
:   allocate request data structure

**Parameters**

`struct crypto_skcipher *tfm`
:   cipher handle to be registered with the request

`gfp_t gfp`
:   memory allocation flag that is handed to kmalloc by the API call.

**Description**

Allocate the request data structure that must be used with the skcipher
encrypt and decrypt API calls. During the allocation, the provided skcipher
handle is registered in the request data structure.

**Return**

allocated request handle in case of success, or NULL if out of memory

void skcipher\_request\_free(struct skcipher\_request \*req)
:   zeroize and free request data structure

**Parameters**

`struct skcipher_request *req`
:   request data structure cipher handle to be freed

void skcipher\_request\_set\_callback(struct skcipher\_request \*req, u32 flags, crypto\_completion\_t compl, void \*data)
:   set asynchronous callback function

**Parameters**

`struct skcipher_request *req`
:   request handle

`u32 flags`
:   specify zero or an ORing of the flags
    CRYPTO\_TFM\_REQ\_MAY\_BACKLOG the request queue may back log and
    increase the wait queue beyond the initial maximum size;
    CRYPTO\_TFM\_REQ\_MAY\_SLEEP the request processing may sleep

`crypto_completion_t compl`
:   callback function pointer to be registered with the request handle

`void *data`
:   The data pointer refers to memory that is not used by the kernel
    crypto API, but provided to the callback function for it to use. Here,
    the caller can provide a reference to memory the callback function can
    operate on. As the callback function is invoked asynchronously to the
    related functionality, it may need to access data structures of the
    related functionality which can be referenced using this pointer. The
    callback function can access the memory via the “data” field in the
    crypto\_async\_request data structure provided to the callback function.

**Description**

This function allows setting the callback function that is triggered once the
cipher operation completes.

The callback function is registered with the skcipher\_request handle and
must comply with the following template:

```
void callback_function(struct crypto_async_request *req, int error)
```

void skcipher\_request\_set\_crypt(struct skcipher\_request \*req, struct scatterlist \*src, struct scatterlist \*dst, unsigned int cryptlen, void \*iv)
:   set data buffers

**Parameters**

`struct skcipher_request *req`
:   request handle

`struct scatterlist *src`
:   source scatter / gather list

`struct scatterlist *dst`
:   destination scatter / gather list

`unsigned int cryptlen`
:   number of bytes to process from **src**

`void *iv`
:   IV for the cipher operation which must comply with the IV size defined
    by crypto\_skcipher\_ivsize

**Description**

This function allows setting of the source data and destination data
scatter / gather lists.

For encryption, the source is treated as the plaintext and the
destination is the ciphertext. For a decryption operation, the use is
reversed - the source is the ciphertext and the destination is the plaintext.

## Single Block Cipher API

The single block cipher API is used with the ciphers of type
CRYPTO\_ALG\_TYPE\_CIPHER (listed as type “cipher” in /proc/crypto).

Using the single block cipher API calls, operations with the basic cipher
primitive can be implemented. These cipher primitives exclude any block
chaining operations including IV handling.

The purpose of this single block cipher API is to support the implementation
of templates or other concepts that only need to perform the cipher operation
on one block at a time. Templates invoke the underlying cipher primitive
block-wise and process either the input or the output data of these cipher
operations.

struct crypto\_cipher \*crypto\_alloc\_cipher(const char \*alg\_name, u32 type, u32 mask)
:   allocate single block cipher handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    single block cipher

`u32 type`
:   specifies the type of the cipher

`u32 mask`
:   specifies the mask for the cipher

**Description**

Allocate a cipher handle for a single block cipher. The returned `struct
crypto_cipher` is the cipher handle that is required for any subsequent API
invocation for that single block cipher.

**Return**

allocated cipher handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_cipher(struct crypto\_cipher \*tfm)
:   zeroize and free the single block cipher handle

**Parameters**

`struct crypto_cipher *tfm`
:   cipher handle to be freed

int crypto\_has\_cipher(const char \*alg\_name, u32 type, u32 mask)
:   Search for the availability of a single block cipher

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    single block cipher

`u32 type`
:   specifies the type of the cipher

`u32 mask`
:   specifies the mask for the cipher

**Return**

true when the single block cipher is known to the kernel crypto API;
false otherwise

unsigned int crypto\_cipher\_blocksize(struct crypto\_cipher \*tfm)
:   obtain block size for cipher

**Parameters**

`struct crypto_cipher *tfm`
:   cipher handle

**Description**

The block size for the single block cipher referenced with the cipher handle
tfm is returned. The caller may use that information to allocate appropriate
memory for the data returned by the encryption or decryption operation

**Return**

block size of cipher

int crypto\_cipher\_setkey(struct crypto\_cipher \*tfm, const u8 \*key, unsigned int keylen)
:   set key for cipher

**Parameters**

`struct crypto_cipher *tfm`
:   cipher handle

`const u8 *key`
:   buffer holding the key

`unsigned int keylen`
:   length of the key in bytes

**Description**

The caller provided key is set for the single block cipher referenced by the
cipher handle.

Note, the key length determines the cipher type. Many block ciphers implement
different cipher modes depending on the key size, such as AES-128 vs AES-192
vs. AES-256. When providing a 16 byte key for an AES cipher handle, AES-128
is performed.

**Return**

0 if the setting of the key was successful; < 0 if an error occurred

void crypto\_cipher\_encrypt\_one(struct crypto\_cipher \*tfm, u8 \*dst, const u8 \*src)
:   encrypt one block of plaintext

**Parameters**

`struct crypto_cipher *tfm`
:   cipher handle

`u8 *dst`
:   points to the buffer that will be filled with the ciphertext

`const u8 *src`
:   buffer holding the plaintext to be encrypted

**Description**

Invoke the encryption operation of one block. The caller must ensure that
the plaintext and ciphertext buffers are at least one block in size.

void crypto\_cipher\_decrypt\_one(struct crypto\_cipher \*tfm, u8 \*dst, const u8 \*src)
:   decrypt one block of ciphertext

**Parameters**

`struct crypto_cipher *tfm`
:   cipher handle

`u8 *dst`
:   points to the buffer that will be filled with the plaintext

`const u8 *src`
:   buffer holding the ciphertext to be decrypted

**Description**

Invoke the decryption operation of one block. The caller must ensure that
the plaintext and ciphertext buffers are at least one block in size.
