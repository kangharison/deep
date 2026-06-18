# User namespaces and resource control

> 출처(원문): https://docs.kernel.org/admin-guide/namespaces/resource-control.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# User namespaces and resource control

The kernel contains many kinds of objects that either don’t have
individual limits or that have limits which are ineffective when
a set of processes is allowed to switch their UID. On a system
where the admins don’t trust their users or their users’ programs,
user namespaces expose the system to potential misuse of resources.

In order to mitigate this, we recommend that admins enable memory
control groups on any system that enables user namespaces.
Furthermore, we recommend that admins configure the memory control
groups to limit the maximum memory usable by any untrusted user.

Memory control groups can be configured by installing the libcgroup
package present on most distros editing /etc/cgrules.conf,
/etc/cgconfig.conf and setting up libpam-cgroup.
