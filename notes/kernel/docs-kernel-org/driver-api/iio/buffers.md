# Buffers

> 출처(원문): https://docs.kernel.org/driver-api/iio/buffers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Buffers

* `struct iio_buffer` — general buffer structure
* [`iio_validate_scan_mask_onehot()`](#c.iio_validate_scan_mask_onehot "iio_validate_scan_mask_onehot") — Validates that exactly one channel
  is selected
* [`iio_buffer_get()`](#c.iio_buffer_get "iio_buffer_get") — Grab a reference to the buffer
* [`iio_buffer_put()`](#c.iio_buffer_put "iio_buffer_put") — Release the reference to the buffer

The Industrial I/O core offers a way for continuous data capture based on a
trigger source. Multiple data channels can be read at once from
`/dev/iio:deviceX` character device node, thus reducing the CPU load.

## IIO buffer sysfs interface

An IIO buffer has an associated attributes directory under
`/sys/bus/iio/devices/iio:deviceX/buffer/*`. Here are some of the
existing attributes:

* `length`, the total number of data samples (capacity) that can be
  stored by the buffer.
* `enable`, activate buffer capture.

## IIO buffer setup

The meta information associated with a channel reading placed in a buffer is
called a scan element. The important bits configuring scan elements are
exposed to userspace applications via the
`/sys/bus/iio/devices/iio:deviceX/scan_elements/` directory. This
directory contains attributes of the following form:

* `enable`, used for enabling a channel. If and only if its attribute
  is non *zero*, then a triggered capture will contain data samples for this
  channel.
* `index`, the scan\_index of the channel.
* `type`, description of the scan element data storage within the buffer
  and hence the form in which it is read from user space.
  Format is [be|le]:[s|u]bits/storagebits[Xrepeat][>>shift] .

  + *be* or *le*, specifies big or little endian.
  + *s* or *u*, specifies if signed (2’s complement) or unsigned.
  + *bits*, is the number of valid data bits.
  + *storagebits*, is the number of bits (after padding) that it occupies in the
    buffer.
  + *repeat*, specifies the number of bits/storagebits repetitions. When the
    repeat element is 0 or 1, then the repeat value is omitted.
  + *shift*, if specified, is the shift that needs to be applied prior to
    masking out unused bits.

For example, a driver for a 3-axis accelerometer with 12 bit resolution where
data is stored in two 8-bits registers as follows:

```
  7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+
|D3 |D2 |D1 |D0 | X | X | X | X | (LOW byte, address 0x06)
+---+---+---+---+---+---+---+---+

  7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+
|D11|D10|D9 |D8 |D7 |D6 |D5 |D4 | (HIGH byte, address 0x07)
+---+---+---+---+---+---+---+---+
```

will have the following scan element type for each axis:

```
$ cat /sys/bus/iio/devices/iio:device0/scan_elements/in_accel_y_type
le:s12/16>>4
```

A user space application will interpret data samples read from the buffer as
two byte little endian signed data, that needs a 4 bits right shift before
masking out the 12 valid bits of data.

For implementing buffer support a driver should initialize the following
fields in iio\_chan\_spec definition:

```
struct iio_chan_spec {
/* other members */
        int scan_index
        struct {
                char sign;
                u8 realbits;
                u8 storagebits;
                u8 shift;
                u8 repeat;
                enum iio_endian endianness;
               } scan_type;
       };
```

The driver implementing the accelerometer described above will have the
following channel definition:

```
struct iio_chan_spec accel_channels[] = {
        {
                .type = IIO_ACCEL,
                .modified = 1,
                .channel2 = IIO_MOD_X,
                /* other stuff here */
                .scan_index = 0,
                .scan_type = {
                        .sign = 's',
                        .realbits = 12,
                        .storagebits = 16,
                        .shift = 4,
                        .endianness = IIO_LE,
                },
        }
        /* similar for Y (with channel2 = IIO_MOD_Y, scan_index = 1)
         * and Z (with channel2 = IIO_MOD_Z, scan_index = 2) axis
         */
 }
```

Here **scan\_index** defines the order in which the enabled channels are placed
inside the buffer. Channels with a lower **scan\_index** will be placed before
channels with a higher index. Each channel needs to have a unique
**scan\_index**.

Setting **scan\_index** to -1 can be used to indicate that the specific channel
does not support buffered capture. In this case no entries will be created for
the channel in the scan\_elements directory.

## More details

int iio\_push\_to\_buffers\_with\_timestamp(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, void \*data, int64\_t timestamp)
:   push data and timestamp to buffers

**Parameters**

`struct iio_dev *indio_dev`
:   iio\_dev structure for device.

`void *data`
:   sample data

`int64_t timestamp`
:   timestamp for the sample data

**Description**

DEPRECATED: Use [`iio_push_to_buffers_with_ts()`](#c.iio_push_to_buffers_with_ts "iio_push_to_buffers_with_ts") instead.

Returns 0 on success, a negative error code otherwise.

int iio\_push\_to\_buffers\_with\_ts(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, void \*data, size\_t data\_total\_len, s64 timestamp)
:   push data and timestamp to buffers

**Parameters**

`struct iio_dev *indio_dev`
:   iio\_dev structure for device.

`void *data`
:   Pointer to sample data buffer.

`size_t data_total_len`
:   The size of **data** in bytes.

`s64 timestamp`
:   Timestamp for the sample data.

**Description**

Pushes data to the IIO device’s buffers. If timestamps are enabled for the
device the function will store the supplied timestamp as the last element in
the sample data buffer before pushing it to the device buffers. The sample
data buffer needs to be large enough to hold the additional timestamp
(usually the buffer should be at least indio->scan\_bytes bytes large).

**Context**

Any context.

**Return**

0 on success, a negative error code otherwise.

bool iio\_validate\_scan\_mask\_onehot(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, const unsigned long \*mask)
:   Validates that exactly one channel is selected

**Parameters**

`struct iio_dev *indio_dev`
:   the iio device

`const unsigned long *mask`
:   scan mask to be checked

**Description**

Return true if exactly one bit is set in the scan mask, false otherwise. It
can be used for devices where only one channel can be active for sampling at
a time.

int iio\_push\_to\_buffers(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, const void \*data)
:   push to a registered buffer.

**Parameters**

`struct iio_dev *indio_dev`
:   iio\_dev structure for device.

`const void *data`
:   Full scan.

**Context**

Any context.

**Return**

0 on success, negative error code on failure.

int iio\_push\_to\_buffers\_with\_ts\_unaligned(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, const void \*data, size\_t data\_sz, int64\_t timestamp)
:   push to registered buffer, no alignment or space requirements.

**Parameters**

`struct iio_dev *indio_dev`
:   iio\_dev structure for device.

`const void *data`
:   channel data excluding the timestamp.

`size_t data_sz`
:   size of data.

`int64_t timestamp`
:   timestamp for the sample data.

**Description**

This special variant of [`iio_push_to_buffers_with_timestamp()`](#c.iio_push_to_buffers_with_timestamp "iio_push_to_buffers_with_timestamp") does
not require space for the timestamp, or 8 byte alignment of data.
It does however require an allocation on first call and additional
copies on all calls, so should be avoided if possible.

**Context**

May sleep.

**Return**

0 on success, negative error code on failure.

struct iio\_buffer \*iio\_buffer\_get(struct iio\_buffer \*buffer)
:   Grab a reference to the buffer

**Parameters**

`struct iio_buffer *buffer`
:   The buffer to grab a reference for, may be NULL

**Description**

Returns the pointer to the buffer that was passed into the function.

void iio\_buffer\_put(struct iio\_buffer \*buffer)
:   Release the reference to the buffer

**Parameters**

`struct iio_buffer *buffer`
:   The buffer to release the reference for, may be NULL

int iio\_device\_attach\_buffer(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, struct iio\_buffer \*buffer)
:   Attach a buffer to a IIO device

**Parameters**

`struct iio_dev *indio_dev`
:   The device the buffer should be attached to

`struct iio_buffer *buffer`
:   The buffer to attach to the device

**Description**

Return 0 if successful, negative if error.

This function attaches a buffer to a IIO device. The buffer stays attached to
the device until the device is freed. For legacy reasons, the first attached
buffer will also be assigned to ‘indio\_dev->buffer’.
The array allocated here, will be free’d via the `iio_device_detach_buffers()`
call which is handled by the [`iio_device_free()`](core.html#c.iio_device_free "iio_device_free").
