# RS485 Serial Communications

> 출처(원문): https://docs.kernel.org/driver-api/serial/serial-rs485.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# RS485 Serial Communications

## 1. Introduction

> EIA-485, also known as TIA/EIA-485 or RS-485, is a standard defining the
> electrical characteristics of drivers and receivers for use in balanced
> digital multipoint systems.
> This standard is widely used for communications in industrial automation
> because it can be used effectively over long distances and in electrically
> noisy environments.

## 2. Hardware-related Considerations

> Some CPUs/UARTs (e.g., Atmel AT91 or 16C950 UART) contain a built-in
> half-duplex mode capable of automatically controlling line direction by
> toggling RTS or DTR signals. That can be used to control external
> half-duplex hardware like an RS485 transceiver or any RS232-connected
> half-duplex devices like some modems.
>
> For these microcontrollers, the Linux driver should be made capable of
> working in both modes, and proper ioctls (see later) should be made
> available at user-level to allow switching from one mode to the other, and
> vice versa.

## 3. Data Structures Already Available in the Kernel

> The Linux kernel provides the [`struct serial_rs485`](#c.serial_rs485 "serial_rs485") to handle RS485
> communications. This data structure is used to set and configure RS485
> parameters in the platform data and in ioctls.
>
> The device tree can also provide RS485 boot time parameters
> [[1]](#dt-bindings). The serial core fills the [`struct serial_rs485`](#c.serial_rs485 "serial_rs485") from the
> values given by the device tree when the driver calls
> `uart_get_rs485_mode()`.
>
> Any driver for devices capable of working both as RS232 and RS485 should
> implement the `rs485_config` callback and provide `rs485_supported`
> in the `struct uart_port`. The serial core calls `rs485_config` to do
> the device specific part in response to TIOCSRS485 ioctl (see below). The
> `rs485_config` callback receives a pointer to a sanitizated [`struct
> serial_rs485`](#c.serial_rs485 "serial_rs485"). The [`struct serial_rs485`](#c.serial_rs485 "serial_rs485") userspace provides is sanitized
> before calling `rs485_config` using `rs485_supported` that indicates
> what RS485 features the driver supports for the `struct uart_port`.
> TIOCGRS485 ioctl can be used to read back the [`struct serial_rs485`](#c.serial_rs485 "serial_rs485")
> matching to the current configuration.

struct serial\_rs485
:   serial interface for controlling RS485 settings.

**Definition**:

```
struct serial_rs485 {
    __u32 flags;
#define SER_RS485_ENABLED               _BITUL(0);
#define SER_RS485_RTS_ON_SEND           _BITUL(1);
#define SER_RS485_RTS_AFTER_SEND        _BITUL(2);
#define SER_RS485_RX_DURING_TX          _BITUL(4);
#define SER_RS485_TERMINATE_BUS         _BITUL(5);
#define SER_RS485_ADDRB                 _BITUL(6);
#define SER_RS485_ADDR_RECV             _BITUL(7);
#define SER_RS485_ADDR_DEST             _BITUL(8);
#define SER_RS485_MODE_RS422            _BITUL(9);
    __u32 delay_rts_before_send;
    __u32 delay_rts_after_send;
    union {
        __u32 padding[5];
        struct {
            __u8 addr_recv;
            __u8 addr_dest;
            __u8 padding0[2];
            __u32 padding1[4];
        };
    };
};
```

**Members**

`flags`
:   RS485 feature flags.

`delay_rts_before_send`
:   Delay before send (milliseconds).

`delay_rts_after_send`
:   Delay after send (milliseconds).

`{unnamed_union}`
:   anonymous

`padding`
:   Deprecated, use **padding0** and **padding1** instead.
    Do not use with **addr\_recv** and **addr\_dest** (due to
    overlap).

`{unnamed_struct}`
:   anonymous

`addr_recv`
:   Receive filter for RS485 addressing mode
    (used only when `SER_RS485_ADDR_RECV` is set).

`addr_dest`
:   Destination address for RS485 addressing mode
    (used only when `SER_RS485_ADDR_DEST` is set).

`padding0`
:   Padding (set to zero).

`padding1`
:   Padding (set to zero).

**Description**

Serial interface for controlling RS485 settings on chips with suitable
support. Set with TIOCSRS485 and get with TIOCGRS485 if supported by your
platform. The set function returns the new state, with any unsupported bits
reverted appropriately.

The flag bits are:

* `SER_RS485_ENABLED` - RS485 enabled.
* `SER_RS485_RTS_ON_SEND` - Logical level for RTS pin when sending.
* `SER_RS485_RTS_AFTER_SEND` - Logical level for RTS pin after sent.
* `SER_RS485_RX_DURING_TX` - Full-duplex RS485 line.
* `SER_RS485_TERMINATE_BUS` - Enable bus termination (if supported).
* `SER_RS485_ADDRB` - Enable RS485 addressing mode.
* `SER_RS485_ADDR_RECV` - Receive address filter (enables **addr\_recv**). Requires `SER_RS485_ADDRB`.
* `SER_RS485_ADDR_DEST` - Destination address (enables **addr\_dest**). Requires `SER_RS485_ADDRB`.
* `SER_RS485_MODE_RS422` - Enable RS422. Requires `SER_RS485_ENABLED`.

## 4. Usage from user-level

> From user-level, RS485 configuration can be get/set using the previous
> ioctls. For instance, to set RS485 you can use the following code:
>
> ```
> #include <linux/serial.h>
>
> /* Include definition for RS485 ioctls: TIOCGRS485 and TIOCSRS485 */
> #include <sys/ioctl.h>
>
> /* Open your specific device (e.g., /dev/mydevice): */
> int fd = open ("/dev/mydevice", O_RDWR);
> if (fd < 0) {
>         /* Error handling. See errno. */
> }
>
> struct serial_rs485 rs485conf;
>
> /* Enable RS485 mode: */
> rs485conf.flags |= SER_RS485_ENABLED;
>
> /* Set logical level for RTS pin equal to 1 when sending: */
> rs485conf.flags |= SER_RS485_RTS_ON_SEND;
> /* or, set logical level for RTS pin equal to 0 when sending: */
> rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);
>
> /* Set logical level for RTS pin equal to 1 after sending: */
> rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
> /* or, set logical level for RTS pin equal to 0 after sending: */
> rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);
>
> /* Set rts delay before send, if needed: */
> rs485conf.delay_rts_before_send = ...;
>
> /* Set rts delay after send, if needed: */
> rs485conf.delay_rts_after_send = ...;
>
> /* Set this flag if you want to receive data even while sending data */
> rs485conf.flags |= SER_RS485_RX_DURING_TX;
>
> if (ioctl (fd, TIOCSRS485, &rs485conf) < 0) {
>         /* Error handling. See errno. */
> }
>
> /* Use read() and write() syscalls here... */
>
> /* Close the device when finished: */
> if (close (fd) < 0) {
>         /* Error handling. See errno. */
> }
> ```

## 5. Multipoint Addressing

> The Linux kernel provides addressing mode for multipoint RS-485 serial
> communications line. The addressing mode is enabled with
> `SER_RS485_ADDRB` flag in [`struct serial_rs485`](#c.serial_rs485 "serial_rs485"). The [`struct serial_rs485`](#c.serial_rs485 "serial_rs485")
> has two additional flags and fields for enabling receive and destination
> addresses.
>
> Address mode flags:
> :   * `SER_RS485_ADDRB`: Enabled addressing mode (sets also ADDRB in termios).
>     * `SER_RS485_ADDR_RECV`: Receive (filter) address enabled.
>     * `SER_RS485_ADDR_DEST`: Set destination address.
>
> Address fields (enabled with corresponding `SER_RS485_ADDR_*` flag):
> :   * `addr_recv`: Receive address.
>     * `addr_dest`: Destination address.
>
> Once a receive address is set, the communication can occur only with the
> particular device and other peers are filtered out. It is left up to the
> receiver side to enforce the filtering. Receive address will be cleared
> if `SER_RS485_ADDR_RECV` is not set.
>
> Note: not all devices supporting RS485 support multipoint addressing.

## 6. References

[[1](#id1)]

Documentation/devicetree/bindings/serial/rs485.txt
