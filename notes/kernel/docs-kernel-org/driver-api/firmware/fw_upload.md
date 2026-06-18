# Firmware Upload API

> 출처(원문): https://docs.kernel.org/driver-api/firmware/fw_upload.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Firmware Upload API

A device driver that registers with the firmware loader will expose
persistent sysfs nodes to enable users to initiate firmware updates for
that device. It is the responsibility of the device driver and/or the
device itself to perform any validation on the data received. Firmware
upload uses the same *loading* and *data* sysfs files described in the
documentation for firmware fallback. It also adds additional sysfs files
to provide status on the transfer of the firmware image to the device.

## Register for firmware upload

A device driver registers for firmware upload by calling
[`firmware_upload_register()`](#c.firmware_upload_register "firmware_upload_register"). Among the parameter list is a name to
identify the device under /sys/class/firmware. A user may initiate a
firmware upload by echoing a 1 to the *loading* sysfs file for the target
device. Next, the user writes the firmware image to the *data* sysfs
file. After writing the firmware data, the user echos 0 to the *loading*
sysfs file to signal completion. Echoing 0 to *loading* also triggers the
transfer of the firmware to the lower-lever device driver in the context
of a kernel worker thread.

To use the firmware upload API, write a driver that implements a set of
ops. The probe function calls [`firmware_upload_register()`](#c.firmware_upload_register "firmware_upload_register") and the remove
function calls [`firmware_upload_unregister()`](#c.firmware_upload_unregister "firmware_upload_unregister") such as:

```
static const struct fw_upload_ops m10bmc_ops = {
        .prepare = m10bmc_sec_prepare,
        .write = m10bmc_sec_write,
        .poll_complete = m10bmc_sec_poll_complete,
        .cancel = m10bmc_sec_cancel,
        .cleanup = m10bmc_sec_cleanup,
};

static int m10bmc_sec_probe(struct platform_device *pdev)
{
        const char *fw_name, *truncate;
        struct m10bmc_sec *sec;
        struct fw_upload *fwl;
        unsigned int len;

        sec = devm_kzalloc(&pdev->dev, sizeof(*sec), GFP_KERNEL);
        if (!sec)
                return -ENOMEM;

        sec->dev = &pdev->dev;
        sec->m10bmc = dev_get_drvdata(pdev->dev.parent);
        dev_set_drvdata(&pdev->dev, sec);

        fw_name = dev_name(sec->dev);
        truncate = strstr(fw_name, ".auto");
        len = (truncate) ? truncate - fw_name : strlen(fw_name);
        sec->fw_name = kmemdup_nul(fw_name, len, GFP_KERNEL);

        fwl = firmware_upload_register(THIS_MODULE, sec->dev, sec->fw_name,
                                       &m10bmc_ops, sec);
        if (IS_ERR(fwl)) {
                dev_err(sec->dev, "Firmware Upload driver failed to start\n");
                kfree(sec->fw_name);
                return PTR_ERR(fwl);
        }

        sec->fwl = fwl;
        return 0;
}

static int m10bmc_sec_remove(struct platform_device *pdev)
{
        struct m10bmc_sec *sec = dev_get_drvdata(&pdev->dev);

        firmware_upload_unregister(sec->fwl);
        kfree(sec->fw_name);
        return 0;
}
```

### firmware\_upload\_register

struct fw\_upload \*firmware\_upload\_register(struct [module](#c.firmware_upload_register "module") \*module, struct [device](../infrastructure.html#c.device "device") \*parent, const char \*name, const struct [fw\_upload\_ops](#c.fw_upload_ops "fw_upload_ops") \*ops, void \*dd\_handle)
:   register for the firmware upload sysfs API

**Parameters**

`struct module *module`
:   kernel module of this device

`struct device *parent`
:   parent device instantiating firmware upload

`const char *name`
:   firmware name to be associated with this device

`const struct fw_upload_ops *ops`
:   pointer to structure of firmware upload ops

`void *dd_handle`
:   pointer to parent driver private data

**Description**

> **name** must be unique among all users of firmware upload. The firmware
> sysfs files for this device will be found at /sys/class/firmware/**name**.

**Return**

`struct fw_upload` pointer or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")

### firmware\_upload\_unregister

void firmware\_upload\_unregister(struct [fw\_upload](#c.firmware_upload_unregister "fw_upload") \*fw\_upload)
:   Unregister firmware upload interface

**Parameters**

`struct fw_upload *fw_upload`
:   pointer to `struct fw_upload`

### Firmware Upload Ops

struct fw\_upload\_ops
:   device specific operations to support firmware upload

**Definition**:

```
struct fw_upload_ops {
    enum fw_upload_err (*prepare)(struct fw_upload *fw_upload, const u8 *data, u32 size);
    enum fw_upload_err (*write)(struct fw_upload *fw_upload, const u8 *data, u32 offset, u32 size, u32 *written);
    enum fw_upload_err (*poll_complete)(struct fw_upload *fw_upload);
    void (*cancel)(struct fw_upload *fw_upload);
    void (*cleanup)(struct fw_upload *fw_upload);
};
```

**Members**

`prepare`
:   Required: Prepare secure update

`write`
:   Required: The write() op receives the remaining
    size to be written and must return the actual
    size written or a negative error code. The write()
    op will be called repeatedly until all data is
    written.

`poll_complete`
:   Required: Check for the completion of the
    HW authentication/programming process.

`cancel`
:   Required: Request cancellation of update. This op
    is called from the context of a different kernel
    thread, so race conditions need to be considered.

`cleanup`
:   Optional: Complements the `prepare()`
    function and is called at the completion
    of the update, on success or failure, if the
    prepare function succeeded.

### Firmware Upload Progress Codes

The following progress codes are used internally by the firmware loader.
Corresponding strings are reported through the status sysfs node that
is described below and are documented in the ABI documentation.

enum fw\_upload\_prog
:   firmware upload progress codes

**Constants**

`FW_UPLOAD_PROG_IDLE`
:   there is no firmware upload in progress

`FW_UPLOAD_PROG_RECEIVING`
:   worker thread is receiving firmware data

`FW_UPLOAD_PROG_PREPARING`
:   target device is preparing for firmware upload

`FW_UPLOAD_PROG_TRANSFERRING`
:   data is being copied to the device

`FW_UPLOAD_PROG_PROGRAMMING`
:   device is performing the firmware update

`FW_UPLOAD_PROG_MAX`
:   Maximum progress code marker

### Firmware Upload Error Codes

The following error codes may be returned by the driver ops in case of
failure:

enum fw\_upload\_err
:   firmware upload error codes

**Constants**

`FW_UPLOAD_ERR_NONE`
:   returned to indicate success

`FW_UPLOAD_ERR_HW_ERROR`
:   error signalled by hardware, see kernel log

`FW_UPLOAD_ERR_TIMEOUT`
:   SW timed out on handshake with HW/firmware

`FW_UPLOAD_ERR_CANCELED`
:   upload was cancelled by the user

`FW_UPLOAD_ERR_BUSY`
:   there is an upload operation already in progress

`FW_UPLOAD_ERR_INVALID_SIZE`
:   invalid firmware image size

`FW_UPLOAD_ERR_RW_ERROR`
:   read or write to HW failed, see kernel log

`FW_UPLOAD_ERR_WEAROUT`
:   FLASH device is approaching wear-out, wait & retry

`FW_UPLOAD_ERR_FW_INVALID`
:   invalid firmware file

`FW_UPLOAD_ERR_MAX`
:   Maximum error code marker

## Sysfs Attributes

In addition to the *loading* and *data* sysfs files, there are additional
sysfs files to monitor the status of the data transfer to the target
device and to determine the final pass/fail status of the transfer.
Depending on the device and the size of the firmware image, a firmware
update could take milliseconds or minutes.

The additional sysfs files are:

* status - provides an indication of the progress of a firmware update
* error - provides error information for a failed firmware update
* remaining\_size - tracks the data transfer portion of an update
* cancel - echo 1 to this file to cancel the update
