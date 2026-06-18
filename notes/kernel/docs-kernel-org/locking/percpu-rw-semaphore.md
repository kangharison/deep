# Percpu rw semaphores

> 출처(원문): https://docs.kernel.org/locking/percpu-rw-semaphore.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Percpu rw semaphores

Percpu rw semaphores is a new read-write semaphore design that is
optimized for locking for reading.

The problem with traditional read-write semaphores is that when multiple
cores take the lock for reading, the cache line containing the semaphore
is bouncing between L1 caches of the cores, causing performance
degradation.

Locking for reading is very fast, it uses RCU and it avoids any atomic
instruction in the lock and unlock path. On the other hand, locking for
writing is very expensive, it calls [`synchronize_rcu()`](../core-api/kernel-api.html#c.synchronize_rcu "synchronize_rcu") that can take
hundreds of milliseconds.

The lock is declared with “`struct percpu_rw_semaphore`” type.
The lock is initialized with percpu\_init\_rwsem, it returns 0 on success
and -ENOMEM on allocation failure.
The lock must be freed with percpu\_free\_rwsem to avoid memory leak.

The lock is locked for read with percpu\_down\_read, percpu\_up\_read and
for write with percpu\_down\_write, percpu\_up\_write.

The idea of using RCU for optimized rw-lock was introduced by
Eric Dumazet <[eric.dumazet@gmail.com](mailto:eric.dumazet%40gmail.com)>.
The code was written by Mikulas Patocka <[mpatocka@redhat.com](mailto:mpatocka%40redhat.com)>
