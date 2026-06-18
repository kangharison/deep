# Quota subsystem

> 출처(원문): https://docs.kernel.org/filesystems/quota.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Quota subsystem

Quota subsystem allows system administrator to set limits on used space and
number of used inodes (inode is a filesystem structure which is associated with
each file or directory) for users and/or groups. For both used space and number
of used inodes there are actually two limits. The first one is called softlimit
and the second one hardlimit. A user can never exceed a hardlimit for any
resource (unless he has CAP\_SYS\_RESOURCE capability). User is allowed to exceed
softlimit but only for limited period of time. This period is called “grace
period” or “grace time”. When grace time is over, user is not able to allocate
more space/inodes until he frees enough of them to get below softlimit.

Quota limits (and amount of grace time) are set independently for each
filesystem.

For more details about quota design, see the documentation in quota-tools package
(<https://sourceforge.net/projects/linuxquota>).

## Quota netlink interface

When user exceeds a softlimit, runs out of grace time or reaches hardlimit,
quota subsystem traditionally printed a message to the controlling terminal of
the process which caused the excess. This method has the disadvantage that
when user is using a graphical desktop he usually cannot see the message.
Thus quota netlink interface has been designed to pass information about
the above events to userspace. There they can be captured by an application
and processed accordingly.

The interface uses generic netlink framework (see
<https://lwn.net/Articles/208755/> and <http://www.infradead.org/~tgr/libnl/> for
more details about this layer). The name of the quota generic netlink interface
is “VFS\_DQUOT”. Definitions of constants below are in <linux/quota.h>. Since
the quota netlink protocol is not namespace aware, quota netlink messages are
sent only in initial network namespace.

Currently, the interface supports only one message type QUOTA\_NL\_C\_WARNING.
This command is used to send a notification about any of the above mentioned
events. Each message has six attributes. These are (type of the argument is
in parentheses):

> QUOTA\_NL\_A\_QTYPE (u32)
> :   * type of quota being exceeded (one of USRQUOTA, GRPQUOTA)
>
> QUOTA\_NL\_A\_EXCESS\_ID (u64)
> :   * UID/GID (depends on quota type) of user / group whose limit
>       is being exceeded.
>
> QUOTA\_NL\_A\_CAUSED\_ID (u64)
> :   * UID of a user who caused the event
>
> QUOTA\_NL\_A\_WARNING (u32)
> :   * what kind of limit is exceeded:
>
>       > QUOTA\_NL\_IHARDWARN
>       > :   inode hardlimit
>       >
>       > QUOTA\_NL\_ISOFTLONGWARN
>       > :   inode softlimit is exceeded longer
>       >     than given grace period
>       >
>       > QUOTA\_NL\_ISOFTWARN
>       > :   inode softlimit
>       >
>       > QUOTA\_NL\_BHARDWARN
>       > :   space (block) hardlimit
>       >
>       > QUOTA\_NL\_BSOFTLONGWARN
>       > :   space (block) softlimit is exceeded
>       >     longer than given grace period.
>       >
>       > QUOTA\_NL\_BSOFTWARN
>       > :   space (block) softlimit
>     * four warnings are also defined for the event when user stops
>       exceeding some limit:
>
>       > QUOTA\_NL\_IHARDBELOW
>       > :   inode hardlimit
>       >
>       > QUOTA\_NL\_ISOFTBELOW
>       > :   inode softlimit
>       >
>       > QUOTA\_NL\_BHARDBELOW
>       > :   space (block) hardlimit
>       >
>       > QUOTA\_NL\_BSOFTBELOW
>       > :   space (block) softlimit
>
> QUOTA\_NL\_A\_DEV\_MAJOR (u32)
> :   * major number of a device with the affected filesystem
>
> QUOTA\_NL\_A\_DEV\_MINOR (u32)
> :   * minor number of a device with the affected filesystem
