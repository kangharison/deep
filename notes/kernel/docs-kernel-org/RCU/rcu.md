# RCU Concepts

> 출처(원문): https://docs.kernel.org/RCU/rcu.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# RCU Concepts

The basic idea behind RCU (read-copy update) is to split destructive
operations into two parts, one that prevents anyone from seeing the data
item being destroyed, and one that actually carries out the destruction.
A “grace period” must elapse between the two parts, and this grace period
must be long enough that any readers accessing the item being deleted have
since dropped their references. For example, an RCU-protected deletion
from a linked list would first remove the item from the list, wait for
a grace period to elapse, then free the element. See [Using RCU to Protect Read-Mostly Linked Lists](listRCU.html) for more
information on using RCU with linked lists.

## Frequently Asked Questions

* Why would anyone want to use RCU?

  The advantage of RCU’s two-part approach is that RCU readers need
  not acquire any locks, perform any atomic instructions, write to
  shared memory, or (on CPUs other than Alpha) execute any memory
  barriers. The fact that these operations are quite expensive
  on modern CPUs is what gives RCU its performance advantages
  in read-mostly situations. The fact that RCU readers need not
  acquire locks can also greatly simplify deadlock-avoidance code.
* How can the updater tell when a grace period has completed
  if the RCU readers give no indication when they are done?

  Just as with spinlocks, RCU readers are not permitted to
  block, switch to user-mode execution, or enter the idle loop.
  Therefore, as soon as a CPU is seen passing through any of these
  three states, we know that that CPU has exited any previous RCU
  read-side critical sections. So, if we remove an item from a
  linked list, and then wait until all CPUs have switched context,
  executed in user mode, or executed in the idle loop, we can
  safely free up that item.

  Preemptible variants of RCU (CONFIG\_PREEMPT\_RCU) get the
  same effect, but require that the readers manipulate CPU-local
  counters. These counters allow limited types of blocking within
  RCU read-side critical sections. SRCU also uses CPU-local
  counters, and permits general blocking within RCU read-side
  critical sections. These variants of RCU detect grace periods
  by sampling these counters.
* If I am running on a uniprocessor kernel, which can only do one
  thing at a time, why should I wait for a grace period?

  See [RCU on Uniprocessor Systems](UP.html) for more information.
* How can I see where RCU is currently used in the Linux kernel?

  Search for “rcu\_read\_lock”, “rcu\_read\_unlock”, “call\_rcu”,
  “rcu\_read\_lock\_bh”, “rcu\_read\_unlock\_bh”, “srcu\_read\_lock”,
  “srcu\_read\_unlock”, “synchronize\_rcu”, “synchronize\_net”,
  “synchronize\_srcu”, and the other RCU primitives. Or grab one
  of the cscope databases from:

  (<http://www.rdrop.com/users/paulmck/RCU/linuxusage/rculocktab.html>).
* What guidelines should I follow when writing code that uses RCU?

  See [Review Checklist for RCU Patches](checklist.html).
* Why the name “RCU”?

  “RCU” stands for “read-copy update”.
  [Using RCU to Protect Read-Mostly Linked Lists](listRCU.html) has more information on where this name came from, search
  for “read-copy update” to find it.
* I hear that RCU is patented? What is with that?

  Yes, it is. There are several known patents related to RCU,
  search for the string “Patent” in Documentation/RCU/RTFP.txt to find them.
  Of these, one was allowed to lapse by the assignee, and the
  others have been contributed to the Linux kernel under GPL.
  Many (but not all) have long since expired.
  There are now also LGPL implementations of user-level RCU
  available (<https://liburcu.org/>).
* I hear that RCU needs work in order to support realtime kernels?

  Realtime-friendly RCU are enabled via the CONFIG\_PREEMPTION
  kernel configuration parameter.
* Where can I find more information on RCU?

  See the Documentation/RCU/RTFP.txt file.
  Or point your browser at (<https://docs.google.com/document/d/1X0lThx8OK0ZgLMqVoXiR4ZrGURHrXK6NyLRbeXe3Xac/edit>)
  or (<https://docs.google.com/document/d/1GCdQC8SDbb54W1shjEXqGZ0Rq8a6kIeYutdSIajfpLA/edit?usp=sharing>).
