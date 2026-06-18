# PCMCIA Driver

> 출처(원문): https://docs.kernel.org/pcmcia/driver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# PCMCIA Driver

## sysfs

New PCMCIA IDs may be added to a device driver pcmcia\_device\_id table at
runtime as shown below:

```
echo "match_flags manf_id card_id func_id function device_no \
prod_id_hash[0] prod_id_hash[1] prod_id_hash[2] prod_id_hash[3]" > \
/sys/bus/pcmcia/drivers/{driver}/new_id
```

All fields are passed in as hexadecimal values (no leading 0x).
The meaning is described in the PCMCIA specification, the match\_flags is
a bitwise or-ed combination from PCMCIA\_DEV\_ID\_MATCH\_\* constants
defined in include/linux/mod\_devicetable.h.

Once added, the driver probe routine will be invoked for any unclaimed
PCMCIA device listed in its (newly updated) pcmcia\_device\_id list.

A common use-case is to add a new device according to the manufacturer ID
and the card ID (form the manf\_id and card\_id file in the device tree).
For this, just use:

```
echo "0x3 manf_id card_id 0 0 0 0 0 0 0" > \
  /sys/bus/pcmcia/drivers/{driver}/new_id
```

after loading the driver.
