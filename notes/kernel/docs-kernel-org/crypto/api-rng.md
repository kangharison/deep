# Random Number Generator (RNG)

> 출처(원문): https://docs.kernel.org/crypto/api-rng.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Random Number Generator (RNG)

## Random Number Algorithm Definitions

struct rng\_alg
:   random number generator definition

**Definition**:

```
struct rng_alg {
    int (*generate)(struct crypto_rng *tfm, const u8 *src, unsigned int slen, u8 *dst, unsigned int dlen);
    int (*seed)(struct crypto_rng *tfm, const u8 *seed, unsigned int slen);
    void (*set_ent)(struct crypto_rng *tfm, const u8 *data, unsigned int len);
    unsigned int seedsize;
    struct crypto_alg base;
};
```

**Members**

`generate`
:   The function defined by this variable obtains a
    random number. The random number generator transform
    must generate the random number out of the context
    provided with this call, plus any additional data
    if provided to the call.

`seed`
:   Seed or reseed the random number generator. With the
    invocation of this function call, the random number
    generator shall become ready for generation. If the
    random number generator requires a seed for setting
    up a new state, the seed must be provided by the
    consumer while invoking this function. The required
    size of the seed is defined with **seedsize** .

`set_ent`
:   Set entropy that would otherwise be obtained from
    entropy source. Internal use only.

`seedsize`
:   The seed size required for a random number generator
    initialization defined with this variable. Some
    random number generators does not require a seed
    as the seeding is implemented internally without
    the need of support by the consumer. In this case,
    the seed size is set to zero.

`base`
:   Common crypto API algorithm data structure.

## Crypto API Random Number API

The random number generator API is used with the ciphers of type
CRYPTO\_ALG\_TYPE\_RNG (listed as type “rng” in /proc/crypto)

struct crypto\_rng \*crypto\_alloc\_rng(const char \*alg\_name, u32 type, u32 mask)
:   * allocate RNG handle

**Parameters**

`const char *alg_name`
:   is the cra\_name / name or cra\_driver\_name / driver name of the
    message digest cipher

`u32 type`
:   specifies the type of the cipher

`u32 mask`
:   specifies the mask for the cipher

**Description**

Allocate a cipher handle for a random number generator. The returned `struct
crypto_rng` is the cipher handle that is required for any subsequent
API invocation for that random number generator.

For all random number generators, this call creates a new private copy of
the random number generator that does not share a state with other
instances. The only exception is the “krng” random number generator which
is a kernel crypto API use case for the `get_random_bytes()` function of the
/dev/random driver.

**Return**

allocated cipher handle in case of success; [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") is true in case
of an error, [`PTR_ERR()`](../core-api/kernel-api.html#c.PTR_ERR "PTR_ERR") returns the error code.

struct [rng\_alg](#c.rng_alg "rng_alg") \*crypto\_rng\_alg(struct crypto\_rng \*tfm)
:   obtain ‘[`struct rng_alg`](#c.rng_alg "rng_alg")’ pointer from RNG handle

**Parameters**

`struct crypto_rng *tfm`
:   RNG handle

**Return**

Pointer to ‘[`struct rng_alg`](#c.rng_alg "rng_alg")’, derived from **tfm** RNG handle

void crypto\_free\_rng(struct crypto\_rng \*tfm)
:   zeroize and free RNG handle

**Parameters**

`struct crypto_rng *tfm`
:   cipher handle to be freed

**Description**

If **tfm** is a NULL or error pointer, this function does nothing.

int crypto\_rng\_generate(struct crypto\_rng \*tfm, const u8 \*src, unsigned int slen, u8 \*dst, unsigned int dlen)
:   get random number

**Parameters**

`struct crypto_rng *tfm`
:   cipher handle

`const u8 *src`
:   Input buffer holding additional data, may be NULL

`unsigned int slen`
:   Length of additional data

`u8 *dst`
:   output buffer holding the random numbers

`unsigned int dlen`
:   length of the output buffer

**Description**

This function fills the caller-allocated buffer with random
numbers using the random number generator referenced by the
cipher handle.

**Return**

0 function was successful; < 0 if an error occurred

int crypto\_rng\_get\_bytes(struct crypto\_rng \*tfm, u8 \*rdata, unsigned int dlen)
:   get random number

**Parameters**

`struct crypto_rng *tfm`
:   cipher handle

`u8 *rdata`
:   output buffer holding the random numbers

`unsigned int dlen`
:   length of the output buffer

**Description**

This function fills the caller-allocated buffer with random numbers using the
random number generator referenced by the cipher handle.

**Return**

0 function was successful; < 0 if an error occurred

int crypto\_rng\_reset(struct crypto\_rng \*tfm, const u8 \*seed, unsigned int slen)
:   re-initialize the RNG

**Parameters**

`struct crypto_rng *tfm`
:   cipher handle

`const u8 *seed`
:   seed input data

`unsigned int slen`
:   length of the seed input data

**Description**

The reset function completely re-initializes the random number generator
referenced by the cipher handle by clearing the current state. The new state
is initialized with the caller provided seed or automatically, depending on
the random number generator type. (The SP800-90A DRBGs perform an automatic
seeding.) The seed is provided as a parameter to this function call. The
provided seed should have the length of the seed size defined for the random
number generator as defined by crypto\_rng\_seedsize.

**Return**

0 if the setting of the key was successful; < 0 if an error occurred

int crypto\_rng\_seedsize(struct crypto\_rng \*tfm)
:   obtain seed size of RNG

**Parameters**

`struct crypto_rng *tfm`
:   cipher handle

**Description**

The function returns the seed size for the random number generator
referenced by the cipher handle. This value may be zero if the random
number generator does not implement or require a reseeding. For example,
the SP800-90A DRBGs implement an automated reseeding after reaching a
pre-defined threshold.

**Return**

seed size for the random number generator
