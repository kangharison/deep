# TODO

> 출처(원문): https://docs.kernel.org/admin-guide/aoe/todo.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TODO

There is a potential for deadlock when allocating a [`struct sk_buff`](../../networking/kapi.html#c.sk_buff "sk_buff") for
data that needs to be written out to aoe storage. If the data is
being written from a dirty page in order to free that page, and if
there are no other pages available, then deadlock may occur when a
free page is needed for the sk\_buff allocation. This situation has
not been observed, but it would be nice to eliminate any potential for
deadlock under memory pressure.

Because ATA over Ethernet is not fragmented by the kernel’s IP code,
the destructor member of the [`struct sk_buff`](../../networking/kapi.html#c.sk_buff "sk_buff") is available to the aoe
driver. By using a mempool for allocating all but the first few
sk\_buffs, and by registering a destructor, we should be able to
efficiently allocate sk\_buffs without introducing any potential for
deadlock.
