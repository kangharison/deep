# Introduction to the 1-wire (w1) subsystem

> 출처(원문): https://docs.kernel.org/w1/w1-generic.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Introduction to the 1-wire (w1) subsystem

The 1-wire bus is a simple master-slave bus that communicates via a single
signal wire (plus ground, so two wires).

Devices communicate on the bus by pulling the signal to ground via an open
drain output and by sampling the logic level of the signal line.

The w1 subsystem provides the framework for managing w1 masters and
communication with slaves.

All w1 slave devices must be connected to a w1 bus master device.

Example w1 master devices:

> * DS9490 usb device
> * W1-over-GPIO
> * DS2482 (i2c to w1 bridge)
> * Emulated devices, such as a RS232 converter, parallel port adapter, etc

## What does the w1 subsystem do?

When a w1 master driver registers with the w1 subsystem, the following occurs:

> * sysfs entries for that w1 master are created
> * the w1 bus is periodically searched for new slave devices

When a device is found on the bus, w1 core tries to load the driver for its family
and check if it is loaded. If so, the family driver is attached to the slave.
If there is no driver for the family, default one is assigned, which allows to perform
almost any kind of operations. Each logical operation is a transaction
in nature, which can contain several (two or one) low-level operations.
Let’s see how one can read EEPROM context:
1. one must write control buffer, i.e. buffer containing command byte
and two byte address. At this step bus is reset and appropriate device
is selected using either W1\_SKIP\_ROM or W1\_MATCH\_ROM command.
Then provided control buffer is being written to the wire.
2. reading. This will issue reading eeprom response.

It is possible that between 1. and 2. w1 master thread will reset bus for searching
and slave device will be even removed, but in this case 0xff will
be read, since no device was selected.

## W1 device families

Slave devices are handled by a driver written for a family of w1 devices.

A family driver populates a [`struct w1_family_ops`](../driver-api/w1.html#c.w1_family_ops "w1_family_ops") (see w1\_family.h) and
registers with the w1 subsystem.

Current family drivers:

w1\_therm
:   * (ds18?20 thermal sensor family driver)
      provides temperature reading function which is bound to ->`rbin()` method
      of the above w1\_family\_ops structure.

w1\_smem
:   * driver for simple 64bit memory cell provides ID reading method.

You can call above methods by reading appropriate sysfs files.

## What does a w1 master driver need to implement?

The driver for w1 bus master must provide at minimum two functions.

Emulated devices must provide the ability to set the output signal level
(write\_bit) and sample the signal level (read\_bit).

Devices that support the 1-wire natively must provide the ability to write and
sample a bit (touch\_bit) and reset the bus (reset\_bus).

Most hardware provides higher-level functions that offload w1 handling.
See [`struct w1_bus_master`](../driver-api/w1.html#c.w1_bus_master "w1_bus_master") definition in w1.h for details.

## w1 master sysfs interface

|  |  |
| --- | --- |
| <xx-xxxxxxxxxxxx> | A directory for a found device. The format is family-serial |
| bus | (standard) symlink to the w1 bus |
| driver | (standard) symlink to the w1 driver |
| w1\_master\_add | (rw) manually register a slave device |
| w1\_master\_attempts | (ro) the number of times a search was attempted |
| w1\_master\_max\_slave\_count | (rw) maximum number of slaves to search for at a time |
| w1\_master\_name | (ro) the name of the device (w1\_bus\_masterX) |
| w1\_master\_pullup | (rw) 5V strong pullup 0 enabled, 1 disabled |
| w1\_master\_remove | (rw) manually remove a slave device |
| w1\_master\_search | (rw) the number of searches left to do, -1=continual (default) |
| w1\_master\_slave\_count | (ro) the number of slaves found |
| w1\_master\_slaves | (ro) the names of the slaves, one per line |
| w1\_master\_timeout | (ro) the delay in seconds between searches |
| w1\_master\_timeout\_us | (ro) the delay in microseconds between searches |

If you have a w1 bus that never changes (you don’t add or remove devices),
you can set the module parameter search\_count to a small positive number
for an initially small number of bus searches. Alternatively it could be
set to zero, then manually add the slave device serial numbers by
w1\_master\_add device file. The w1\_master\_add and w1\_master\_remove files
generally only make sense when searching is disabled, as a search will
redetect manually removed devices that are present and timeout manually
added devices that aren’t on the bus.

Bus searches occur at an interval, specified as a sum of timeout and
timeout\_us module parameters (either of which may be 0) for as long as
w1\_master\_search remains greater than 0 or is -1. Each search attempt
decrements w1\_master\_search by 1 (down to 0) and increments
w1\_master\_attempts by 1.

## w1 slave sysfs interface

|  |  |
| --- | --- |
| bus | (standard) symlink to the w1 bus |
| driver | (standard) symlink to the w1 driver |
| name | the device name, usually the same as the directory name |
| w1\_slave | (optional) a binary file whose meaning depends on the family driver |
| rw | (optional) created for slave devices which do not have appropriate family driver. Allows to read/write binary data. |
