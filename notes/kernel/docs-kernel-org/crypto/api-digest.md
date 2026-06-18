# Message Digest

> 출처(원문): https://docs.kernel.org/crypto/api-digest.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Message Digest

## Message Digest Algorithm Definitions

These data structures define modular message digest algorithm
implementations, managed via `crypto_register_ahash()`,
`crypto_register_shash()`, `crypto_unregister_ahash()` and
`crypto_unregister_shash()`.

struct ahash\_alg
:   asynchronous message digest definition

**Definition**:

```
struct ahash_alg {
    int (*init)(struct ahash_request *req);
    int (*update)(struct ahash_request *req);
    int (*final)(struct ahash_request *req);
    int (*finup)(struct ahash_request *req);
    int (*digest)(struct ahash_request *req);
    int (*export)(struct ahash_request *req, void *out);
    int (*import)(struct ahash_request *req, const void *in);
    int (*export_core)(struct ahash_request *req, void *out);
    int (*import_core)(struct ahash_request *req, const void *in);
    int (*setkey)(struct crypto_ahash *tfm, const u8 *key, unsigned int keylen);
    int (*init_tfm)(struct crypto_ahash *tfm);
    void (*exit_tfm)(struct crypto_ahash *tfm);
    int (*clone_tfm)(struct crypto_ahash *dst, struct crypto_ahash *src);
    struct hash_alg_common halg;
};
```

**Members**

`init`
:   **[mandatory]** Initialize the transformation context. Intended only to initialize the
    state of the HASH transformation at the beginning. This shall fill in
    the internal structures used during the entire duration of the whole
    transformation. No data processing happens at this point. Driver code
    implementation must not use req->result.

`update`
:   **[mandatory]** Push a chunk of data into the driver for transformation. This
    function actually pushes blocks of data from upper layers into the
    driver, which then passes those to the hardware as seen fit. This
    function must not finalize the HASH transformation by calculating the
    final message digest as this only adds more data into the
    transformation. This function shall not modify the transformation
    context, as this function may be called in parallel with the same
    transformation object. Data processing can happen synchronously
    [SHASH] or asynchronously [AHASH] at this point. Driver must not use
    req->result.
    For block-only algorithms, **update** must return the number
    of bytes to store in the API partial block buffer.

`final`
:   **[mandatory]** Retrieve result from the driver. This function finalizes the
    transformation and retrieves the resulting hash from the driver and
    pushes it back to upper layers. No data processing happens at this
    point unless hardware requires it to finish the transformation
    (then the data buffered by the device driver is processed).

`finup`
:   **[optional]** Combination of **update** and **final**. This function is effectively a
    combination of **update** and **final** calls issued in sequence. As some
    hardware cannot do **update** and **final** separately, this callback was
    added to allow such hardware to be used at least by IPsec. Data
    processing can happen synchronously [SHASH] or asynchronously [AHASH]
    at this point.

`digest`
:   Combination of **init** and **update** and **final**. This function
    effectively behaves as the entire chain of operations, **init**,
    **update** and **final** issued in sequence. Just like **finup**, this was
    added for hardware which cannot do even the **finup**, but can only do
    the whole transformation in one run. Data processing can happen
    synchronously [SHASH] or asynchronously [AHASH] at this point.

`export`
:   Export partial state of the transformation. This function dumps the
    entire state of the ongoing transformation into a provided block of
    data so it can be **import** ‘ed back later on. This is useful in case
    you want to save partial result of the transformation after
    processing certain amount of data and reload this partial result
    multiple times later on for multiple re-use. No data processing
    happens at this point. Driver must not use req->result.

`import`
:   Import partial state of the transformation. This function loads the
    entire state of the ongoing transformation from a provided block of
    data so the transformation can continue from this point onward. No
    data processing happens at this point. Driver must not use
    req->result.

`export_core`
:   Export partial state without partial block. Only defined
    for algorithms that are not block-only.

`import_core`
:   Import partial state without partial block. Only defined
    for algorithms that are not block-only.

`setkey`
:   Set optional key used by the hashing algorithm. Intended to push
    optional key used by the hashing algorithm from upper layers into
    the driver. This function can store the key in the transformation
    context or can outright program it into the hardware. In the former
    case, one must be careful to program the key into the hardware at
    appropriate time and one must be careful that .`setkey()` can be
    called multiple times during the existence of the transformation
    object. Not all hashing algorithms do implement this function as it
    is only needed for keyed message digests. SHAx/MDx/CRCx do NOT
    implement this function. HMAC(MDx)/HMAC(SHAx)/CMAC(AES) do implement
    this function. This function must be called before any other of the
    **init**, **update**, **final**, **finup**, **digest** is called. No data
    processing happens at this point.

`init_tfm`
:   Initialize the cryptographic transformation object.
    This function is called only once at the instantiation
    time, right after the transformation context was
    allocated. In case the cryptographic hardware has
    some special requirements which need to be handled
    by software, this function shall check for the precise
    requirement of the transformation and put any software
    fallbacks in place.

`exit_tfm`
:   Deinitialize the cryptographic transformation object.
    This is a counterpart to **init\_tfm**, used to remove
    various changes set in **init\_tfm**.

`clone_tfm`
:   Copy transform into new object, may allocate memory.

`halg`
:   see `struct hash_alg_common`

struct shash\_alg
:   synchronous message digest definition

**Definition**:

```
struct shash_alg {
    int (*init)(struct shash_desc *desc);
    int (*update)(struct shash_desc *desc, const u8 *data, unsigned int len);
    int (*final)(struct shash_desc *desc, u8 *out);
    int (*finup)(struct shash_desc *desc, const u8 *data, unsigned int len, u8 *out);
    int (*digest)(struct shash_desc *desc, const u8 *data, unsigned int len, u8 *out);
    int (*export)(struct shash_desc *desc, void *out);
    int (*import)(struct shash_desc *desc, const void *in);
    int (*export_core)(struct shash_desc *desc, void *out);
    int (*import_core)(struct shash_desc *desc, const void *in);
    int (*setkey)(struct crypto_shash *tfm, const u8 *key, unsigned int keylen);
    int (*init_tfm)(struct crypto_shash *tfm);
    void (*exit_tfm)(struct crypto_shash *tfm);
    int (*clone_tfm)(struct crypto_shash *dst, struct crypto_shash *src);
    unsigned int descsize;
    union {
        struct HASH_ALG_COMMON;
        struct hash_alg_common halg;
    };
};
```

**Members**

`init`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`update`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`final`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`finup`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`digest`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`export`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`import`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`export_core`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`import_core`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`setkey`
:   see [`struct ahash_alg`](#c.ahash_alg "ahash_alg")

`init_tfm`
:   Initialize the cryptographic transformation object.
    This function is called only once at the instantiation
    time, right after the transformation context was
    allocated. In case the cryptographic hardware has
    some special requirements which need to be handled
    by software, this function shall check for the precise
    requirement of the transformation and put any software
    fallbacks in place.

`exit_tfm`
:   Deinitialize the cryptographic transformation object.
    This is a counterpart to **init\_tfm**, used to remove
    various changes set in **init\_tfm**.

`clone_tfm`
:   Copy transform into new object, may allocate memory.

`descsize`
:   Size of the operational state for the message digest. This state
    size is the memory size that needs to be allocated for
    shash\_desc.\_\_ctx

`{unnamed_union}`
:   anonymous

`HASH_ALG_COMMON`
:   see `struct hash_alg_common`

`halg`
:   see `struct hash_alg_common`

## Asynchronous Message Digest API

The asynchronous message digest API is used with the ciphers of type
CRYPTO\_ALG\_TYPE\_AHASH (listed as type “ahash” in /proc/crypto)

The asynchronous cipher operation discussion provided for the
CRYPTO\_ALG\_TYPE\_SKCIPHER API applies here as well.

struct crypto\_ahash \*crypto\_alloc\_ahash(const char \*alg\_name, u32 type, u32 mask)
:   allocate ahash cipher handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    ahash cipher

`u32 type`
:   specifies the type of the cipher

`u32 mask`
:   specifies the mask for the cipher

**Description**

Allocate a cipher handle for an ahash. The returned `struct
crypto_ahash` is the cipher handle that is required for any subsequent
API invocation for that ahash.

**Return**

allocated cipher handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_ahash(struct crypto\_ahash \*tfm)
:   zeroize and free the ahash handle

**Parameters**

`struct crypto_ahash *tfm`
:   cipher handle to be freed

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

unsigned int crypto\_ahash\_digestsize(struct crypto\_ahash \*tfm)
:   obtain message digest size

**Parameters**

`struct crypto_ahash *tfm`
:   cipher handle

**Description**

The size for the message digest created by the message digest cipher
referenced with the cipher handle is returned.

**Return**

message digest size of cipher

unsigned int crypto\_ahash\_statesize(struct crypto\_ahash \*tfm)
:   obtain size of the ahash state

**Parameters**

`struct crypto_ahash *tfm`
:   cipher handle

**Description**

Return the size of the ahash state. With the [`crypto_ahash_export()`](#c.crypto_ahash_export "crypto_ahash_export")
function, the caller can export the state into a buffer whose size is
defined with this function.

**Return**

size of the ahash state

struct crypto\_ahash \*crypto\_ahash\_reqtfm(struct ahash\_request \*req)
:   obtain cipher handle from request

**Parameters**

`struct ahash_request *req`
:   asynchronous request handle that contains the reference to the ahash
    cipher handle

**Description**

Return the ahash cipher handle that is registered with the asynchronous
request handle ahash\_request.

**Return**

ahash cipher handle

unsigned int crypto\_ahash\_reqsize(struct crypto\_ahash \*tfm)
:   obtain size of the request data structure

**Parameters**

`struct crypto_ahash *tfm`
:   cipher handle

**Return**

size of the request data

int crypto\_ahash\_setkey(struct crypto\_ahash \*tfm, const u8 \*key, unsigned int keylen)
:   set key for cipher handle

**Parameters**

`struct crypto_ahash *tfm`
:   cipher handle

`const u8 *key`
:   buffer holding the key

`unsigned int keylen`
:   length of the key in bytes

**Description**

The caller provided key is set for the ahash cipher. The cipher
handle must point to a keyed hash in order for this function to succeed.

**Return**

0 if the setting of the key was successful; < 0 if an error occurred

int crypto\_ahash\_finup(struct ahash\_request \*req)
:   update and finalize message digest

**Parameters**

`struct ahash_request *req`
:   reference to the ahash\_request handle that holds all information
    needed to perform the cipher operation

**Description**

This function is a “short-hand” for the function calls of
crypto\_ahash\_update and crypto\_ahash\_final. The parameters have the same
meaning as discussed for those separate functions.

**Return**

see [`crypto_ahash_final()`](#c.crypto_ahash_final "crypto_ahash_final")

int crypto\_ahash\_final(struct ahash\_request \*req)
:   calculate message digest

**Parameters**

`struct ahash_request *req`
:   reference to the ahash\_request handle that holds all information
    needed to perform the cipher operation

**Description**

Finalize the message digest operation and create the message digest
based on all data added to the cipher handle. The message digest is placed
into the output buffer registered with the ahash\_request handle.

**Return**

0 if the message digest was successfully calculated;
-EINPROGRESS if data is fed into hardware (DMA) or queued for later;
-EBUSY if queue is full and request should be resubmitted later;
other < 0 if an error occurred

int crypto\_ahash\_digest(struct ahash\_request \*req)
:   calculate message digest for a buffer

**Parameters**

`struct ahash_request *req`
:   reference to the ahash\_request handle that holds all information
    needed to perform the cipher operation

**Description**

This function is a “short-hand” for the function calls of crypto\_ahash\_init,
crypto\_ahash\_update and crypto\_ahash\_final. The parameters have the same
meaning as discussed for those separate three functions.

**Return**

see [`crypto_ahash_final()`](#c.crypto_ahash_final "crypto_ahash_final")

int crypto\_ahash\_export(struct ahash\_request \*req, void \*out)
:   extract current message digest state

**Parameters**

`struct ahash_request *req`
:   reference to the ahash\_request handle whose state is exported

`void *out`
:   output buffer of sufficient size that can hold the hash state

**Description**

This function exports the hash state of the ahash\_request handle into the
caller-allocated output buffer out which must have sufficient size (e.g. by
calling [`crypto_ahash_statesize()`](#c.crypto_ahash_statesize "crypto_ahash_statesize")).

**Return**

0 if the export was successful; < 0 if an error occurred

int crypto\_ahash\_import(struct ahash\_request \*req, const void \*in)
:   import message digest state

**Parameters**

`struct ahash_request *req`
:   reference to ahash\_request handle the state is imported into

`const void *in`
:   buffer holding the state

**Description**

This function imports the hash state into the ahash\_request handle from the
input buffer. That buffer should have been generated with the
crypto\_ahash\_export function.

**Return**

0 if the import was successful; < 0 if an error occurred

int crypto\_ahash\_init(struct ahash\_request \*req)
:   (re)initialize message digest handle

**Parameters**

`struct ahash_request *req`
:   ahash\_request handle that already is initialized with all necessary
    data using the ahash\_request\_\* API functions

**Description**

The call (re-)initializes the message digest referenced by the ahash\_request
handle. Any potentially existing state created by previous operations is
discarded.

**Return**

see [`crypto_ahash_final()`](#c.crypto_ahash_final "crypto_ahash_final")

## Asynchronous Hash Request Handle

The `ahash_request` data structure contains all pointers to data
required for the asynchronous cipher operation. This includes the cipher
handle (which can be used by multiple `ahash_request` instances), pointer
to plaintext and the message digest output buffer, asynchronous callback
function, etc. It acts as a handle to the ahash\_request\_\* API calls in a
similar way as ahash handle to the crypto\_ahash\_\* API calls.

void ahash\_request\_set\_tfm(struct ahash\_request \*req, struct crypto\_ahash \*tfm)
:   update cipher handle reference in request

**Parameters**

`struct ahash_request *req`
:   request handle to be modified

`struct crypto_ahash *tfm`
:   cipher handle that shall be added to the request handle

**Description**

Allow the caller to replace the existing ahash handle in the request
data structure with a different one.

struct ahash\_request \*ahash\_request\_alloc(struct crypto\_ahash \*tfm, gfp\_t gfp)
:   allocate request data structure

**Parameters**

`struct crypto_ahash *tfm`
:   cipher handle to be registered with the request

`gfp_t gfp`
:   memory allocation flag that is handed to kmalloc by the API call.

**Description**

Allocate the request data structure that must be used with the ahash
message digest API calls. During
the allocation, the provided ahash handle
is registered in the request data structure.

**Return**

allocated request handle in case of success, or NULL if out of memory

void ahash\_request\_free(struct ahash\_request \*req)
:   zeroize and free the request data structure

**Parameters**

`struct ahash_request *req`
:   request data structure cipher handle to be freed

void ahash\_request\_set\_callback(struct ahash\_request \*req, u32 flags, crypto\_completion\_t compl, void \*data)
:   set asynchronous callback function

**Parameters**

`struct ahash_request *req`
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
    `crypto_async_request` data structure provided to the callback function.

**Description**

This function allows setting the callback function that is triggered once
the cipher operation completes.

The callback function is registered with the `ahash_request` handle and
must comply with the following template:

```
void callback_function(struct crypto_async_request *req, int error)
```

void ahash\_request\_set\_crypt(struct ahash\_request \*req, struct scatterlist \*src, u8 \*result, unsigned int nbytes)
:   set data buffers

**Parameters**

`struct ahash_request *req`
:   ahash\_request handle to be updated

`struct scatterlist *src`
:   source scatter/gather list

`u8 *result`
:   buffer that is filled with the message digest -- the caller must
    ensure that the buffer has sufficient space by, for example, calling
    [`crypto_ahash_digestsize()`](#c.crypto_ahash_digestsize "crypto_ahash_digestsize")

`unsigned int nbytes`
:   number of bytes to process from the source scatter/gather list

**Description**

By using this call, the caller references the source scatter/gather list.
The source scatter/gather list points to the data the message digest is to
be calculated for.

## Synchronous Message Digest API

The synchronous message digest API is used with the ciphers of type
CRYPTO\_ALG\_TYPE\_SHASH (listed as type “shash” in /proc/crypto)

The message digest API is able to maintain state information for the
caller.

The synchronous message digest API can store user-related context in its
shash\_desc request data structure.

struct crypto\_shash \*crypto\_alloc\_shash(const char \*alg\_name, u32 type, u32 mask)
:   allocate message digest handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    message digest cipher

`u32 type`
:   specifies the type of the cipher

`u32 mask`
:   specifies the mask for the cipher

**Description**

Allocate a cipher handle for a message digest. The returned `struct
crypto_shash` is the cipher handle that is required for any subsequent
API invocation for that message digest.

**Return**

allocated cipher handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_shash(struct crypto\_shash \*tfm)
:   zeroize and free the message digest handle

**Parameters**

`struct crypto_shash *tfm`
:   cipher handle to be freed

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

unsigned int crypto\_shash\_blocksize(struct crypto\_shash \*tfm)
:   obtain block size for cipher

**Parameters**

`struct crypto_shash *tfm`
:   cipher handle

**Description**

The block size for the message digest cipher referenced with the cipher
handle is returned.

**Return**

block size of cipher

unsigned int crypto\_shash\_digestsize(struct crypto\_shash \*tfm)
:   obtain message digest size

**Parameters**

`struct crypto_shash *tfm`
:   cipher handle

**Description**

The size for the message digest created by the message digest cipher
referenced with the cipher handle is returned.

**Return**

digest size of cipher

unsigned int crypto\_shash\_descsize(struct crypto\_shash \*tfm)
:   obtain the operational state size

**Parameters**

`struct crypto_shash *tfm`
:   cipher handle

**Description**

The size of the operational state the cipher needs during operation is
returned for the hash referenced with the cipher handle. This size is
required to calculate the memory requirements to allow the caller allocating
sufficient memory for operational state.

The operational state is defined with `struct shash_desc` where the size of
that data structure is to be calculated as
sizeof(`struct shash_desc`) + crypto\_shash\_descsize(alg)

**Return**

size of the operational state

int crypto\_shash\_setkey(struct crypto\_shash \*tfm, const u8 \*key, unsigned int keylen)
:   set key for message digest

**Parameters**

`struct crypto_shash *tfm`
:   cipher handle

`const u8 *key`
:   buffer holding the key

`unsigned int keylen`
:   length of the key in bytes

**Description**

The caller provided key is set for the keyed message digest cipher. The
cipher handle must point to a keyed message digest cipher in order for this
function to succeed.

**Context**

Softirq or process context.

**Return**

0 if the setting of the key was successful; < 0 if an error occurred

int crypto\_shash\_digest(struct shash\_desc \*desc, const u8 \*data, unsigned int len, u8 \*out)
:   calculate message digest for buffer

**Parameters**

`struct shash_desc *desc`
:   see [`crypto_shash_final()`](#c.crypto_shash_final "crypto_shash_final")

`const u8 *data`
:   see [`crypto_shash_update()`](#c.crypto_shash_update "crypto_shash_update")

`unsigned int len`
:   see [`crypto_shash_update()`](#c.crypto_shash_update "crypto_shash_update")

`u8 *out`
:   see [`crypto_shash_final()`](#c.crypto_shash_final "crypto_shash_final")

**Description**

This function is a “short-hand” for the function calls of crypto\_shash\_init,
crypto\_shash\_update and crypto\_shash\_final. The parameters have the same
meaning as discussed for those separate three functions.

**Context**

Softirq or process context.

**Return**

0 if the message digest creation was successful; < 0 if an error
occurred

int crypto\_shash\_export(struct shash\_desc \*desc, void \*out)
:   extract operational state for message digest

**Parameters**

`struct shash_desc *desc`
:   reference to the operational state handle whose state is exported

`void *out`
:   output buffer of sufficient size that can hold the hash state

**Description**

This function exports the hash state of the operational state handle into the
caller-allocated output buffer out which must have sufficient size (e.g. by
calling crypto\_shash\_descsize).

**Context**

Softirq or process context.

**Return**

0 if the export creation was successful; < 0 if an error occurred

int crypto\_shash\_import(struct shash\_desc \*desc, const void \*in)
:   import operational state

**Parameters**

`struct shash_desc *desc`
:   reference to the operational state handle the state imported into

`const void *in`
:   buffer holding the state

**Description**

This function imports the hash state into the operational state handle from
the input buffer. That buffer should have been generated with the
crypto\_ahash\_export function.

**Context**

Softirq or process context.

**Return**

0 if the import was successful; < 0 if an error occurred

int crypto\_shash\_init(struct shash\_desc \*desc)
:   (re)initialize message digest

**Parameters**

`struct shash_desc *desc`
:   operational state handle that is already filled

**Description**

The call (re-)initializes the message digest referenced by the
operational state handle. Any potentially existing state created by
previous operations is discarded.

**Context**

Softirq or process context.

**Return**

0 if the message digest initialization was successful; < 0 if an
error occurred

int crypto\_shash\_finup(struct shash\_desc \*desc, const u8 \*data, unsigned int len, u8 \*out)
:   calculate message digest of buffer

**Parameters**

`struct shash_desc *desc`
:   see [`crypto_shash_final()`](#c.crypto_shash_final "crypto_shash_final")

`const u8 *data`
:   see [`crypto_shash_update()`](#c.crypto_shash_update "crypto_shash_update")

`unsigned int len`
:   see [`crypto_shash_update()`](#c.crypto_shash_update "crypto_shash_update")

`u8 *out`
:   see [`crypto_shash_final()`](#c.crypto_shash_final "crypto_shash_final")

**Description**

This function is a “short-hand” for the function calls of
crypto\_shash\_update and crypto\_shash\_final. The parameters have the same
meaning as discussed for those separate functions.

**Context**

Softirq or process context.

**Return**

0 if the message digest creation was successful; < 0 if an error
occurred

int crypto\_shash\_update(struct shash\_desc \*desc, const u8 \*data, unsigned int len)
:   add data to message digest for processing

**Parameters**

`struct shash_desc *desc`
:   operational state handle that is already initialized

`const u8 *data`
:   input data to be added to the message digest

`unsigned int len`
:   length of the input data

**Description**

Updates the message digest state of the operational state handle.

**Context**

Softirq or process context.

**Return**

0 if the message digest update was successful; < 0 if an error
occurred

int crypto\_shash\_final(struct shash\_desc \*desc, u8 \*out)
:   calculate message digest

**Parameters**

`struct shash_desc *desc`
:   operational state handle that is already filled with data

`u8 *out`
:   output buffer filled with the message digest

**Description**

Finalize the message digest operation and create the message digest
based on all data added to the cipher handle. The message digest is placed
into the output buffer. The caller must ensure that the output buffer is
large enough by using crypto\_shash\_digestsize.

**Context**

Softirq or process context.

**Return**

0 if the message digest creation was successful; < 0 if an error
occurred
