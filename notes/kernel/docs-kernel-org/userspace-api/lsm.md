# Linux Security Modules

> 출처(원문): https://docs.kernel.org/userspace-api/lsm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Security Modules

Author:
:   Casey Schaufler

Date:
:   July 2023

Linux security modules (LSM) provide a mechanism to implement
additional access controls to the Linux security policies.

The various security modules may support any of these attributes:

`LSM_ATTR_CURRENT` is the current, active security context of the
process.
The proc filesystem provides this value in `/proc/self/attr/current`.
This is supported by the SELinux, Smack and AppArmor security modules.
Smack also provides this value in `/proc/self/attr/smack/current`.
AppArmor also provides this value in `/proc/self/attr/apparmor/current`.

`LSM_ATTR_EXEC` is the security context of the process at the time the
current image was executed.
The proc filesystem provides this value in `/proc/self/attr/exec`.
This is supported by the SELinux and AppArmor security modules.
AppArmor also provides this value in `/proc/self/attr/apparmor/exec`.

`LSM_ATTR_FSCREATE` is the security context of the process used when
creating file system objects.
The proc filesystem provides this value in `/proc/self/attr/fscreate`.
This is supported by the SELinux security module.

`LSM_ATTR_KEYCREATE` is the security context of the process used when
creating key objects.
The proc filesystem provides this value in `/proc/self/attr/keycreate`.
This is supported by the SELinux security module.

`LSM_ATTR_PREV` is the security context of the process at the time the
current security context was set.
The proc filesystem provides this value in `/proc/self/attr/prev`.
This is supported by the SELinux and AppArmor security modules.
AppArmor also provides this value in `/proc/self/attr/apparmor/prev`.

`LSM_ATTR_SOCKCREATE` is the security context of the process used when
creating socket objects.
The proc filesystem provides this value in `/proc/self/attr/sockcreate`.
This is supported by the SELinux security module.

## Kernel interface

### Set a security attribute of the current process

long sys\_lsm\_set\_self\_attr(unsigned int attr, struct lsm\_ctx \_\_user \*ctx, u32 size, u32 flags)
:   Set current task’s security module attribute

**Parameters**

`unsigned int attr`
:   which attribute to set

`struct lsm_ctx __user * ctx`
:   the LSM contexts

`u32 size`
:   size of **ctx**

`u32 flags`
:   reserved for future use

**Description**

Sets the calling task’s LSM context. On success this function
returns 0. If the attribute specified cannot be set a negative
value indicating the reason for the error is returned.

### Get the specified security attributes of the current process

long sys\_lsm\_get\_self\_attr(unsigned int attr, struct lsm\_ctx \_\_user \*ctx, u32 \_\_user \*size, u32 flags)
:   Return current task’s security module attributes

**Parameters**

`unsigned int attr`
:   which attribute to return

`struct lsm_ctx __user * ctx`
:   the user-space destination for the information, or NULL

`u32 __user * size`
:   pointer to the size of space available to receive the data

`u32 flags`
:   special handling options. LSM\_FLAG\_SINGLE indicates that only
    attributes associated with the LSM identified in the passed **ctx** be
    reported.

**Description**

Returns the calling task’s LSM contexts. On success this
function returns the number of **ctx** array elements. This value
may be zero if there are no LSM contexts assigned. If **size** is
insufficient to contain the return data -E2BIG is returned and
**size** is set to the minimum required size. In all other cases
a negative value indicating the error is returned.

long sys\_lsm\_list\_modules(u64 \_\_user \*ids, u32 \_\_user \*size, u32 flags)
:   Return a list of the active security modules

**Parameters**

`u64 __user * ids`
:   the LSM module ids

`u32 __user * size`
:   pointer to size of **ids**, updated on return

`u32 flags`
:   reserved for future use, must be zero

**Description**

Returns a list of the active LSM ids. On success this function
returns the number of **ids** array elements. This value may be zero
if there are no LSMs active. If **size** is insufficient to contain
the return data -E2BIG is returned and **size** is set to the minimum
required size. In all other cases a negative value indicating the
error is returned.

## Additional documentation

* [Linux Security Modules: General Security Hooks for Linux](../security/lsm.html)
* [Linux Security Module Development](../security/lsm-development.html)
