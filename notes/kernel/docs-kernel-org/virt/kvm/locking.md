# KVM Lock Overview

> 출처(원문): https://docs.kernel.org/virt/kvm/locking.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# KVM Lock Overview

## 1. Acquisition Orders

The acquisition orders for mutexes are as follows:

* `cpus_read_lock()` is taken outside kvm\_lock
* kvm\_usage\_lock is taken outside `cpus_read_lock()`
* kvm->lock is taken outside vcpu->mutex
* kvm->lock is taken outside kvm->slots\_lock and kvm->irq\_lock
* vcpu->mutex is taken outside kvm->slots\_lock and kvm->slots\_arch\_lock
* kvm->slots\_lock is taken outside kvm->irq\_lock, though acquiring
  them together is quite rare.
* kvm->mn\_active\_invalidate\_count ensures that pairs of
  `invalidate_range_start()` and `invalidate_range_end()` callbacks
  use the same memslots array. kvm->slots\_lock and kvm->slots\_arch\_lock
  are taken on the waiting side when modifying memslots, so MMU notifiers
  must not take either kvm->slots\_lock or kvm->slots\_arch\_lock.

`cpus_read_lock()` vs kvm\_lock:

* Taking `cpus_read_lock()` outside of kvm\_lock is problematic, despite that
  being the official ordering, as it is quite easy to unknowingly trigger
  `cpus_read_lock()` while holding kvm\_lock. Use caution when walking vm\_list,
  e.g. avoid complex operations when possible.

For SRCU:

* `synchronize_srcu(&kvm->srcu)` is called inside critical sections
  for kvm->lock, vcpu->mutex and kvm->slots\_lock. These locks \_cannot\_
  be taken inside a kvm->srcu read-side critical section; that is, the
  following is broken:

  ```
  srcu_read_lock(&kvm->srcu);
  mutex_lock(&kvm->slots_lock);
  ```
* kvm->slots\_arch\_lock instead is released before the call to
  `synchronize_srcu()`. It \_can\_ therefore be taken inside a
  kvm->srcu read-side critical section, for example while processing
  a vmexit.

On x86:

* vcpu->mutex is taken outside kvm->arch.hyperv.hv\_lock and kvm->arch.xen.xen\_lock
* kvm->arch.mmu\_lock is an rwlock; critical sections for
  kvm->arch.tdp\_mmu\_pages\_lock and kvm->arch.mmu\_unsync\_pages\_lock must
  also take kvm->arch.mmu\_lock

Everything else is a leaf: no other lock is taken inside the critical
sections.

## 2. Exception

Fast page fault:

Fast page fault is the fast path which fixes the guest page fault out of
the mmu-lock on x86. Currently, the page fault can be fast in one of the
following two cases:

1. Access Tracking: The SPTE is not present, but it is marked for access
   tracking. That means we need to restore the saved R/X bits. This is
   described in more detail later below.
2. Write-Protection: The SPTE is present and the fault is caused by
   write-protect. That means we just need to change the W bit of the spte.

What we use to avoid all the races is the Host-writable bit and MMU-writable bit
on the spte:

* Host-writable means the gfn is writable in the host kernel page tables and in
  its KVM memslot.
* MMU-writable means the gfn is writable in the guest’s mmu and it is not
  write-protected by shadow page write-protection.

On fast page fault path, we will use cmpxchg to atomically set the spte W
bit if spte.HOST\_WRITEABLE = 1 and spte.WRITE\_PROTECT = 1, to restore the saved
R/X bits if for an access-traced spte, or both. This is safe because whenever
changing these bits can be detected by cmpxchg.

But we need carefully check these cases:

1. The mapping from gfn to pfn

The mapping from gfn to pfn may be changed since we can only ensure the pfn
is not changed during cmpxchg. This is a ABA problem, for example, below case
will happen:

|  |  |
| --- | --- |
| At the beginning:  ``` gpte = gfn1 gfn1 is mapped to pfn1 on host spte is the shadow page table entry corresponding with gpte and spte = pfn1 ``` | |
| On fast page fault path: | |
| CPU 0: | CPU 1: |
| ``` old_spte = *spte; ``` |  |
|  | pfn1 is swapped out:  ``` spte = 0; ```  pfn1 is re-alloced for gfn2.  gpte is changed to point to gfn2 by the guest:  ``` spte = pfn1; ``` |
| ``` if (cmpxchg(spte, old_spte, old_spte+W)     mark_page_dirty(vcpu->kvm, gfn1)          OOPS!!! ``` | |

We dirty-log for gfn1, that means gfn2 is lost in dirty-bitmap.

For direct sp, we can easily avoid it since the spte of direct sp is fixed
to gfn. For indirect sp, we disabled fast page fault for simplicity.

A solution for indirect sp could be to pin the gfn before the cmpxchg. After
the pinning:

* We have held the refcount of pfn; that means the pfn can not be freed and
  be reused for another gfn.
* The pfn is writable and therefore it cannot be shared between different gfns
  by KSM.

Then, we can ensure the dirty bitmaps is correctly set for a gfn.

2. Dirty bit tracking

In the original code, the spte can be fast updated (non-atomically) if the
spte is read-only and the Accessed bit has already been set since the
Accessed bit and Dirty bit can not be lost.

But it is not true after fast page fault since the spte can be marked
writable between reading spte and updating spte. Like below case:

|  |  |
| --- | --- |
| At the beginning:  ``` spte.W = 0 spte.Accessed = 1 ``` | |
| CPU 0: | CPU 1: |
| In `mmu_spte_update()`:  ``` old_spte = *spte;   /* 'if' condition is satisfied. */ if (old_spte.Accessed == 1 &&      old_spte.W == 0)    spte = new_spte; ``` |  |
|  | on fast page fault path:  ``` spte.W = 1 ```  memory write on the spte:  ``` spte.Dirty = 1 ``` |
| ``` else   old_spte = xchg(spte, new_spte); if (old_spte.Accessed &&     !new_spte.Accessed)   flush = true; if (old_spte.Dirty &&     !new_spte.Dirty)   flush = true;   OOPS!!! ``` |  |

The Dirty bit is lost in this case.

In order to avoid this kind of issue, we always treat the spte as “volatile”
if it can be updated out of mmu-lock [see `spte_needs_atomic_update()`]; it means
the spte is always atomically updated in this case.

3. flush tlbs due to spte updated

If the spte is updated from writable to read-only, we should flush all TLBs,
otherwise rmap\_write\_protect will find a read-only spte, even though the
writable spte might be cached on a CPU’s TLB.

As mentioned before, the spte can be updated to writable out of mmu-lock on
fast page fault path. In order to easily audit the path, we see if TLBs needing
to be flushed caused this reason in `mmu_spte_update()` since this is a common
function to update spte (present -> present).

Since the spte is “volatile” if it can be updated out of mmu-lock, we always
atomically update the spte and the race caused by fast page fault can be avoided.
See the comments in `spte_needs_atomic_update()` and `mmu_spte_update()`.

Lockless Access Tracking:

This is used for Intel CPUs that are using EPT but do not support the EPT A/D
bits. In this case, PTEs are tagged as A/D disabled (using ignored bits), and
when the KVM MMU notifier is called to track accesses to a page (via
kvm\_mmu\_notifier\_clear\_flush\_young), it marks the PTE not-present in hardware
by clearing the RWX bits in the PTE and storing the original R & X bits in more
unused/ignored bits. When the VM tries to access the page later on, a fault is
generated and the fast page fault mechanism described above is used to
atomically restore the PTE to a Present state. The W bit is not saved when the
PTE is marked for access tracking and during restoration to the Present state,
the W bit is set depending on whether or not it was a write access. If it
wasn’t, then the W bit will remain clear until a write access happens, at which
time it will be set using the Dirty tracking mechanism described above.

## 3. Reference

### `kvm_lock`

Type:
:   mutex

Arch:
:   any

Protects:
:   * vm\_list

### `kvm_usage_lock`

Type:
:   mutex

Arch:
:   any

Protects:
:   * kvm\_usage\_count
    * hardware virtualization enable/disable

Comment:
:   Exists to allow taking `cpus_read_lock()` while kvm\_usage\_count is
    protected, which simplifies the virtualization enabling logic.

### `kvm->mn_invalidate_lock`

Type:
:   spinlock\_t

Arch:
:   any

Protects:
:   mn\_active\_invalidate\_count, mn\_memslots\_update\_rcuwait

### `kvm_arch::tsc_write_lock`

Type:
:   raw\_spinlock\_t

Arch:
:   x86

Protects:
:   * kvm\_arch::{last\_tsc\_write,last\_tsc\_nsec,last\_tsc\_offset}
    * tsc offset in vmcb

Comment:
:   ‘raw’ because updating the tsc offsets must not be preempted.

### `kvm->mmu_lock`

Type:
:   spinlock\_t or rwlock\_t

Arch:
:   any

Protects:
:   -shadow page/shadow tlb entry

Comment:
:   it is a spinlock since it is used in mmu notifier.

### `kvm->srcu`

Type:
:   srcu lock

Arch:
:   any

Protects:
:   * kvm->memslots
    * kvm->buses

Comment:
:   The srcu read lock must be held while accessing memslots (e.g.
    when using gfn\_to\_\* functions) and while accessing in-kernel
    MMIO/PIO address->device structure mapping (kvm->buses).
    The srcu index can be stored in kvm\_vcpu->srcu\_idx per vcpu
    if it is needed by multiple functions.

### `kvm->slots_arch_lock`

Type:
:   mutex

Arch:
:   any (only needed on x86 though)

Protects:
:   any arch-specific fields of memslots that have to be modified
    in a `kvm->srcu` read-side critical section.

Comment:
:   must be held before reading the pointer to the current memslots,
    until after all changes to the memslots are complete

### `wakeup_vcpus_on_cpu_lock`

Type:
:   spinlock\_t

Arch:
:   x86

Protects:
:   wakeup\_vcpus\_on\_cpu

Comment:
:   This is a per-CPU lock and it is used for VT-d posted-interrupts.
    When VT-d posted-interrupts are supported and the VM has assigned
    devices, we put the blocked vCPU on the list blocked\_vcpu\_on\_cpu
    protected by blocked\_vcpu\_on\_cpu\_lock. When VT-d hardware issues
    wakeup notification event since external interrupts from the
    assigned devices happens, we will find the vCPU on the list to
    wakeup.

### `vendor_module_lock`

Type:
:   mutex

Arch:
:   x86

Protects:
:   loading a vendor module (kvm\_amd or kvm\_intel)

Comment:
:   Exists because using kvm\_lock leads to deadlock. kvm\_lock is taken
    in notifiers, e.g. `__kvmclock_cpufreq_notifier()`, that may be invoked while
    cpu\_hotplug\_lock is held, e.g. from `cpufreq_boost_trigger_state()`, and many
    operations need to take cpu\_hotplug\_lock when loading a vendor module, e.g.
    updating static calls.
