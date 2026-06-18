# Measurement Registers

> 출처(원문): https://docs.kernel.org/driver-api/coco/measurement-registers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Measurement Registers

struct tsm\_measurement\_register
:   describes an architectural measurement register (MR)

**Definition**:

```
struct tsm_measurement_register {
    const char *mr_name;
    void *mr_value;
    u32 mr_size;
    u32 mr_flags;
    enum hash_algo mr_hash;
};
```

**Members**

`mr_name`
:   name of the MR

`mr_value`
:   buffer containing the current value of the MR

`mr_size`
:   size of the MR - typically the digest size of **mr\_hash**

`mr_flags`
:   bitwise OR of one or more flags, detailed below

`mr_hash`
:   optional hash identifier defined in include/uapi/linux/hash\_info.h.

**Description**

A CC guest driver encloses an array of this structure in [`struct
tsm_measurements`](#c.tsm_measurements "tsm_measurements") to detail the measurement facility supported by the
underlying CC hardware.

**mr\_name** and **mr\_value** must stay valid until this structure is no longer in
use.

**mr\_flags** is the bitwise-OR of zero or more of the flags below.

* `TSM_MR_F_READABLE` - the sysfs attribute corresponding to this MR is readable.
* `TSM_MR_F_WRITABLE` - the sysfs attribute corresponding to this MR is writable.
  The semantics is typically to extend the MR but could vary depending on the
  architecture and the MR.
* `TSM_MR_F_LIVE` - this MR’s value may differ from the last value written, so
  must be read back from the underlying CC hardware/firmware.
* `TSM_MR_F_RTMR` - bitwise-OR of `TSM_MR_F_LIVE` and `TSM_MR_F_WRITABLE`.
* `TSM_MR_F_NOHASH` - this MR does NOT have an associated hash algorithm.
  **mr\_hash** will be ignored when this flag is set.

struct tsm\_measurements
:   defines the CC architecture specific measurement facility and methods for updating measurement registers (MRs)

**Definition**:

```
struct tsm_measurements {
    const struct tsm_measurement_register *mrs;
    size_t nr_mrs;
    int (*refresh)(const struct tsm_measurements *tm);
    int (*write)(const struct tsm_measurements *tm, const struct tsm_measurement_register *mr, const u8 *data);
};
```

**Members**

`mrs`
:   Array of MR definitions.

`nr_mrs`
:   Number of elements in **mrs**.

`refresh`
:   Callback function to load/sync all MRs from TVM hardware/firmware
    into the kernel cache.

`write`
:   Callback function to write to the MR specified by the parameter **mr**.
    Typically, writing to an MR extends the input buffer to that MR.

**Description**

The **refresh** callback is invoked when an MR with `TSM_MR_F_LIVE` set is being
read and the cache is stale. It must reload all MRs with `TSM_MR_F_LIVE` set.
The function parameter **tm** is a pointer pointing back to this structure.

The **write** callback is invoked whenever an MR is being written. It takes two
additional parameters besides **tm**:

* **mr** - points to the MR (an element of **tm->mrs**) being written.
* **data** - contains the bytes to write and whose size is **mr->mr\_size**.

Both **refresh** and **write** should return 0 on success and an appropriate error
code on failure.

const struct attribute\_group \*tsm\_mr\_create\_attribute\_group(const struct [tsm\_measurements](#c.tsm_measurements "tsm_measurements") \*tm)
:   creates an attribute group for measurement registers (MRs)

**Parameters**

`const struct tsm_measurements *tm`
:   pointer to [`struct tsm_measurements`](#c.tsm_measurements "tsm_measurements") containing the MR definitions.

**Description**

This function creates attributes corresponding to the MR definitions
provided by **tm->mrs**.

The created attributes will reference **tm** and its members. The caller must
not free **tm** until after [`tsm_mr_free_attribute_group()`](#c.tsm_mr_free_attribute_group "tsm_mr_free_attribute_group") is called.

**Context**

Process context. May sleep due to memory allocation.

**Return**

* On success, the pointer to a an attribute group is returned; otherwise
* `-EINVAL` - Invalid MR definitions.
* `-ENOMEM` - Out of memory.

void tsm\_mr\_free\_attribute\_group(const struct attribute\_group \*attr\_grp)
:   frees the attribute group returned by [`tsm_mr_create_attribute_group()`](#c.tsm_mr_create_attribute_group "tsm_mr_create_attribute_group")

**Parameters**

`const struct attribute_group *attr_grp`
:   attribute group returned by [`tsm_mr_create_attribute_group()`](#c.tsm_mr_create_attribute_group "tsm_mr_create_attribute_group")

**Context**

Process context.
