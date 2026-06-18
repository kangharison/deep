# tcm_qla2xxx Driver Notes

> 출처(원문): https://docs.kernel.org/scsi/tcm_qla2xxx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# tcm\_qla2xxx Driver Notes

## tcm\_qla2xxx jam\_host attribute

There is now a new module endpoint attribute called jam\_host
attribute:

```
jam_host: boolean=0/1
```

This attribute and accompanying code is only included if the
Kconfig parameter TCM\_QLA2XXX\_DEBUG is set to Y

By default this jammer code and functionality is disabled

Use this attribute to control the discarding of SCSI commands to a
selected host.

This may be useful for testing error handling and simulating slow drain
and other fabric issues.

Setting a boolean of 1 for the jam\_host attribute for a particular host
will discard the commands for that host.

Reset back to 0 to stop the jamming.

Enable host 4 to be jammed:

```
echo 1 > /sys/kernel/config/target/qla2xxx/21:00:00:24:ff:27:8f:ae/tpgt_1/attrib/jam_host
```

Disable jamming on host 4:

```
echo 0 > /sys/kernel/config/target/qla2xxx/21:00:00:24:ff:27:8f:ae/tpgt_1/attrib/jam_host
```
