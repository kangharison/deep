# Digital signature algorithms

> 출처(원문): https://docs.kernel.org/crypto/libcrypto-signature.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Digital signature algorithms

## ML-DSA

Support for the ML-DSA digital signature algorithm.

int mldsa\_verify(enum mldsa\_alg alg, const u8 \*sig, size\_t sig\_len, const u8 \*msg, size\_t msg\_len, const u8 \*pk, size\_t pk\_len)
:   Verify an ML-DSA signature

**Parameters**

`enum mldsa_alg alg`
:   The ML-DSA parameter set to use

`const u8 *sig`
:   The signature

`size_t sig_len`
:   Length of the signature in bytes. Should match the
    MLDSA\*\_SIGNATURE\_SIZE constant associated with **alg**,
    otherwise -EBADMSG will be returned.

`const u8 *msg`
:   The message

`size_t msg_len`
:   Length of the message in bytes

`const u8 *pk`
:   The public key

`size_t pk_len`
:   Length of the public key in bytes. Should match the
    MLDSA\*\_PUBLIC\_KEY\_SIZE constant associated with **alg**,
    otherwise -EBADMSG will be returned.

**Description**

This verifies a signature using pure ML-DSA with the specified parameter set.
The context string is assumed to be empty. This corresponds to FIPS 204
Algorithm 3 “ML-DSA.Verify” with the ctx parameter set to the empty string
and the lengths of the signature and key given explicitly by the caller.

**Context**

Might sleep

**Return**

* 0 if the signature is valid
* -EBADMSG if the signature and/or public key is malformed
* -EKEYREJECTED if the signature is invalid but otherwise well-formed
* -ENOMEM if out of memory so the validity of the signature is unknown
