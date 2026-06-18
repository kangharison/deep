# 33.Software Guard eXtensions (SGX)

> 출처(원문): https://docs.kernel.org/arch/x86/sgx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 33. Software Guard eXtensions (SGX)

## 33.1. Overview

Software Guard eXtensions (SGX) hardware enables for user space applications
to set aside private memory regions of code and data:

* Privileged (ring-0) ENCLS functions orchestrate the construction of the
  regions.
* Unprivileged (ring-3) ENCLU functions allow an application to enter and
  execute inside the regions.

These memory regions are called enclaves. An enclave can be only entered at a
fixed set of entry points. Each entry point can hold a single hardware thread
at a time. While the enclave is loaded from a regular binary file by using
ENCLS functions, only the threads inside the enclave can access its memory. The
region is denied from outside access by the CPU, and encrypted before it leaves
from LLC.

The support can be determined by

> `grep sgx /proc/cpuinfo`

SGX must both be supported in the processor and enabled by the BIOS. If SGX
appears to be unsupported on a system which has hardware support, ensure
support is enabled in the BIOS. If a BIOS presents a choice between “Enabled”
and “Software Enabled” modes for SGX, choose “Enabled”.

## 33.2. Enclave Page Cache

SGX utilizes an *Enclave Page Cache (EPC)* to store pages that are associated
with an enclave. It is contained in a BIOS-reserved region of physical memory.
Unlike pages used for regular memory, pages can only be accessed from outside of
the enclave during enclave construction with special, limited SGX instructions.

Only a CPU executing inside an enclave can directly access enclave memory.
However, a CPU executing inside an enclave may access normal memory outside the
enclave.

The kernel manages enclave memory similar to how it treats device memory.

### 33.2.1. Enclave Page Types

**SGX Enclave Control Structure (SECS)**
:   Enclave’s address range, attributes and other global data are defined
    by this structure.

**Regular (REG)**
:   Regular EPC pages contain the code and data of an enclave.

**Thread Control Structure (TCS)**
:   Thread Control Structure pages define the entry points to an enclave and
    track the execution state of an enclave thread.

**Version Array (VA)**
:   Version Array pages contain 512 slots, each of which can contain a version
    number for a page evicted from the EPC.

### 33.2.2. Enclave Page Cache Map

The processor tracks EPC pages in a hardware metadata structure called the
*Enclave Page Cache Map (EPCM)*. The EPCM contains an entry for each EPC page
which describes the owning enclave, access rights and page type among the other
things.

EPCM permissions are separate from the normal page tables. This prevents the
kernel from, for instance, allowing writes to data which an enclave wishes to
remain read-only. EPCM permissions may only impose additional restrictions on
top of normal x86 page permissions.

For all intents and purposes, the SGX architecture allows the processor to
invalidate all EPCM entries at will. This requires that software be prepared to
handle an EPCM fault at any time. In practice, this can happen on events like
power transitions when the ephemeral key that encrypts enclave memory is lost.

## 33.3. Application interface

### 33.3.1. Enclave build functions

In addition to the traditional compiler and linker build process, SGX has a
separate enclave “build” process. Enclaves must be built before they can be
executed (entered). The first step in building an enclave is opening the
**/dev/sgx\_enclave** device. Since enclave memory is protected from direct
access, special privileged instructions are then used to copy data into enclave
pages and establish enclave page permissions.

long sgx\_ioc\_enclave\_create(struct sgx\_encl \*encl, void \_\_user \*arg)
:   handler for `SGX_IOC_ENCLAVE_CREATE`

**Parameters**

`struct sgx_encl *encl`
:   An enclave pointer.

`void __user *arg`
:   The ioctl argument.

**Description**

Allocate kernel data structures for the enclave and invoke ECREATE.

**Return**

* 0: Success.
* -EIO: ECREATE failed.
* -errno: POSIX error.

long sgx\_ioc\_enclave\_add\_pages(struct sgx\_encl \*encl, void \_\_user \*arg)
:   The handler for `SGX_IOC_ENCLAVE_ADD_PAGES`

**Parameters**

`struct sgx_encl *encl`
:   an enclave pointer

`void __user *arg`
:   a user pointer to a `struct sgx_enclave_add_pages` instance

**Description**

Add one or more pages to an uninitialized enclave, and optionally extend the
measurement with the contents of the page. The SECINFO and measurement mask
are applied to all pages.

A SECINFO for a TCS is required to always contain zero permissions because
CPU silently zeros them. Allowing anything else would cause a mismatch in
the measurement.

mmap()’s protection bits are capped by the page permissions. For each page
address, the maximum protection bits are computed with the following
heuristics:

1. A regular page: PROT\_R, PROT\_W and PROT\_X match the SECINFO permissions.
2. A TCS page: PROT\_R | PROT\_W.

mmap() is not allowed to surpass the minimum of the maximum protection bits
within the given address range.

The function deinitializes kernel data structures for enclave and returns
-EIO in any of the following conditions:

* Enclave Page Cache (EPC), the physical memory holding enclaves, has
  been invalidated. This will cause EADD and EEXTEND to fail.
* If the source address is corrupted somehow when executing EADD.

**Return**

* 0: Success.
* -EACCES: The source page is located in a noexec partition.
* -ENOMEM: Out of EPC pages.
* -EINTR: The call was interrupted before data was processed.
* -EIO: Either EADD or EEXTEND failed because invalid source address
  :   or power cycle.
* -errno: POSIX error.

long sgx\_ioc\_enclave\_init(struct sgx\_encl \*encl, void \_\_user \*arg)
:   handler for `SGX_IOC_ENCLAVE_INIT`

**Parameters**

`struct sgx_encl *encl`
:   an enclave pointer

`void __user *arg`
:   userspace pointer to a `struct sgx_enclave_init` instance

**Description**

Flush any outstanding enqueued EADD operations and perform EINIT. The
Launch Enclave Public Key Hash MSRs are rewritten as necessary to match
the enclave’s MRSIGNER, which is calculated from the provided sigstruct.

**Return**

* 0: Success.
* -EPERM: Invalid SIGSTRUCT.
* -EIO: EINIT failed because of a power cycle.
* -errno: POSIX error.

long sgx\_ioc\_enclave\_provision(struct sgx\_encl \*encl, void \_\_user \*arg)
:   handler for `SGX_IOC_ENCLAVE_PROVISION`

**Parameters**

`struct sgx_encl *encl`
:   an enclave pointer

`void __user *arg`
:   userspace pointer to a `struct sgx_enclave_provision` instance

**Description**

Allow ATTRIBUTE.PROVISION\_KEY for an enclave by providing a file handle to
/dev/sgx\_provision.

**Return**

* 0: Success.
* -errno: Otherwise.

### 33.3.2. Enclave runtime management

Systems supporting SGX2 additionally support changes to initialized
enclaves: modifying enclave page permissions and type, and dynamically
adding and removing of enclave pages. When an enclave accesses an address
within its address range that does not have a backing page then a new
regular page will be dynamically added to the enclave. The enclave is
still required to run EACCEPT on the new page before it can be used.

long sgx\_ioc\_enclave\_restrict\_permissions(struct sgx\_encl \*encl, void \_\_user \*arg)
:   handler for `SGX_IOC_ENCLAVE_RESTRICT_PERMISSIONS`

**Parameters**

`struct sgx_encl *encl`
:   an enclave pointer

`void __user *arg`
:   userspace pointer to a `struct sgx_enclave_restrict_permissions`
    instance

**Description**

SGX2 distinguishes between relaxing and restricting the enclave page
permissions maintained by the hardware (EPCM permissions) of pages
belonging to an initialized enclave (after SGX\_IOC\_ENCLAVE\_INIT).

EPCM permissions cannot be restricted from within the enclave, the enclave
requires the kernel to run the privileged level 0 instructions ENCLS[EMODPR]
and ENCLS[ETRACK]. An attempt to relax EPCM permissions with this call
will be ignored by the hardware.

**Return**

* 0: Success
* -errno: Otherwise

long sgx\_ioc\_enclave\_modify\_types(struct sgx\_encl \*encl, void \_\_user \*arg)
:   handler for `SGX_IOC_ENCLAVE_MODIFY_TYPES`

**Parameters**

`struct sgx_encl *encl`
:   an enclave pointer

`void __user *arg`
:   userspace pointer to a `struct sgx_enclave_modify_types` instance

**Description**

Ability to change the enclave page type supports the following use cases:

* It is possible to add TCS pages to an enclave by changing the type of
  regular pages (`SGX_PAGE_TYPE_REG`) to TCS (`SGX_PAGE_TYPE_TCS`) pages.
  With this support the number of threads supported by an initialized
  enclave can be increased dynamically.
* Regular or TCS pages can dynamically be removed from an initialized
  enclave by changing the page type to `SGX_PAGE_TYPE_TRIM`. Changing the
  page type to `SGX_PAGE_TYPE_TRIM` marks the page for removal with actual
  removal done by handler of `SGX_IOC_ENCLAVE_REMOVE_PAGES` ioctl() called
  after ENCLU[EACCEPT] is run on `SGX_PAGE_TYPE_TRIM` page from within the
  enclave.

**Return**

* 0: Success
* -errno: Otherwise

long sgx\_ioc\_enclave\_remove\_pages(struct sgx\_encl \*encl, void \_\_user \*arg)
:   handler for `SGX_IOC_ENCLAVE_REMOVE_PAGES`

**Parameters**

`struct sgx_encl *encl`
:   an enclave pointer

`void __user *arg`
:   userspace pointer to `struct sgx_enclave_remove_pages` instance

**Description**

Final step of the flow removing pages from an initialized enclave. The
complete flow is:

1. User changes the type of the pages to be removed to `SGX_PAGE_TYPE_TRIM`
   using the `SGX_IOC_ENCLAVE_MODIFY_TYPES` ioctl().
2. User approves the page removal by running ENCLU[EACCEPT] from within
   the enclave.
3. User initiates actual page removal using the
   `SGX_IOC_ENCLAVE_REMOVE_PAGES` ioctl() that is handled here.

First remove any page table entries pointing to the page and then proceed
with the actual removal of the enclave page and data in support of it.

VA pages are not affected by this removal. It is thus possible that the
enclave may end up with more VA pages than needed to support all its
pages.

**Return**

* 0: Success
* -errno: Otherwise

### 33.3.3. Enclave vDSO

Entering an enclave can only be done through SGX-specific EENTER and ERESUME
functions, and is a non-trivial process. Because of the complexity of
transitioning to and from an enclave, enclaves typically utilize a library to
handle the actual transitions. This is roughly analogous to how glibc
implementations are used by most applications to wrap system calls.

Another crucial characteristic of enclaves is that they can generate exceptions
as part of their normal operation that need to be handled in the enclave or are
unique to SGX.

Instead of the traditional signal mechanism to handle these exceptions, SGX
can leverage special exception fixup provided by the vDSO. The kernel-provided
vDSO function wraps low-level transitions to/from the enclave like EENTER and
ERESUME. The vDSO function intercepts exceptions that would otherwise generate
a signal and return the fault information directly to its caller. This avoids
the need to juggle signal handlers.

vdso\_sgx\_enter\_enclave\_t
:   **Typedef**: Prototype for `__vdso_sgx_enter_enclave()`, a vDSO function to enter an SGX enclave.

**Syntax**

> `int vdso_sgx_enter_enclave_t (unsigned long rdi, unsigned long rsi, unsigned long rdx, unsigned int function, unsigned long r8, unsigned long r9, struct sgx_enclave_run *run)`

**Parameters**

`unsigned long rdi`
:   Pass-through value for RDI

`unsigned long rsi`
:   Pass-through value for RSI

`unsigned long rdx`
:   Pass-through value for RDX

`unsigned int function`
:   ENCLU function, must be EENTER or ERESUME

`unsigned long r8`
:   Pass-through value for R8

`unsigned long r9`
:   Pass-through value for R9

`struct sgx_enclave_run *run`
:   `struct sgx_enclave_run`, must be non-NULL

**NOTE**

`__vdso_sgx_enter_enclave()` does not ensure full compliance with the
x86-64 ABI, e.g. doesn’t handle XSAVE state. Except for non-volatile
general purpose registers, EFLAGS.DF, and RSP alignment, preserving/setting
state in accordance with the x86-64 ABI is the responsibility of the enclave
and its runtime, i.e. `__vdso_sgx_enter_enclave()` cannot be called from C
code without careful consideration by both the enclave and its runtime.

All general purpose registers except RAX, RBX and RCX are passed as-is to the
enclave. RAX, RBX and RCX are consumed by EENTER and ERESUME and are loaded
with **function**, asynchronous exit pointer, and **run.tcs** respectively.

RBP and the stack are used to anchor `__vdso_sgx_enter_enclave()` to the
pre-enclave state, e.g. to retrieve **run.exception** and **run.user\_handler**
after an enclave exit. All other registers are available for use by the
enclave and its runtime, e.g. an enclave can push additional data onto the
stack (and modify RSP) to pass information to the optional user handler (see
below).

Most exceptions reported on ENCLU, including those that occur within the
enclave, are fixed up and reported synchronously instead of being delivered
via a standard signal. Debug Exceptions (#DB) and Breakpoints (#BP) are
never fixed up and are always delivered via standard signals. On synchronously
reported exceptions, -EFAULT is returned and details about the exception are
recorded in **run.exception**, the optional sgx\_enclave\_exception struct.

**Return**

* 0: ENCLU function was successfully executed.
* -EINVAL: Invalid ENCL number (neither EENTER nor ERESUME).

## 33.4. ksgxd

SGX support includes a kernel thread called *ksgxd*.

### 33.4.1. EPC sanitization

ksgxd is started when SGX initializes. Enclave memory is typically ready
for use when the processor powers on or resets. However, if SGX has been in
use since the reset, enclave pages may be in an inconsistent state. This might
occur after a crash and `kexec()` cycle, for instance. At boot, ksgxd
reinitializes all enclave pages so that they can be allocated and re-used.

The sanitization is done by going through EPC address space and applying the
EREMOVE function to each physical page. Some enclave pages like SECS pages have
hardware dependencies on other pages which prevents EREMOVE from functioning.
Executing two EREMOVE passes removes the dependencies.

### 33.4.2. Page reclaimer

Similar to the core kswapd, ksgxd, is responsible for managing the
overcommitment of enclave memory. If the system runs out of enclave memory,
*ksgxd* “swaps” enclave memory to normal memory.

## 33.5. Launch Control

SGX provides a launch control mechanism. After all enclave pages have been
copied, kernel executes EINIT function, which initializes the enclave. Only after
this the CPU can execute inside the enclave.

EINIT function takes an RSA-3072 signature of the enclave measurement. The function
checks that the measurement is correct and signature is signed with the key
hashed to the four **IA32\_SGXLEPUBKEYHASH{0, 1, 2, 3}** MSRs representing the
SHA256 of a public key.

Those MSRs can be configured by the BIOS to be either readable or writable.
Linux supports only writable configuration in order to give full control to the
kernel on launch control policy. Before calling EINIT function, the driver sets
the MSRs to match the enclave’s signing key.

## 33.6. Encryption engines

In order to conceal the enclave data while it is out of the CPU package, the
memory controller has an encryption engine to transparently encrypt and decrypt
enclave memory.

In CPUs prior to Ice Lake, the Memory Encryption Engine (MEE) is used to
encrypt pages leaving the CPU caches. MEE uses a n-ary Merkle tree with root in
SRAM to maintain integrity of the encrypted data. This provides integrity and
anti-replay protection but does not scale to large memory sizes because the time
required to update the Merkle tree grows logarithmically in relation to the
memory size.

CPUs starting from Icelake use Total Memory Encryption (TME) in the place of
MEE. TME-based SGX implementations do not have an integrity Merkle tree, which
means integrity and replay-attacks are not mitigated. B, it includes
additional changes to prevent cipher text from being returned and SW memory
aliases from being created.

DMA to enclave memory is blocked by range registers on both MEE and TME systems
(SDM section 41.10).

## 33.7. Usage Models

### 33.7.1. Shared Library

Sensitive data and the code that acts on it is partitioned from the application
into a separate library. The library is then linked as a DSO which can be loaded
into an enclave. The application can then make individual function calls into
the enclave through special SGX instructions. A run-time within the enclave is
configured to marshal function parameters into and out of the enclave and to
call the correct library function.

### 33.7.2. Application Container

An application may be loaded into a container enclave which is specially
configured with a library OS and run-time which permits the application to run.
The enclave run-time and library OS work together to execute the application
when a thread enters the enclave.

## 33.8. Impact of Potential Kernel SGX Bugs

### 33.8.1. EPC leaks

When EPC page leaks happen, a WARNING like this is shown in dmesg:

“EREMOVE returned ... and an EPC page was leaked. SGX may become unusable...”

This is effectively a kernel use-after-free of an EPC page, and due
to the way SGX works, the bug is detected at freeing. Rather than
adding the page back to the pool of available EPC pages, the kernel
intentionally leaks the page to avoid additional errors in the future.

When this happens, the kernel will likely soon leak more EPC pages, and
SGX will likely become unusable because the memory available to SGX is
limited. However, while this may be fatal to SGX, the rest of the kernel
is unlikely to be impacted and should continue to work.

As a result, when this happens, user should stop running any new
SGX workloads, (or just any new workloads), and migrate all valuable
workloads. Although a machine reboot can recover all EPC memory, the bug
should be reported to Linux developers.

## 33.9. Virtual EPC

The implementation has also a virtual EPC driver to support SGX enclaves
in guests. Unlike the SGX driver, an EPC page allocated by the virtual
EPC driver doesn’t have a specific enclave associated with it. This is
because KVM doesn’t track how a guest uses EPC pages.

As a result, the SGX core page reclaimer doesn’t support reclaiming EPC
pages allocated to KVM guests through the virtual EPC driver. If the
user wants to deploy SGX applications both on the host and in guests
on the same machine, the user should reserve enough EPC (by taking out
total virtual EPC size of all SGX VMs from the physical EPC size) for
host SGX applications so they can run with acceptable performance.

Architectural behavior is to restore all EPC pages to an uninitialized
state also after a guest reboot. Because this state can be reached only
through the privileged `ENCLS[EREMOVE]` instruction, `/dev/sgx_vepc`
provides the `SGX_IOC_VEPC_REMOVE_ALL` ioctl to execute the instruction
on all pages in the virtual EPC.

`EREMOVE` can fail for three reasons. Userspace must pay attention
to expected failures and handle them as follows:

1. Page removal will always fail when any thread is running in the
   enclave to which the page belongs. In this case the ioctl will
   return `EBUSY` independent of whether it has successfully removed
   some pages; userspace can avoid these failures by preventing execution
   of any vcpu which maps the virtual EPC.
2. Page removal will cause a general protection fault if two calls to
   `EREMOVE` happen concurrently for pages that refer to the same
   “SECS” metadata pages. This can happen if there are concurrent
   invocations to `SGX_IOC_VEPC_REMOVE_ALL`, or if a `/dev/sgx_vepc`
   file descriptor in the guest is closed at the same time as
   `SGX_IOC_VEPC_REMOVE_ALL`; it will also be reported as `EBUSY`.
   This can be avoided in userspace by serializing calls to the ioctl()
   and to close(), but in general it should not be a problem.
3. Finally, page removal will fail for SECS metadata pages which still
   have child pages. Child pages can be removed by executing
   `SGX_IOC_VEPC_REMOVE_ALL` on all `/dev/sgx_vepc` file descriptors
   mapped into the guest. This means that the ioctl() must be called
   twice: an initial set of calls to remove child pages and a subsequent
   set of calls to remove SECS pages. The second set of calls is only
   required for those mappings that returned a nonzero value from the
   first call. It indicates a bug in the kernel or the userspace client
   if any of the second round of `SGX_IOC_VEPC_REMOVE_ALL` calls has
   a return code other than 0.
