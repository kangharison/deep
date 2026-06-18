# ABI obsolete symbols

> 출처(원문): https://docs.kernel.org/admin-guide/abi-obsolete.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ABI obsolete symbols

Documents interfaces that are still remaining in the kernel, but are
marked to be removed at some later point in time.

The description of the interface will document the reason why it is
obsolete and when it can be expected to be removed.

## Symbols under /proc/i8k

|  |
| --- |
| **/proc/i8k** |

Defined on file [procfs-i8k](abi-obsolete-files.html#abi-file-obsolete-procfs-i8k)

Legacy interface for getting/setting sensor information like
fan speed, temperature, serial number, hotkey status etc
on Dell Laptops.
Since the driver is now using the standard hwmon sysfs interface,
the procfs interface is deprecated.

Users:
:   <https://github.com/vitorafsr/i8kutils>

## Symbols under /sys

|  |
| --- |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_incli\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltage\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY\_supply\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_timestamp\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_pressureY\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_pressure\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_quaternion\_type** |
| **/sys/.../iio:deviceX/scan\_elements/in\_proximity\_type** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

Description of the scan element data storage within the buffer
and hence the form in which it is read from user-space.
Form is [be|le]:[s|u]bits/storagebits[>>shift].
be or le specifies big or little endian. s or u specifies if
signed (2’s complement) or unsigned. bits is the number of bits
of data and storagebits is the space (after padding) that it
occupies in the buffer. shift if specified, is the shift that
needs to be applied prior to masking out unused bits. Some
devices put their data in the middle of the transferred elements
with additional information on both sides. Note that some
devices will have additional information in the unused bits
so to get a clean value, the bits value must be used to mask
the buffer output value appropriately. The storagebits value
also specifies the data alignment. So s48/64>>2 will be a
signed 48 bit integer stored in a 64 bit location aligned to
a 64 bit boundary. To obtain the clean value, shift right 2
and apply a mask to zero the top 16 bits of the result.
For other storage combinations this attribute will be extended
appropriately.

Since kernel 5.11 the scan\_elements attributes are merged into
the bufferY directory, to be configurable per buffer.

|  |
| --- |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_x\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_y\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_z\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_x\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_y\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_z\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_x\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_y\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_z\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_magnetic\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_true\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_magnetic\_tilt\_comp\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_true\_tilt\_comp\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_timestamp\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY\_supply\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY-voltageZ\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_incli\_x\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_incli\_y\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_pressureY\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_pressure\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_quaternion\_en** |
| **/sys/.../iio:deviceX/scan\_elements/in\_proximity\_en** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

Scan element control for triggered data capture.

Since kernel 5.11 the scan\_elements attributes are merged into
the bufferY directory, to be configurable per buffer.

|  |
| --- |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_voltageY\_supply\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_x\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_y\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_accel\_z\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_x\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_y\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_anglvel\_z\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_x\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_y\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_magn\_z\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_magnetic\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_true\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_magnetic\_tilt\_comp\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_from\_north\_true\_tilt\_comp\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_incli\_x\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_incli\_y\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_timestamp\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_pressureY\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_pressure\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_rot\_quaternion\_index** |
| **/sys/.../iio:deviceX/scan\_elements/in\_proximity\_index** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

A single positive integer specifying the position of this
scan element in the buffer. Note these are not dependent on
what is enabled and may not be contiguous. Thus for user-space
to establish the full layout these must be used in conjunction
with all \_en attributes to establish which channels are present,
and the relevant \_type attributes to establish the data storage
format.

Since kernel 5.11 the scan\_elements attributes are merged into
the bufferY directory, to be configurable per buffer.

## Symbols under /sys/bus

|  |
| --- |
| **/sys/bus/iio/devices/iio:deviceX/buffer/data\_available** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

A read-only value indicating the bytes of data available in the
buffer. In the case of an output buffer, this indicates the
amount of empty space available to write data to. In the case of
an input buffer, this indicates the amount of data available for
reading.

Since Kernel 5.11, multiple buffers are supported.
so, it is better to use, instead:

> /sys/bus/iio/devices/iio:deviceX/bufferY/data\_available

|  |
| --- |
| **/sys/bus/iio/devices/iio:deviceX/buffer/enable** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

Actually start the buffer capture up. Will start trigger
if first device and appropriate.

Since Kernel 5.11, multiple buffers are supported.
so, it is better to use, instead:

> /sys/bus/iio/devices/iio:deviceX/bufferY/enable

|  |
| --- |
| **/sys/bus/iio/devices/iio:deviceX/buffer/length** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

Number of scans contained by the buffer.

Since Kernel 5.11, multiple buffers are supported.
so, it is better to use, instead:

> /sys/bus/iio/devices/iio:deviceX/bufferY/length

|  |
| --- |
| **/sys/bus/iio/devices/iio:deviceX/buffer/watermark** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

A single positive integer specifying the maximum number of scan
elements to wait for.

Poll will block until the watermark is reached.

Blocking read will wait until the minimum between the requested
read amount or the low water mark is available.

Non-blocking read will retrieve the available samples from the
buffer even if there are less samples then watermark level. This
allows the application to block on poll with a timeout and read
the available samples after the timeout expires and thus have a
maximum delay guarantee.

Since Kernel 5.11, multiple buffers are supported.
so, it is better to use, instead:

> /sys/bus/iio/devices/iio:deviceX/bufferY/watermark

|  |
| --- |
| **/sys/bus/iio/devices/iio:deviceX/scan\_elements** |

Defined on file [sysfs-bus-iio](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-iio)

Directory containing interfaces for elements that will be
captured for a single triggered sample set in the buffer.

Since kernel 5.11 the scan\_elements attributes are merged into
the bufferY directory, to be configurable per buffer.

|  |
| --- |
| **/sys/bus/platform/devices/INT34D2:00/northpeak** |

Defined on file [sysfs-driver-intel\_pmc\_bxt](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-intel-pmc-bxt)

This interface allows userspace to enable and disable
Northpeak through the PMC/SCU.

Format: %u.

|  |
| --- |
| **/sys/bus/platform/devices/INT34D2:00/simplecmd** |

Defined on file [sysfs-driver-intel\_pmc\_bxt](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-intel-pmc-bxt)

This interface allows userspace to send an arbitrary
IPC command to the PMC/SCU.

Format: %d %d where first number is command and
second number is subcommand.

|  |
| --- |
| **/sys/bus/platform/devices/VPC2004:\*/conservation\_mode** |

Defined on file [sysfs-platform-ideapad-laptop](abi-obsolete-files.html#abi-file-obsolete-sysfs-platform-ideapad-laptop)

Controls whether the conservation mode is enabled or not.
This feature limits the maximum battery charge percentage to
around 50-60% in order to prolong the lifetime of the battery.

|  |
| --- |
| **/sys/bus/usb/devices/.../power/level** |

Defined on file [sysfs-bus-usb](abi-obsolete-files.html#abi-file-obsolete-sysfs-bus-usb)

Each USB device directory will contain a file named
power/level. This file holds a power-level setting for
the device, either “on” or “auto”.

“on” means that the device is not allowed to autosuspend,
although normal suspends for system sleep will still
be honored. “auto” means the device will autosuspend
and autoresume in the usual manner, according to the
capabilities of its driver.

During normal use, devices should be left in the “auto”
level. The “on” level is meant for administrative uses.
If you want to suspend a device immediately but leave it
free to wake up in response to I/O requests, you should
write “0” to power/autosuspend.

Device not capable of proper suspend and resume should be
left in the “on” level. Although the USB spec requires
devices to support suspend/resume, many of them do not.
In fact so many don’t that by default, the USB core
initializes all non-hub devices in the “on” level. Some
drivers may change this setting when they are bound.

This file is deprecated and will be removed after 2010.
Use the power/control file instead; it does exactly the
same thing.

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/arvo/roccatarvo<minor>/actual\_profile** |

Defined on file [sysfs-driver-hid-roccat-arvo](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-arvo)

The integer value of this attribute ranges from 1-5.
When read, this attribute returns the number of the actual
profile which is also the profile that’s active on device startup.
When written this attribute activates the selected profile
immediately.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/arvo/roccatarvo<minor>/button** |

Defined on file [sysfs-driver-hid-roccat-arvo](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-arvo)

The keyboard can store short macros with consist of 1 button with
several modifier keys internally.
When written, this file lets one set the sequence for a specific
button for a specific profile. Button and profile numbers are
included in written data. The data has to be 24 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/arvo/roccatarvo<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-arvo](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-arvo)

When read, this file returns some info about the device like the
installed firmware version.
The size of the data is 8 bytes in size.
This file is readonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/arvo/roccatarvo<minor>/key\_mask** |

Defined on file [sysfs-driver-hid-roccat-arvo](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-arvo)

The keyboard lets the user deactivate 5 certain keys like the
windows and application keys, to protect the user from the outcome
of accidentally pressing them.
The integer value of this attribute has bits 0-4 set depending
on the state of the corresponding key.
When read, this file returns the current state of the buttons.
When written, the given buttons are activated/deactivated
immediately.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/arvo/roccatarvo<minor>/mode\_key** |

Defined on file [sysfs-driver-hid-roccat-arvo](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-arvo)

The keyboard has a condensed layout without num-lock key.
Instead it uses a mode-key which activates a gaming mode where
the assignment of the number block changes.
The integer value of this attribute ranges from 0 (OFF) to 1 (ON).
When read, this file returns the actual state of the key.
When written, the key is activated/deactivated immediately.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/actual\_profile** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

The integer value of this attribute ranges from 0-4.
When read, this attribute returns the number of the actual
profile. This value is persistent, so its equivalent to the
profile that’s active when the device is powered on next time.
When written, this file sets the number of the startup profile
and the device activates this profile immediately.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/control** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one select which data from which
profile will be read next. The data has to be 3 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When read, this file returns general data like firmware version.
The data is 6 bytes long.
This file is readonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/key\_mask** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one deactivate certain keys like
windows and application keys, to prevent accidental presses.
Profile number for which this settings occur is included in
written data. The data has to be 6 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/keys\_capslock** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the function of the
capslock key for a specific profile. Profile number is included
in written data. The data has to be 6 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/keys\_easyzone** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the function of the
easyzone keys for a specific profile. Profile number is included
in written data. The data has to be 65 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/keys\_function** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the function of the
function keys for a specific profile. Profile number is included
in written data. The data has to be 41 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/keys\_macro** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the function of the macro
keys for a specific profile. Profile number is included in
written data. The data has to be 35 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/keys\_media** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the function of the media
keys for a specific profile. Profile number is included in
written data. The data has to be 29 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/keys\_thumbster** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the function of the
thumbster keys for a specific profile. Profile number is included
in written data. The data has to be 23 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/last\_set** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the time in secs since
epoch in which the last configuration took place.
The data has to be 20 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/light** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one set the backlight intensity for
a specific profile. Profile number is included in written data.
The data has to be 10 bytes long for Isku, IskuFX needs 16 bytes
of data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/macro** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one store macros with max 500
keystrokes for a specific button for a specific profile.
Button and profile numbers are included in written data.
The data has to be 2083 bytes long.
Before reading this file, control has to be written to select
which profile and key to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/reset** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one reset the device.
The data has to be 3 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/talk** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one trigger easyshift functionality
from the host.
The data has to be 16 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/isku/roccatisku<minor>/talkfx** |

Defined on file [sysfs-driver-hid-roccat-isku](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-isku)

When written, this file lets one trigger temporary color schemes
from the host.
The data has to be 16 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/actual\_profile** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The integer value of this attribute ranges from 0-4.
When read, this attribute returns the number of the actual
profile. This value is persistent, so its equivalent to the
profile that’s active when the mouse is powered on next time.
When written, this file sets the number of the startup profile
and the mouse activates this profile immediately.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/firmware\_version** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

When read, this file returns the raw integer version number of the
firmware reported by the mouse. Using the integer value eases
further usage in other programs. To receive the real version
number the decimal point has to be shifted 2 positions to the
left. E.g. a returned value of 121 means 1.21
This file is readonly.
Please read binary attribute info which contains firmware version.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

When read, this file returns general data like firmware version.
When written, the device can be reset.
The data is 8 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/macro** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The mouse can store a macro with max 500 key/button strokes
internally.
When written, this file lets one set the sequence for a specific
button for a specific profile. Button and profile numbers are
included in written data. The data has to be 2082 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/profile[1-5]\_buttons** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When read, these files return the respective profile buttons.
The returned data is 77 bytes in size.
This file is readonly.
Write control to select profile and read profile\_buttons instead.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/profile[1-5]\_settings** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When read, these files return the respective profile settings.
The returned data is 43 bytes in size.
This file is readonly.
Write control to select profile and read profile\_settings instead.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/profile\_buttons** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When written, this file lets one write the respective profile
buttons back to the mouse. The data has to be 77 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/profile\_settings** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When written, this file lets one write the respective profile
settings back to the mouse. The data has to be 43 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/sensor** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The mouse has a tracking- and a distance-control-unit. These
can be activated/deactivated and the lift-off distance can be
set. The data has to be 6 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/startup\_profile** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

The integer value of this attribute ranges from 0-4.
When read, this attribute returns the number of the actual
profile. This value is persistent, so its equivalent to the
profile that’s active when the mouse is powered on next time.
When written, this file sets the number of the startup profile
and the mouse activates this profile immediately.
Please use actual\_profile, it does the same thing.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/talk** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

Used to active some easy\* functions of the mouse from outside.
The data has to be 16 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/tcu** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

When written a calibration process for the tracking control unit
can be initiated/cancelled. Also lets one read/write sensor
registers.
The data has to be 4 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/koneplus/roccatkoneplus<minor>/tcu\_image** |

Defined on file [sysfs-driver-hid-roccat-koneplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-koneplus)

When read the mouse returns a 30x30 pixel image of the
sampled underground. This works only in the course of a
calibration process initiated with tcu.
The returned data is 1028 bytes in size.
This file is readonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/actual\_profile** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

The mouse can store 5 profiles which can be switched by the
press of a button. actual\_profile holds number of actual profile.
This value is persistent, so its value determines the profile
that’s active when the mouse is powered on next time.
When written, the mouse activates the set profile immediately.
The data has to be 3 bytes long.
The mouse will reject invalid data.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/control** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

When written, this file lets one select which data from which
profile will be read next. The data has to be 3 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

When read, this file returns general data like firmware version.
When written, the device can be reset.
The data is 6 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/macro** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

The mouse can store a macro with max 500 key/button strokes
internally.
When written, this file lets one set the sequence for a specific
button for a specific profile. Button and profile numbers are
included in written data. The data has to be 2082 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/profile\_buttons** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When written, this file lets one write the respective profile
buttons back to the mouse. The data has to be 59 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/profile\_settings** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When written, this file lets one write the respective profile
settings back to the mouse. The data has to be 31 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/sensor** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

The mouse has a tracking- and a distance-control-unit. These
can be activated/deactivated and the lift-off distance can be
set. The data has to be 6 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/talk** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

Used to active some easy\* functions of the mouse from outside.
The data has to be 16 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/tcu** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

When written a calibration process for the tracking control unit
can be initiated/cancelled. Also lets one read/write sensor
registers.
The data has to be 4 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/konepure/roccatkonepure<minor>/tcu\_image** |

Defined on file [sysfs-driver-hid-roccat-konepure](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-konepure)

When read the mouse returns a 30x30 pixel image of the
sampled underground. This works only in the course of a
calibration process initiated with tcu.
The returned data is 1028 bytes in size.
This file is readonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/actual\_cpi** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The integer value of this attribute ranges from 1-4.
When read, this attribute returns the number of the active
cpi level.
This file is readonly.
Has never been used. If bookkeeping is done, it’s done in userland tools.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/actual\_profile** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The integer value of this attribute ranges from 0-4.
When read, this attribute returns the number of the active
profile.
When written, the mouse activates this profile immediately.
The profile that’s active when powered down is the same that’s
active when the mouse is powered on.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/actual\_sensitivity\_x** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The integer value of this attribute ranges from 1-10.
When read, this attribute returns the number of the actual
sensitivity in x direction.
This file is readonly.
Has never been used. If bookkeeping is done, it’s done in userland tools.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/actual\_sensitivity\_y** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The integer value of this attribute ranges from 1-10.
When read, this attribute returns the number of the actual
sensitivity in y direction.
This file is readonly.
Has never been used. If bookkeeping is done, it’s done in userland tools.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/firmware\_version** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

When read, this file returns the raw integer version number of the
firmware reported by the mouse. Using the integer value eases
further usage in other programs. To receive the real version
number the decimal point has to be shifted 2 positions to the
left. E.g. a returned value of 121 means 1.21
This file is readonly.
Obsoleted by binary sysfs attribute “info”.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

When read, this file returns general data like firmware version.
When written, the device can be reset.
The data is 6 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/profile[1-5]\_buttons** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When read, these files return the respective profile buttons.
The returned data is 23 bytes in size.
This file is readonly.
Write control to select profile and read profile\_buttons instead.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/profile[1-5]\_settings** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When read, these files return the respective profile settings.
The returned data is 16 bytes in size.
This file is readonly.
Write control to select profile and read profile\_settings instead.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/profile\_buttons** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When written, this file lets one write the respective profile
buttons back to the mouse. The data has to be 23 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/kovaplus/roccatkovaplus<minor>/profile\_settings** |

Defined on file [sysfs-driver-hid-roccat-kovaplus](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-kovaplus)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When written, this file lets one write the respective profile
settings back to the mouse. The data has to be 16 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/actual\_cpi** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

It is possible to switch the cpi setting of the mouse with the
press of a button.
When read, this file returns the raw number of the actual cpi
setting reported by the mouse. This number has to be further
processed to receive the real dpi value:

| VALUE | DPI |
| --- | --- |
| 1 | 400 |
| 2 | 800 |
| 4 | 1600 |

This file is readonly.
Has never been used. If bookkeeping is done, it’s done in userland tools.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/actual\_profile** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

When read, this file returns the number of the actual profile in
range 0-4.
This file is readonly.
Please use binary attribute “settings” which provides this information.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/firmware\_version** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

When read, this file returns the raw integer version number of the
firmware reported by the mouse. Using the integer value eases
further usage in other programs. To receive the real version
number the decimal point has to be shifted 2 positions to the
left. E.g. a returned value of 138 means 1.38
This file is readonly.
Please use binary attribute “info” which provides this information.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

When read, this file returns general data like firmware version.
When written, the device can be reset.
The data is 6 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/profile[1-5]\_buttons** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When read, these files return the respective profile buttons.
The returned data is 19 bytes in size.
This file is readonly.
Write control to select profile and read profile\_buttons instead.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/profile[1-5]\_settings** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When read, these files return the respective profile settings.
The returned data is 13 bytes in size.
This file is readonly.
Write control to select profile and read profile\_settings instead.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/profile\_buttons** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_buttons holds information about button layout.
When written, this file lets one write the respective profile
buttons back to the mouse. The data has to be 19 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/profile\_settings** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split in settings and buttons.
profile\_settings holds information like resolution, sensitivity
and light effects.
When written, this file lets one write the respective profile
settings back to the mouse. The data has to be 13 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/settings** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

When read, this file returns the settings stored in the mouse.
The size of the data is 3 bytes and holds information on the
startup\_profile.
When written, this file lets write settings back to the mouse.
The data has to be 3 bytes long. The mouse will reject invalid
data.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/pyra/roccatpyra<minor>/startup\_profile** |

Defined on file [sysfs-driver-hid-roccat-pyra](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-pyra)

The integer value of this attribute ranges from 0-4.
When read, this attribute returns the number of the profile
that’s active when the mouse is powered on.
This file is readonly.
Please use binary attribute “settings” which provides this information.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/control** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one select which data from which
profile will be read next. The data has to be 3 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/custom\_lights** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the actual per-key lighting.
This attribute is only valid for the pro variant.
The data has to be 20 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When read, this file returns general data like firmware version.
The data is 8 bytes long.
This file is readonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/key\_mask** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one deactivate certain keys like
windows and application keys, to prevent accidental presses.
Profile index for which this settings occur is included in
written data. The data has to be 6 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/keys\_easyzone** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the function of the
easyzone keys for a specific profile. Profile index is included
in written data. The data has to be 294 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/keys\_extra** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the function of the
capslock and function keys for a specific profile. Profile index
is included in written data. The data has to be 8 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/keys\_function** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the function of the
function keys for a specific profile. Profile index is included
in written data. The data has to be 95 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/keys\_macro** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the function of the macro
keys for a specific profile. Profile index is included in
written data. The data has to be 35 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/keys\_primary** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the default of all keys for
a specific profile. Profile index is included in written data.
The data has to be 125 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/keys\_thumbster** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the function of the
thumbster keys for a specific profile. Profile index is included
in written data. The data has to be 23 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/light** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set the backlight intensity for
a specific profile. Profile index is included in written data.
This attribute is only valid for the glow and pro variant.
The data has to be 16 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/light\_control** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one switch between stored and custom
light settings.
This attribute is only valid for the pro variant.
The data has to be 8 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/light\_macro** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set a light macro that is looped
whenever the device gets in dimness mode.
This attribute is only valid for the pro variant.
The data has to be 2002 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/macro** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one store macros with max 480
keystrokes for a specific button for a specific profile.
Button and profile indexes are included in written data.
The data has to be 2002 bytes long.
Before reading this file, control has to be written to select
which profile and key to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/profile** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

The mouse can store 5 profiles which can be switched by the
press of a button. profile holds index of actual profile.
This value is persistent, so its value determines the profile
that’s active when the device is powered on next time.
When written, the device activates the set profile immediately.
The data has to be 3 bytes long.
The device will reject invalid data.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/reset** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one reset the device.
The data has to be 3 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/stored\_lights** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one set per-key lighting for different
layers.
This attribute is only valid for the pro variant.
The data has to be 1382 bytes long.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/ryos/roccatryos<minor>/talk** |

Defined on file [sysfs-driver-hid-roccat-ryos](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-ryos)

When written, this file lets one trigger easyshift functionality
from the host.
The data has to be 16 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/buttons** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split into general settings and
button settings. The buttons variable holds information about
button layout. When written, this file lets one write the
respective profile buttons to the mouse. The data has to be
47 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
Before reading this file, control has to be written to select
which profile to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/control** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

When written, this file lets one select which data from which
profile will be read next. The data has to be 3 bytes long.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/general** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

The mouse can store 5 profiles which can be switched by the
press of a button. A profile is split into general settings and
button settings. A profile holds information like resolution,
sensitivity and light effects.
When written, this file lets one write the respective profile
settings back to the mouse. The data has to be 43 bytes long.
The mouse will reject invalid data.
Which profile to write is determined by the profile number
contained in the data.
This file is writeonly.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/info** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

When read, this file returns general data like firmware version.
When written, the device can be reset.
The data is 8 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/macro** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

When written, this file lets one store macros with max 500
keystrokes for a specific button for a specific profile.
Button and profile numbers are included in written data.
The data has to be 2083 bytes long.
Before reading this file, control has to be written to select
which profile and key to read.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/profile** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

The mouse can store 5 profiles which can be switched by the
press of a button. profile holds number of actual profile.
This value is persistent, so its value determines the profile
that’s active when the mouse is powered on next time.
When written, the mouse activates the set profile immediately.
The data has to be 3 bytes long.
The mouse will reject invalid data.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/<hid-bus>:<vendor-id>:<product-id>.<num>/savu/roccatsavu<minor>/sensor** |

Defined on file [sysfs-driver-hid-roccat-savu](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-savu)

The mouse has a Avago ADNS-3090 sensor.
This file allows reading and writing of the mouse sensors registers.
The data has to be 4 bytes long.

Users:
:   <http://roccat.sourceforge.net>

|  |
| --- |
| **/sys/bus/usb/devices/<busnum>-<devnum>:<config num>.<interface num>/control** |

Defined on file [sysfs-driver-hid-roccat-lua](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-hid-roccat-lua)

When written, cpi, button and light settings can be configured.
When read, actual cpi setting and sensor data are returned.
The data has to be 8 bytes long.

Users:
:   <http://roccat.sourceforge.net>

## Symbols under /sys/class

|  |
| --- |
| **/sys/class/gpio/** |

Defined on file [sysfs-gpio](abi-obsolete-files.html#abi-file-obsolete-sysfs-gpio)

As a Kconfig option, individual GPIO signals may be accessed from
userspace. GPIOs are only made available to userspace by an explicit
“export” operation. If a given GPIO is not claimed for use by
kernel code, it may be exported by userspace (and unexported later).
Kernel code may export it for complete or partial access.

GPIOs are identified as they are inside the kernel, using integers in
the range 0..INT\_MAX. See Documentation/admin-guide/gpio for more information.

```
/sys/class/gpio
    /export ... asks the kernel to export a GPIO to userspace
    /unexport ... to return a GPIO to the kernel
    /gpioN ... for each exported GPIO #N OR
        /value ... always readable, writes fail for input GPIOs
        /direction ... r/w as: in, out (default low); write: high, low
        /edge ... r/w as: none, falling, rising, both
        /active_low ... r/w as: 0, 1
    /gpiochipN ... for each gpiochip; #N is its first GPIO
        /base ... (r/o) same as N
        /label ... (r/o) descriptive chip name
        /ngpio ... (r/o) number of GPIOs; numbered N to N + (ngpio - 1)
        /gpio<OFFSET>
            /value ... always readable, writes fail for input GPIOs
            /direction ... r/w as: in, out (default low); write: high, low
    /chipX ... for each gpiochip; #X is the gpio device ID
        /export ... asks the kernel to export a GPIO at HW offset X to userspace
        /unexport ... to return a GPIO at HW offset X to the kernel
        /label ... (r/o) descriptive chip name
        /ngpio ... (r/o) number of GPIOs exposed by the chip
```

This ABI is obsoleted by [testing/gpio-cdev](abi-testing-files.html#abi-file-testing-gpio-cdev) and will be
removed after 2020.

|  |
| --- |
| **/sys/class/typec/<port|partner|cable>/<dev>/mode<index>/** |

Defined on file [sysfs-class-typec](abi-obsolete-files.html#abi-file-obsolete-sysfs-class-typec)

Every supported mode will have its own directory. The name of
a mode will be “mode<index>” (for example mode1), where <index>
is the actual index to the mode VDO returned by Discover Modes
USB power delivery command.

|  |
| --- |
| **/sys/class/typec/<port|partner|cable>/<dev>/mode<index>/active** |

Defined on file [sysfs-class-typec](abi-obsolete-files.html#abi-file-obsolete-sysfs-class-typec)

Shows if the mode is active or not. The attribute can be used
for entering/exiting the mode with partners and cable plugs, and
with the port alternate modes it can be used for disabling
support for specific alternate modes. Entering/exiting modes is
supported as synchronous operation so write(2) to the attribute
does not return until the enter/exit mode operation has
finished. The attribute is notified when the mode is
entered/exited so poll(2) on the attribute wakes up.
Entering/exiting a mode will also generate uevent KOBJ\_CHANGE.

Valid values: yes, no

|  |
| --- |
| **/sys/class/typec/<port|partner|cable>/<dev>/mode<index>/description** |

Defined on file [sysfs-class-typec](abi-obsolete-files.html#abi-file-obsolete-sysfs-class-typec)

Shows description of the mode. The description is optional for
the drivers, just like with the Billboard Devices.

|  |
| --- |
| **/sys/class/typec/<port|partner|cable>/<dev>/mode<index>/vdo** |

Defined on file [sysfs-class-typec](abi-obsolete-files.html#abi-file-obsolete-sysfs-class-typec)

Shows the VDO in hexadecimal returned by Discover Modes command
for this mode.

|  |
| --- |
| **/sys/class/typec/<port|partner|cable>/<dev>/svid** |

Defined on file [sysfs-class-typec](abi-obsolete-files.html#abi-file-obsolete-sysfs-class-typec)

The SVID (Standard or Vendor ID) assigned by USB-IF for this
alternate mode.

## Symbols under /sys/devices

|  |
| --- |
| **/sys/devices/platform/samsung/battery\_life\_extender** |

Defined on file [sysfs-driver-samsung-laptop](abi-obsolete-files.html#abi-file-obsolete-sysfs-driver-samsung-laptop)

Max battery charge level can be modified, battery cycle
life can be extended by reducing the max battery charge
level.

* 0 means normal battery mode (100% charge)
* 1 means battery life extender mode (80% charge)

|  |
| --- |
| **/sys/devices/system/cpu/cpuidle/current\_governor\_ro** |

Defined on file [sysfs-cpuidle](abi-obsolete-files.html#abi-file-obsolete-sysfs-cpuidle)

current\_governor\_ro shows current using cpuidle governor, but read only.
with the update that cpuidle governor can be changed at runtime in default,
both current\_governor and current\_governor\_ro co-exist under
/sys/devices/system/cpu/cpuidle/ file, it’s duplicate so make
current\_governor\_ro obsolete.

## Symbols under /sys/firmware

|  |
| --- |
| **/sys/firmware/acpi/hotplug/force\_remove** |

Defined on file [sysfs-firmware-acpi](abi-obsolete-files.html#abi-file-obsolete-sysfs-firmware-acpi)

Since the force\_remove is inherently broken and dangerous to
use for some hotplugable resources like memory (because ignoring
the offline failure might lead to memory corruption and crashes)
enabling this knob is not safe and thus unsupported.

## Symbols under /sys/kernel

|  |
| --- |
| **/sys/kernel/crash\_elfcorehdr\_size** |

Defined on file [sysfs-kernel-kexec-kdump](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-kexec-kdump)

read only
Indicates the preferred size of the memory buffer for the
ELF core header used by the crash (kdump) kernel. It defines
how much space is needed to hold metadata about the crashed
system, including CPU and memory information. This information
is used by the user space utility kexec to support updating the
in-kernel kdump image during hotplug operations.
User: Kexec tools

|  |
| --- |
| **/sys/kernel/debug/tracing** |

Defined on file [automount-tracefs-debugfs](abi-obsolete-files.html#abi-file-obsolete-automount-tracefs-debugfs)

The ftrace was first added to the kernel, its interface was placed
into the debugfs file system under the “tracing” directory. Access
to the files were in /sys/kernel/debug/tracing. As systems wanted
access to the tracing interface without having to enable debugfs, a
new interface was created called “tracefs”. This was a stand alone
file system and was usually mounted in /sys/kernel/tracing.

To allow older tooling to continue to operate, when mounting
debugfs, the tracefs file system would automatically get mounted in
the “tracing” directory of debugfs. The tracefs interface was added
in January 2015 in the v4.1 kernel.

All tooling should now be using tracefs directly and the “tracing”
directory in debugfs should be removed by January 2030.

|  |
| --- |
| **/sys/kernel/fadump\_enabled** |

Defined on file [sysfs-kernel-fadump\_enabled](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-fadump-enabled)

read only
Primarily used to identify whether the FADump is enabled in
the kernel or not.
User: Kdump service

|  |
| --- |
| **/sys/kernel/fadump\_registered** |

Defined on file [sysfs-kernel-fadump\_registered](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-fadump-registered)

read/write
Helps to control the dump collect feature from userspace.
Setting 1 to this file enables the system to collect the
dump and 0 to disable it.
User: Kdump service

|  |
| --- |
| **/sys/kernel/fadump\_release\_mem** |

Defined on file [sysfs-kernel-fadump\_release\_mem](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-fadump-release-mem)

write only
This is a special sysfs file and only available when
the system is booted to capture the vmcore using FADump.
It is used to release the memory reserved by FADump to
save the crash dump.

|  |
| --- |
| **/sys/kernel/kexec\_crash\_cma\_ranges** |

Defined on file [sysfs-kernel-kexec-kdump](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-kexec-kdump)

read only
Provides information about the memory ranges reserved from
the Contiguous Memory Allocator (CMA) area that are allocated
to the crash (kdump) kernel. It lists the start and end physical
addresses of CMA regions assigned for crashkernel use.
User: kdump service

|  |
| --- |
| **/sys/kernel/kexec\_crash\_loaded** |

Defined on file [sysfs-kernel-kexec-kdump](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-kexec-kdump)

read only
Indicates whether a crash (kdump) kernel is currently
loaded into memory. It shows 1 if a crash kernel has been
successfully loaded for panic handling, or 0 if no crash
kernel is present.
User: Kexec tools, Kdump service

|  |
| --- |
| **/sys/kernel/kexec\_crash\_size** |

Defined on file [sysfs-kernel-kexec-kdump](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-kexec-kdump)

read/write
Shows the amount of memory reserved for loading the crash
(kdump) kernel. It reports the size, in bytes, of the
crash kernel area defined by the crashkernel= parameter.
This interface also allows reducing the crashkernel
reservation by writing a smaller value, and the reclaimed
space is added back to the system RAM.
User: Kdump service

|  |
| --- |
| **/sys/kernel/kexec\_loaded** |

Defined on file [sysfs-kernel-kexec-kdump](abi-obsolete-files.html#abi-file-obsolete-sysfs-kernel-kexec-kdump)

read only
Indicates whether a new kernel image has been loaded
into memory using the kexec system call. It shows 1 if
a kexec image is present and ready to boot, or 0 if none
is loaded.
User: kexec tools, kdump service

## Symbols under /sys/o2cb

|  |
| --- |
| **/sys/o2cb** |

Defined on file [o2cb](abi-obsolete-files.html#abi-file-obsolete-o2cb)

Ocfs2-tools looks at ‘interface-revision’ for versioning
information. Each logmask/ file controls a set of debug prints
and can be written into with the strings “allow”, “deny”, or
“off”. Reading the file returns the current state.
Was renamed to /sys/fs/u2cb/

Users:
:   ocfs2-tools. It’s sufficient to mail proposed changes to
    [ocfs2-devel@lists.linux.dev](mailto:ocfs2-devel%40lists.linux.dev).
