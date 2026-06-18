# 2.4.7.ioctl FE_DISEQC_SEND_MASTER_CMD

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-diseqc-send-master-cmd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.7. ioctl FE\_DISEQC\_SEND\_MASTER\_CMD

## 2.4.7.1. Name

FE\_DISEQC\_SEND\_MASTER\_CMD - Sends a DiSEqC command

## 2.4.7.2. Synopsis

FE\_DISEQC\_SEND\_MASTER\_CMD

`int ioctl(int fd, FE_DISEQC_SEND_MASTER_CMD, struct dvb_diseqc_master_cmd *argp)`

## 2.4.7.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`argp`
:   pointer to struct
    [`dvb_diseqc_master_cmd`](frontend-header.html#c.dvb_diseqc_master_cmd "dvb_diseqc_master_cmd")

## 2.4.7.4. Description

Sends the DiSEqC command pointed by [`dvb_diseqc_master_cmd`](frontend-header.html#c.dvb_diseqc_master_cmd "dvb_diseqc_master_cmd")
to the antenna subsystem.

## 2.4.7.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
