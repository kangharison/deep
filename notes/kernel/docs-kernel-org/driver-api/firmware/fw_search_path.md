# Firmware search paths

> 출처(원문): https://docs.kernel.org/driver-api/firmware/fw_search_path.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Firmware search paths

The following search paths are used to look for firmware on your
root filesystem.

* fw\_path\_para - module parameter - default is empty so this is ignored
* /lib/firmware/updates/UTS\_RELEASE/
* /lib/firmware/updates/
* /lib/firmware/UTS\_RELEASE/
* /lib/firmware/

The module parameter ‘’path’’ can be passed to the firmware\_class module
to activate the first optional custom fw\_path\_para. The custom path can
only be up to 256 characters long. The kernel parameter passed would be:

* ‘firmware\_class.path=$CUSTOMIZED\_PATH’

There is an alternative to customize the path at run time after bootup, you
can use the file:

* /sys/module/firmware\_class/parameters/path

You would echo into it your custom path and firmware requested will be searched
for there first. Be aware that newline characters will be taken into account
and may not produce the intended effects. For instance you might want to use:

echo -n /path/to/script > /sys/module/firmware\_class/parameters/path

to ensure that your script is being used.
