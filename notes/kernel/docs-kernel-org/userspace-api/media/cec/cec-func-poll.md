# 2.4.cec poll()

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-func-poll.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4. cec poll()

## 2.4.1. Name

cec-poll - Wait for some event on a file descriptor

## 2.4.2. Synopsis

```
#include <sys/poll.h>
```

int poll(struct pollfd \*ufds, unsigned int nfds, int timeout)

## 2.4.3. Arguments

`ufds`
:   List of FD events to be watched

`nfds`
:   Number of FD events at the \*ufds array

`timeout`
:   Timeout to wait for events

## 2.4.4. Description

With the [`poll()`](#c.CEC.poll "poll") function applications can wait for CEC
events.

On success [`poll()`](#c.CEC.poll "poll") returns the number of file descriptors
that have been selected (that is, file descriptors for which the
`revents` field of the respective struct `pollfd`
is non-zero). CEC devices set the `POLLIN` and `POLLRDNORM` flags in
the `revents` field if there are messages in the receive queue. If the
transmit queue has room for new messages, the `POLLOUT` and
`POLLWRNORM` flags are set. If there are events in the event queue,
then the `POLLPRI` flag is set. When the function times out it returns
a value of zero, on failure it returns -1 and the `errno` variable is
set appropriately.

For more details see the [`poll()`](#c.CEC.poll "poll") manual page.

## 2.4.5. Return Value

On success, [`poll()`](#c.CEC.poll "poll") returns the number structures which have
non-zero `revents` fields, or zero if the call timed out. On error -1
is returned, and the `errno` variable is set appropriately:

`EBADF`
:   One or more of the `ufds` members specify an invalid file
    descriptor.

`EFAULT`
:   `ufds` references an inaccessible memory area.

`EINTR`
:   The call was interrupted by a signal.

`EINVAL`
:   The `nfds` value exceeds the `RLIMIT_NOFILE` value. Use
    `getrlimit()` to obtain this value.
