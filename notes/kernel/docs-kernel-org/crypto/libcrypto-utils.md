# Utility functions

> 출처(원문): https://docs.kernel.org/crypto/libcrypto-utils.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Utility functions

int crypto\_memneq(const void \*a, const void \*b, size\_t size)
:   Compare two areas of memory without leaking timing information.

**Parameters**

`const void *a`
:   One area of memory

`const void *b`
:   Another area of memory

`size_t size`
:   The size of the area.

**Description**

Returns 0 when data is equal, 1 otherwise.
