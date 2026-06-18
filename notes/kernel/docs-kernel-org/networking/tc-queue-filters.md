# TC queue based filtering

> 출처(원문): https://docs.kernel.org/networking/tc-queue-filters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TC queue based filtering

TC can be used for directing traffic to either a set of queues or
to a single queue on both the transmit and receive side.

On the transmit side:

1. TC filter directing traffic to a set of queues is achieved
   using the action skbedit priority for Tx priority selection,
   the priority maps to a traffic class (set of queues) when
   the queue-sets are configured using mqprio.
2. TC filter directs traffic to a transmit queue with the action
   skbedit queue\_mapping $tx\_qid. The action skbedit queue\_mapping
   for transmit queue is executed in software only and cannot be
   offloaded.

Likewise, on the receive side, the two filters for selecting set of
queues and/or a single queue are supported as below:

1. TC flower filter directs incoming traffic to a set of queues using
   the ‘hw\_tc’ option.
   hw\_tc $TCID - Specify a hardware traffic class to pass matching
   packets on to. TCID is in the range 0 through 15.
2. TC filter with action skbedit queue\_mapping $rx\_qid selects a
   receive queue. The action skbedit queue\_mapping for receive queue
   is supported only in hardware. Multiple filters may compete in
   the hardware for queue selection. In such case, the hardware
   pipeline resolves conflicts based on priority. On Intel E810
   devices, TC filter directing traffic to a queue have higher
   priority over flow director filter assigning a queue. The hash
   filter has lowest priority.
