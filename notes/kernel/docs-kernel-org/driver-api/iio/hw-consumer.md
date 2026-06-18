# HW consumer

> 출처(원문): https://docs.kernel.org/driver-api/iio/hw-consumer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# HW consumer

An IIO device can be directly connected to another device in hardware. In this
case the buffers between IIO provider and IIO consumer are handled by hardware.
The Industrial I/O HW consumer offers a way to bond these IIO devices without
software buffer for data. The implementation can be found under
`drivers/iio/buffer/hw-consumer.c`

* `struct iio_hw_consumer` — Hardware consumer structure
* [`iio_hw_consumer_alloc()`](#c.iio_hw_consumer_alloc "iio_hw_consumer_alloc") — Allocate IIO hardware consumer
* [`iio_hw_consumer_free()`](#c.iio_hw_consumer_free "iio_hw_consumer_free") — Free IIO hardware consumer
* [`iio_hw_consumer_enable()`](#c.iio_hw_consumer_enable "iio_hw_consumer_enable") — Enable IIO hardware consumer
* [`iio_hw_consumer_disable()`](#c.iio_hw_consumer_disable "iio_hw_consumer_disable") — Disable IIO hardware consumer

## HW consumer setup

As standard IIO device the implementation is based on IIO provider/consumer.
A typical IIO HW consumer setup looks like this:

```
static struct iio_hw_consumer *hwc;

static const struct iio_info adc_info = {
        .read_raw = adc_read_raw,
};

static int adc_read_raw(struct iio_dev *indio_dev,
                        struct iio_chan_spec const *chan, int *val,
                        int *val2, long mask)
{
        ret = iio_hw_consumer_enable(hwc);

        /* Acquire data */

        ret = iio_hw_consumer_disable(hwc);
}

static int adc_probe(struct platform_device *pdev)
{
        hwc = devm_iio_hw_consumer_alloc(&iio->dev);
}
```

## More details

struct iio\_hw\_consumer \*iio\_hw\_consumer\_alloc(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Allocate IIO hardware consumer

**Parameters**

`struct device *dev`
:   Pointer to consumer device.

**Description**

Returns a valid iio\_hw\_consumer on success or a [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on failure.

void iio\_hw\_consumer\_free(struct iio\_hw\_consumer \*hwc)
:   Free IIO hardware consumer

**Parameters**

`struct iio_hw_consumer *hwc`
:   hw consumer to free.

struct iio\_hw\_consumer \*devm\_iio\_hw\_consumer\_alloc(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Resource-managed [`iio_hw_consumer_alloc()`](#c.iio_hw_consumer_alloc "iio_hw_consumer_alloc")

**Parameters**

`struct device *dev`
:   Pointer to consumer device.

**Description**

Managed iio\_hw\_consumer\_alloc. iio\_hw\_consumer allocated with this function
is automatically freed on driver detach.

returns pointer to allocated iio\_hw\_consumer on success, NULL on failure.

int iio\_hw\_consumer\_enable(struct iio\_hw\_consumer \*hwc)
:   Enable IIO hardware consumer

**Parameters**

`struct iio_hw_consumer *hwc`
:   iio\_hw\_consumer to enable.

**Description**

Returns 0 on success.

void iio\_hw\_consumer\_disable(struct iio\_hw\_consumer \*hwc)
:   Disable IIO hardware consumer

**Parameters**

`struct iio_hw_consumer *hwc`
:   iio\_hw\_consumer to disable.
