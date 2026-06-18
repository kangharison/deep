# Kyber I/O scheduler tunables

> 출처(원문): https://docs.kernel.org/block/kyber-iosched.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kyber I/O scheduler tunables

The only two tunables for the Kyber scheduler are the target latencies for
reads and synchronous writes. Kyber will throttle requests in order to meet
these target latencies.

## read\_lat\_nsec

Target latency for reads (in nanoseconds).

## write\_lat\_nsec

Target latency for synchronous writes (in nanoseconds).
