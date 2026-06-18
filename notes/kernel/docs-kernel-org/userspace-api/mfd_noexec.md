# Introduction of non-executable mfd

> 출처(원문): https://docs.kernel.org/userspace-api/mfd_noexec.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Introduction of non-executable mfd

Author:
:   Daniel Verkamp <[dverkamp@chromium.org](mailto:dverkamp%40chromium.org)>
    Jeff Xu <[jeffxu@chromium.org](mailto:jeffxu%40chromium.org)>

Contributor:
:   Aleksa Sarai <[cyphar@cyphar.com](mailto:cyphar%40cyphar.com)>

Since Linux introduced the memfd feature, memfds have always had their
execute bit set, and the `memfd_create()` syscall doesn’t allow setting
it differently.

However, in a secure-by-default system, such as ChromeOS, (where all
executables should come from the rootfs, which is protected by verified
boot), this executable nature of memfd opens a door for NoExec bypass
and enables “confused deputy attack”. E.g, in VRP bug [1]: cros\_vm
process created a memfd to share the content with an external process,
however the memfd is overwritten and used for executing arbitrary code
and root escalation. [2] lists more VRP of this kind.

On the other hand, executable memfd has its legit use: runc uses memfd’s
seal and executable feature to copy the contents of the binary then
execute them. For such a system, we need a solution to differentiate runc’s
use of executable memfds and an attacker’s [3].

To address those above:
:   * Let `memfd_create()` set X bit at creation time.
    * Let memfd be sealed for modifying X bit when NX is set.
    * Add a new pid namespace sysctl: vm.memfd\_noexec to help applications in
      migrating and enforcing non-executable MFD.

## User API

`int memfd_create(const char *name, unsigned int flags)`

`MFD_NOEXEC_SEAL`
:   When MFD\_NOEXEC\_SEAL bit is set in the `flags`, memfd is created
    with NX. F\_SEAL\_EXEC is set and the memfd can’t be modified to
    add X later. MFD\_ALLOW\_SEALING is also implied.
    This is the most common case for the application to use memfd.

`MFD_EXEC`
:   When MFD\_EXEC bit is set in the `flags`, memfd is created with X.

Note:
:   `MFD_NOEXEC_SEAL` implies `MFD_ALLOW_SEALING`. In case that
    an app doesn’t want sealing, it can add F\_SEAL\_SEAL after creation.

## Sysctl:

`pid namespaced sysctl vm.memfd_noexec`

The new pid namespaced sysctl vm.memfd\_noexec has 3 values:

> * 0: MEMFD\_NOEXEC\_SCOPE\_EXEC
>   :   `memfd_create()` without MFD\_EXEC nor MFD\_NOEXEC\_SEAL acts like
>       MFD\_EXEC was set.
> * 1: MEMFD\_NOEXEC\_SCOPE\_NOEXEC\_SEAL
>   :   `memfd_create()` without MFD\_EXEC nor MFD\_NOEXEC\_SEAL acts like
>       MFD\_NOEXEC\_SEAL was set.
> * 2: MEMFD\_NOEXEC\_SCOPE\_NOEXEC\_ENFORCED
>   :   `memfd_create()` without MFD\_NOEXEC\_SEAL will be rejected.

The sysctl allows finer control of memfd\_create for old software that
doesn’t set the executable bit; for example, a container with
vm.memfd\_noexec=1 means the old software will create non-executable memfd
by default while new software can create executable memfd by setting
MFD\_EXEC.

The value of vm.memfd\_noexec is passed to child namespace at creation
time. In addition, the setting is hierarchical, i.e. during memfd\_create,
we will search from current ns to root ns and use the most restrictive
setting.

[1] <https://crbug.com/1305267>

[2] <https://bugs.chromium.org/p/chromium/issues/list?q=type%3Dbug-security%20memfd%20escalation&can=1>

[3] <https://lwn.net/Articles/781013/>
