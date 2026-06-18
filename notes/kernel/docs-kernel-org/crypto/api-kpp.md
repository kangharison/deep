# Key-agreement Protocol Primitives (KPP)

> 출처(원문): https://docs.kernel.org/crypto/api-kpp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Key-agreement Protocol Primitives (KPP)

## Key-agreement Protocol Primitives (KPP) Cipher Algorithm Definitions

struct kpp\_request

**Definition**:

```
struct kpp_request {
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
:   Size of the output buffer. It needs to be at least
    as big as the expected result depending on the operation
    After operation it will be updated with the actual size of the
    result. In case of error where the dst sgl size was insufficient,
    it will be updated to the size required for the operation.

`__ctx`
:   Start of private context data

struct crypto\_kpp
:   user-instantiated object which encapsulate algorithms and core processing logic

**Definition**:

```
struct crypto_kpp {
    unsigned int reqsize;
    struct crypto_tfm base;
};
```

**Members**

`reqsize`
:   Request context size required by algorithm
    implementation

`base`
:   Common crypto API algorithm data structure

struct kpp\_alg
:   generic key-agreement protocol primitives

**Definition**:

```
struct kpp_alg {
    int (*set_secret)(struct crypto_kpp *tfm, const void *buffer, unsigned int len);
    int (*generate_public_key)(struct kpp_request *req);
    int (*compute_shared_secret)(struct kpp_request *req);
    unsigned int (*max_size)(struct crypto_kpp *tfm);
    int (*init)(struct crypto_kpp *tfm);
    void (*exit)(struct crypto_kpp *tfm);
    struct crypto_alg base;
};
```

**Members**

`set_secret`
:   Function invokes the protocol specific function to
    store the secret private key along with parameters.
    The implementation knows how to decode the buffer

`generate_public_key`
:   Function generate the public key to be sent to the
    counterpart. In case of error, where output is not big
    enough req->dst\_len will be updated to the size
    required

`compute_shared_secret`
:   Function compute the shared secret as defined by
    the algorithm. The result is given back to the user.
    In case of error, where output is not big enough,
    req->dst\_len will be updated to the size required

`max_size`
:   Function returns the size of the output buffer

`init`
:   Initialize the object. This is called only once at
    instantiation time. In case the cryptographic hardware
    needs to be initialized. Software fallback should be
    put in place here.

`exit`
:   Undo everything **init** did.

`base`
:   Common crypto API algorithm data structure

struct kpp\_secret
:   small header for packing secret buffer

**Definition**:

```
struct kpp_secret {
    unsigned short type;
    unsigned short len;
};
```

**Members**

`type`
:   define type of secret. Each kpp type will define its own

`len`
:   specify the len of the secret, include the header, that
    follows the struct

## Key-agreement Protocol Primitives (KPP) Cipher API

The KPP API is used with the algorithm type
CRYPTO\_ALG\_TYPE\_KPP (listed as type “kpp” in /proc/crypto)

struct [crypto\_kpp](#c.crypto_kpp "crypto_kpp") \*crypto\_alloc\_kpp(const char \*alg\_name, u32 type, u32 mask)
:   allocate KPP tfm handle

**Parameters**

`const char *alg_name`
:   is the name of the kpp algorithm (e.g. “dh”, “ecdh”)

`u32 type`
:   specifies the type of the algorithm

`u32 mask`
:   specifies the mask for the algorithm

**Description**

Allocate a handle for kpp algorithm. The returned [`struct crypto_kpp`](#c.crypto_kpp "crypto_kpp")
is required for any following API invocation

**Return**

allocated handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case of
an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

void crypto\_free\_kpp(struct [crypto\_kpp](#c.crypto_kpp "crypto_kpp") \*tfm)
:   free KPP tfm handle

**Parameters**

`struct crypto_kpp *tfm`
:   KPP tfm handle allocated with [`crypto_alloc_kpp()`](#c.crypto_alloc_kpp "crypto_alloc_kpp")

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

int crypto\_kpp\_set\_secret(struct [crypto\_kpp](#c.crypto_kpp "crypto_kpp") \*tfm, const void \*buffer, unsigned int len)
:   Invoke kpp operation

**Parameters**

`struct crypto_kpp *tfm`
:   tfm handle

`const void *buffer`
:   Buffer holding the packet representation of the private
    key. The structure of the packet key depends on the particular
    KPP implementation. Packing and unpacking helpers are provided
    for ECDH and DH (see the respective header files for those
    implementations).

`unsigned int len`
:   Length of the packet private key buffer.

**Description**

Function invokes the specific kpp operation for a given alg.

**Return**

zero on success; error code in case of error

int crypto\_kpp\_generate\_public\_key(struct [kpp\_request](#c.kpp_request "kpp_request") \*req)
:   Invoke kpp operation

**Parameters**

`struct kpp_request *req`
:   kpp key request

**Description**

Function invokes the specific kpp operation for generating the public part
for a given kpp algorithm.

To generate a private key, the caller should use a random number generator.
The output of the requested length serves as the private key.

**Return**

zero on success; error code in case of error

int crypto\_kpp\_compute\_shared\_secret(struct [kpp\_request](#c.kpp_request "kpp_request") \*req)
:   Invoke kpp operation

**Parameters**

`struct kpp_request *req`
:   kpp key request

**Description**

Function invokes the specific kpp operation for computing the shared secret
for a given kpp algorithm.

**Return**

zero on success; error code in case of error

unsigned int crypto\_kpp\_maxsize(struct [crypto\_kpp](#c.crypto_kpp "crypto_kpp") \*tfm)
:   Get len for output buffer

**Parameters**

`struct crypto_kpp *tfm`
:   KPP tfm handle allocated with [`crypto_alloc_kpp()`](#c.crypto_alloc_kpp "crypto_alloc_kpp")

**Description**

Function returns the output buffer size required for a given key.
Function assumes that the key is already set in the transformation. If this
function is called without a setkey or with a failed setkey, you will end up
in a NULL dereference.

## Key-agreement Protocol Primitives (KPP) Cipher Request Handle

struct [kpp\_request](#c.kpp_request "kpp_request") \*kpp\_request\_alloc(struct [crypto\_kpp](#c.crypto_kpp "crypto_kpp") \*tfm, gfp\_t gfp)
:   allocates kpp request

**Parameters**

`struct crypto_kpp *tfm`
:   KPP tfm handle allocated with [`crypto_alloc_kpp()`](#c.crypto_alloc_kpp "crypto_alloc_kpp")

`gfp_t gfp`
:   allocation flags

**Return**

allocated handle in case of success or NULL in case of an error.

void kpp\_request\_free(struct [kpp\_request](#c.kpp_request "kpp_request") \*req)
:   zeroize and free kpp request

**Parameters**

`struct kpp_request *req`
:   request to free

void kpp\_request\_set\_callback(struct [kpp\_request](#c.kpp_request "kpp_request") \*req, u32 flgs, crypto\_completion\_t cmpl, void \*data)
:   Sets an asynchronous callback.

**Parameters**

`struct kpp_request *req`
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

void kpp\_request\_set\_input(struct [kpp\_request](#c.kpp_request "kpp_request") \*req, struct scatterlist \*input, unsigned int input\_len)
:   Sets input buffer

**Parameters**

`struct kpp_request *req`
:   kpp request

`struct scatterlist *input`
:   ptr to input scatter list

`unsigned int input_len`
:   size of the input scatter list

**Description**

Sets parameters required by generate\_public\_key

void kpp\_request\_set\_output(struct [kpp\_request](#c.kpp_request "kpp_request") \*req, struct scatterlist \*output, unsigned int output\_len)
:   Sets output buffer

**Parameters**

`struct kpp_request *req`
:   kpp request

`struct scatterlist *output`
:   ptr to output scatter list

`unsigned int output_len`
:   size of the output scatter list

**Description**

Sets parameters required by kpp operation

## ECDH Helper Functions

To use ECDH with the KPP cipher API, the following data structure and
functions should be used.

The ECC curves known to the ECDH implementation are specified in this
header file.

To use ECDH with KPP, the following functions should be used to operate on
an ECDH private key. The packet private key that can be set with
the KPP API function call of crypto\_kpp\_set\_secret.

struct ecdh
:   define an ECDH private key

**Definition**:

```
struct ecdh {
    char *key;
    unsigned short key_size;
};
```

**Members**

`key`
:   Private ECDH key

`key_size`
:   Size of the private ECDH key

unsigned int crypto\_ecdh\_key\_len(const struct [ecdh](#c.ecdh "ecdh") \*params)
:   Obtain the size of the private ECDH key

**Parameters**

`const struct ecdh *params`
:   private ECDH key

**Description**

This function returns the packet ECDH key size. A caller can use that
with the provided ECDH private key reference to obtain the required
memory size to hold a packet key.

**Return**

size of the key in bytes

int crypto\_ecdh\_encode\_key(char \*buf, unsigned int len, const struct [ecdh](#c.ecdh "ecdh") \*p)
:   encode the private key

**Parameters**

`char *buf`
:   Buffer allocated by the caller to hold the packet ECDH
    private key. The buffer should be at least crypto\_ecdh\_key\_len
    bytes in size.

`unsigned int len`
:   Length of the packet private key buffer

`const struct ecdh *p`
:   Buffer with the caller-specified private key

**Description**

The ECDH implementations operate on a packet representation of the private
key.

**Return**

-EINVAL if buffer has insufficient size, 0 on success

int crypto\_ecdh\_decode\_key(const char \*buf, unsigned int len, struct [ecdh](#c.ecdh "ecdh") \*p)
:   decode a private key

**Parameters**

`const char *buf`
:   Buffer holding a packet key that should be decoded

`unsigned int len`
:   Length of the packet private key buffer

`struct ecdh *p`
:   Buffer allocated by the caller that is filled with the
    unpacked ECDH private key.

**Description**

The unpacking obtains the private key by pointing **p** to the correct location
in **buf**. Thus, both pointers refer to the same memory.

**Return**

-EINVAL if buffer has insufficient size, 0 on success

## DH Helper Functions

To use DH with the KPP cipher API, the following data structure and
functions should be used.

To use DH with KPP, the following functions should be used to operate on
a DH private key. The packet private key that can be set with
the KPP API function call of crypto\_kpp\_set\_secret.

struct dh
:   define a DH private key

**Definition**:

```
struct dh {
    const void *key;
    const void *p;
    const void *g;
    unsigned int key_size;
    unsigned int p_size;
    unsigned int g_size;
};
```

**Members**

`key`
:   Private DH key

`p`
:   Diffie-Hellman parameter P

`g`
:   Diffie-Hellman generator G

`key_size`
:   Size of the private DH key

`p_size`
:   Size of DH parameter P

`g_size`
:   Size of DH generator G

unsigned int crypto\_dh\_key\_len(const struct [dh](#c.dh "dh") \*params)
:   Obtain the size of the private DH key

**Parameters**

`const struct dh *params`
:   private DH key

**Description**

This function returns the packet DH key size. A caller can use that
with the provided DH private key reference to obtain the required
memory size to hold a packet key.

**Return**

size of the key in bytes

int crypto\_dh\_encode\_key(char \*buf, unsigned int len, const struct [dh](#c.dh "dh") \*params)
:   encode the private key

**Parameters**

`char *buf`
:   Buffer allocated by the caller to hold the packet DH
    private key. The buffer should be at least crypto\_dh\_key\_len
    bytes in size.

`unsigned int len`
:   Length of the packet private key buffer

`const struct dh *params`
:   Buffer with the caller-specified private key

**Description**

The DH implementations operate on a packet representation of the private
key.

**Return**

-EINVAL if buffer has insufficient size, 0 on success

int crypto\_dh\_decode\_key(const char \*buf, unsigned int len, struct [dh](#c.dh "dh") \*params)
:   decode a private key

**Parameters**

`const char *buf`
:   Buffer holding a packet key that should be decoded

`unsigned int len`
:   Length of the packet private key buffer

`struct dh *params`
:   Buffer allocated by the caller that is filled with the
    unpacked DH private key.

**Description**

The unpacking obtains the private key by pointing **p** to the correct location
in **buf**. Thus, both pointers refer to the same memory.

**Return**

-EINVAL if buffer has insufficient size, 0 on success
