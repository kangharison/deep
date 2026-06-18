# ACPI CA Debug Output

> 출처(원문): https://docs.kernel.org/firmware-guide/acpi/debug.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ACPI CA Debug Output

The ACPI CA can generate debug output. This document describes how to use this
facility.

## Compile-time configuration

The ACPI CA debug output is globally enabled by CONFIG\_ACPI\_DEBUG. If this
config option is not set, the debug messages are not even built into the kernel.

## Boot- and run-time configuration

When CONFIG\_ACPI\_DEBUG=y, you can select the component and level of messages
you’re interested in. At boot-time, use the acpi.debug\_layer and
acpi.debug\_level kernel command line options. After boot, you can use the
debug\_layer and debug\_level files in /sys/module/acpi/parameters/ to control
the debug messages.

## debug\_layer (component)

The “debug\_layer” is a mask that selects components of interest, e.g., a
specific part of the ACPI interpreter. To build the debug\_layer bitmask, look
for the “#define \_COMPONENT” in an ACPI source file.

You can set the debug\_layer mask at boot-time using the acpi.debug\_layer
command line argument, and you can change it after boot by writing values
to /sys/module/acpi/parameters/debug\_layer.

The possible components are defined in include/acpi/acoutput.h.

Reading /sys/module/acpi/parameters/debug\_layer shows the supported mask values:

```
ACPI_UTILITIES                  0x00000001
ACPI_HARDWARE                   0x00000002
ACPI_EVENTS                     0x00000004
ACPI_TABLES                     0x00000008
ACPI_NAMESPACE                  0x00000010
ACPI_PARSER                     0x00000020
ACPI_DISPATCHER                 0x00000040
ACPI_EXECUTER                   0x00000080
ACPI_RESOURCES                  0x00000100
ACPI_CA_DEBUGGER                0x00000200
ACPI_OS_SERVICES                0x00000400
ACPI_CA_DISASSEMBLER            0x00000800
ACPI_COMPILER                   0x00001000
ACPI_TOOLS                      0x00002000
```

## debug\_level

The “debug\_level” is a mask that selects different types of messages, e.g.,
those related to initialization, method execution, informational messages, etc.
To build debug\_level, look at the level specified in an `ACPI_DEBUG_PRINT()`
statement.

The ACPI interpreter uses several different levels, but the Linux
ACPI core and ACPI drivers generally only use ACPI\_LV\_INFO.

You can set the debug\_level mask at boot-time using the acpi.debug\_level
command line argument, and you can change it after boot by writing values
to /sys/module/acpi/parameters/debug\_level.

The possible levels are defined in include/acpi/acoutput.h. Reading
/sys/module/acpi/parameters/debug\_level shows the supported mask values,
currently these:

```
ACPI_LV_INIT                    0x00000001
ACPI_LV_DEBUG_OBJECT            0x00000002
ACPI_LV_INFO                    0x00000004
ACPI_LV_INIT_NAMES              0x00000020
ACPI_LV_PARSE                   0x00000040
ACPI_LV_LOAD                    0x00000080
ACPI_LV_DISPATCH                0x00000100
ACPI_LV_EXEC                    0x00000200
ACPI_LV_NAMES                   0x00000400
ACPI_LV_OPREGION                0x00000800
ACPI_LV_BFIELD                  0x00001000
ACPI_LV_TABLES                  0x00002000
ACPI_LV_VALUES                  0x00004000
ACPI_LV_OBJECTS                 0x00008000
ACPI_LV_RESOURCES               0x00010000
ACPI_LV_USER_REQUESTS           0x00020000
ACPI_LV_PACKAGE                 0x00040000
ACPI_LV_ALLOCATIONS             0x00100000
ACPI_LV_FUNCTIONS               0x00200000
ACPI_LV_OPTIMIZATIONS           0x00400000
ACPI_LV_MUTEX                   0x01000000
ACPI_LV_THREADS                 0x02000000
ACPI_LV_IO                      0x04000000
ACPI_LV_INTERRUPTS              0x08000000
ACPI_LV_AML_DISASSEMBLE         0x10000000
ACPI_LV_VERBOSE_INFO            0x20000000
ACPI_LV_FULL_TABLES             0x40000000
ACPI_LV_EVENTS                  0x80000000
```

## Examples

For example, drivers/acpi/acpica/evxfevnt.c contains this:

```
#define _COMPONENT          ACPI_EVENTS
...
ACPI_DEBUG_PRINT((ACPI_DB_INIT, "ACPI mode disabled\n"));
```

To turn on this message, set the ACPI\_EVENTS bit in acpi.debug\_layer
and the ACPI\_LV\_INIT bit in acpi.debug\_level. (The ACPI\_DEBUG\_PRINT
statement uses ACPI\_DB\_INIT, which is a macro based on the ACPI\_LV\_INIT
definition.)

Enable all AML “Debug” output (stores to the Debug object while interpreting
AML) during boot:

```
acpi.debug_layer=0xffffffff acpi.debug_level=0x2
```

Enable all ACPI hardware-related messages:

```
acpi.debug_layer=0x2 acpi.debug_level=0xffffffff
```

Enable all ACPI\_DB\_INFO messages after boot:

```
# echo 0x4 > /sys/module/acpi/parameters/debug_level
```

Show all valid component values:

```
# cat /sys/module/acpi/parameters/debug_layer
```
