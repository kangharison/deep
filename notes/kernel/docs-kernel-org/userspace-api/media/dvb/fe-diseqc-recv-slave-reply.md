# 2.4.8.ioctl FE_DISEQC_RECV_SLAVE_REPLY

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-diseqc-recv-slave-reply.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.8. ioctl FE\_DISEQC\_RECV\_SLAVE\_REPLY

## 2.4.8.1. Name

FE\_DISEQC\_RECV\_SLAVE\_REPLY - Receives reply from a DiSEqC 2.0 command

## 2.4.8.2. Synopsis

FE\_DISEQC\_RECV\_SLAVE\_REPLY

`int ioctl(int fd, FE_DISEQC_RECV_SLAVE_REPLY, struct dvb_diseqc_slave_reply *argp)`

## 2.4.8.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`argp`
:   pointer to struct [`dvb_diseqc_slave_reply`](frontend-header.html#c.dvb_diseqc_slave_reply "dvb_diseqc_slave_reply").

## 2.4.8.4. Description

Receives reply from a DiSEqC 2.0 command.

The received message is stored at the buffer pointed by `argp`.

## 2.4.8.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
