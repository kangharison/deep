# Programming Interface

> 출처(원문): https://docs.kernel.org/crypto/api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Programming Interface

Table of contents

* [Symmetric Key Cipher](api-skcipher.html)
  + [Block Cipher Algorithm Definitions](api-skcipher.html#block-cipher-algorithm-definitions)
  + [Symmetric Key Cipher API](api-skcipher.html#symmetric-key-cipher-api)
  + [Symmetric Key Cipher Request Handle](api-skcipher.html#symmetric-key-cipher-request-handle)
  + [Single Block Cipher API](api-skcipher.html#single-block-cipher-api)
* [Authenticated Encryption With Associated Data (AEAD)](api-aead.html)
  + [Authenticated Encryption With Associated Data (AEAD) Algorithm Definitions](api-aead.html#authenticated-encryption-with-associated-data-aead-algorithm-definitions)
  + [Authenticated Encryption With Associated Data (AEAD) Cipher API](api-aead.html#authenticated-encryption-with-associated-data-aead-cipher-api)
  + [Asynchronous AEAD Request Handle](api-aead.html#asynchronous-aead-request-handle)
* [Message Digest](api-digest.html)
  + [Message Digest Algorithm Definitions](api-digest.html#message-digest-algorithm-definitions)
  + [Asynchronous Message Digest API](api-digest.html#asynchronous-message-digest-api)
  + [Asynchronous Hash Request Handle](api-digest.html#asynchronous-hash-request-handle)
  + [Synchronous Message Digest API](api-digest.html#synchronous-message-digest-api)
* [Random Number Generator (RNG)](api-rng.html)
  + [Random Number Algorithm Definitions](api-rng.html#random-number-algorithm-definitions)
  + [Crypto API Random Number API](api-rng.html#crypto-api-random-number-api)
* [Asymmetric Cipher](api-akcipher.html)
  + [Asymmetric Cipher Algorithm Definitions](api-akcipher.html#asymmetric-cipher-algorithm-definitions)
  + [Asymmetric Cipher API](api-akcipher.html#asymmetric-cipher-api)
  + [Asymmetric Cipher Request Handle](api-akcipher.html#asymmetric-cipher-request-handle)
* [Asymmetric Signature](api-sig.html)
  + [Asymmetric Signature Algorithm Definitions](api-sig.html#asymmetric-signature-algorithm-definitions)
  + [Asymmetric Signature API](api-sig.html#asymmetric-signature-api)
* [Key-agreement Protocol Primitives (KPP)](api-kpp.html)
  + [Key-agreement Protocol Primitives (KPP) Cipher Algorithm Definitions](api-kpp.html#key-agreement-protocol-primitives-kpp-cipher-algorithm-definitions)
  + [Key-agreement Protocol Primitives (KPP) Cipher API](api-kpp.html#key-agreement-protocol-primitives-kpp-cipher-api)
  + [Key-agreement Protocol Primitives (KPP) Cipher Request Handle](api-kpp.html#key-agreement-protocol-primitives-kpp-cipher-request-handle)
  + [ECDH Helper Functions](api-kpp.html#ecdh-helper-functions)
  + [DH Helper Functions](api-kpp.html#dh-helper-functions)
