# Triggered Buffers

> 출처(원문): https://docs.kernel.org/driver-api/iio/triggered-buffers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Triggered Buffers

Now that we know what buffers and triggers are let’s see how they work together.

## IIO triggered buffer setup

* `iio_triggered_buffer_setup()` — Setup triggered buffer and pollfunc
* [`iio_triggered_buffer_cleanup()`](#c.iio_triggered_buffer_cleanup "iio_triggered_buffer_cleanup") — Free resources allocated by
  `iio_triggered_buffer_setup()`
* [`struct iio_buffer_setup_ops`](core.html#c.iio_buffer_setup_ops "iio_buffer_setup_ops") — buffer setup related callbacks

A typical triggered buffer setup looks like this:

```
const struct iio_buffer_setup_ops sensor_buffer_setup_ops = {
  .preenable    = sensor_buffer_preenable,
  .postenable   = sensor_buffer_postenable,
  .postdisable  = sensor_buffer_postdisable,
  .predisable   = sensor_buffer_predisable,
};

irqreturn_t sensor_iio_pollfunc(int irq, void *p)
{
    pf->timestamp = iio_get_time_ns((struct indio_dev *)p);
    return IRQ_WAKE_THREAD;
}

irqreturn_t sensor_trigger_handler(int irq, void *p)
{
    u16 buf[8];
    int i = 0;

    /* read data for each active channel */
    for_each_set_bit(bit, active_scan_mask, masklength)
        buf[i++] = sensor_get_data(bit)

    iio_push_to_buffers_with_timestamp(indio_dev, buf, timestamp);

    iio_trigger_notify_done(trigger);
    return IRQ_HANDLED;
}

/* setup triggered buffer, usually in probe function */
iio_triggered_buffer_setup(indio_dev, sensor_iio_polfunc,
                           sensor_trigger_handler,
                           sensor_buffer_setup_ops);
```

The important things to notice here are:

* [`iio_buffer_setup_ops`](core.html#c.iio_buffer_setup_ops "iio_buffer_setup_ops"), the buffer setup functions to be called at
  predefined points in the buffer configuration sequence (e.g. before enable,
  after disable). If not specified, the IIO core uses the default
  iio\_triggered\_buffer\_setup\_ops.
* **sensor\_iio\_pollfunc**, the function that will be used as top half of poll
  function. It should do as little processing as possible, because it runs in
  interrupt context. The most common operation is recording of the current
  timestamp and for this reason one can use the IIO core defined
  `iio_pollfunc_store_time()` function.
* **sensor\_trigger\_handler**, the function that will be used as bottom half of
  the poll function. This runs in the context of a kernel thread and all the
  processing takes place here. It usually reads data from the device and
  stores it in the internal buffer together with the timestamp recorded in the
  top half.

## More details

int iio\_triggered\_buffer\_setup\_ext(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev, irqreturn\_t (\*h)(int irq, void \*p), irqreturn\_t (\*thread)(int irq, void \*p), enum iio\_buffer\_direction direction, const struct [iio\_buffer\_setup\_ops](core.html#c.iio_buffer_setup_ops "iio_buffer_setup_ops") \*setup\_ops, const struct iio\_dev\_attr \*\*buffer\_attrs)
:   Setup triggered buffer and pollfunc

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure

`irqreturn_t (*h)(int irq, void *p)`
:   Function which will be used as pollfunc top half

`irqreturn_t (*thread)(int irq, void *p)`
:   Function which will be used as pollfunc bottom half

`enum iio_buffer_direction direction`
:   Direction of the data stream (in/out).

`const struct iio_buffer_setup_ops *setup_ops`
:   Buffer setup functions to use for this device.
    If NULL the default setup functions for triggered
    buffers will be used.

`const struct iio_dev_attr **buffer_attrs`
:   Extra sysfs buffer attributes for this IIO buffer

**Description**

This function combines some common tasks which will normally be performed
when setting up a triggered buffer. It will allocate the buffer and the
pollfunc.

Before calling this function the indio\_dev structure should already be
completely initialized, but not yet registered. In practice this means that
this function should be called right before [`iio_device_register()`](core.html#c.iio_device_register "iio_device_register").

To free the resources allocated by this function call
[`iio_triggered_buffer_cleanup()`](#c.iio_triggered_buffer_cleanup "iio_triggered_buffer_cleanup").

void iio\_triggered\_buffer\_cleanup(struct [iio\_dev](core.html#c.iio_dev "iio_dev") \*indio\_dev)
:   Free resources allocated by [`iio_triggered_buffer_setup_ext()`](#c.iio_triggered_buffer_setup_ext "iio_triggered_buffer_setup_ext")

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure
