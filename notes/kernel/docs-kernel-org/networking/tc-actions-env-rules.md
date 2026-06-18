# TC Actions - Environmental Rules

> 출처(원문): https://docs.kernel.org/networking/tc-actions-env-rules.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TC Actions - Environmental Rules

The “environmental” rules for authors of any new tc actions are:

1. If you stealeth or borroweth any packet thou shalt be branching
   from the righteous path and thou shalt cloneth.

   For example if your action queues a packet to be processed later,
   or intentionally branches by redirecting a packet, then you need to
   clone the packet.
2. If you munge any packet thou shalt call pskb\_expand\_head in the case
   someone else is referencing the skb. After that you “own” the skb.
3. Dropping packets you don’t own is a no-no. You simply return
   TC\_ACT\_SHOT to the caller and they will drop it.

The “environmental” rules for callers of actions (qdiscs etc) are:

1. Thou art responsible for freeing anything returned as being
   TC\_ACT\_SHOT/STOLEN/QUEUED. If none of TC\_ACT\_SHOT/STOLEN/QUEUED is
   returned, then all is great and you don’t need to do anything.

Post on netdev if something is unclear.
