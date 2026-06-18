# Platform Device API

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/api/platformdevice.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Platform Device API

The KUnit platform device API is used to test platform devices.

struct platform\_device \*kunit\_platform\_device\_alloc(struct [kunit](test.html#c.kunit "kunit") \*test, const char \*name, int id)
:   Allocate a KUnit test managed platform device

**Parameters**

`struct kunit *test`
:   test context

`const char *name`
:   device name of platform device to alloc

`int id`
:   identifier of platform device to alloc.

**Description**

Allocate a test managed platform device. The device is put when the test completes.

**Return**

Allocated platform device on success, NULL on failure.

int kunit\_platform\_device\_add(struct [kunit](test.html#c.kunit "kunit") \*test, struct platform\_device \*pdev)
:   Register a KUnit test managed platform device

**Parameters**

`struct kunit *test`
:   test context

`struct platform_device *pdev`
:   platform device to add

**Description**

Register a test managed platform device. The device is unregistered when the
test completes.

**Return**

0 on success, negative errno on failure.

int kunit\_platform\_device\_prepare\_wait\_for\_probe(struct [kunit](test.html#c.kunit "kunit") \*test, struct platform\_device \*pdev, struct completion \*x)
:   Prepare a completion variable to wait for a platform device to probe

**Parameters**

`struct kunit *test`
:   test context

`struct platform_device *pdev`
:   platform device to prepare to wait for probe of

`struct completion *x`
:   completion variable completed when **dev** has probed

**Description**

Prepare a completion variable **x** to wait for **pdev** to probe. Waiting on the
completion forces a preemption, allowing the platform driver to probe.

Example

```
static int kunit_platform_driver_probe(struct platform_device *pdev)
{
        return 0;
}

static void kunit_platform_driver_test(struct kunit *test)
{
        struct platform_device *pdev;
        struct platform_driver *pdrv;
        DECLARE_COMPLETION_ONSTACK(comp);

        pdev = kunit_platform_device_alloc(test, "kunit-platform", -1);
        KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);
        KUNIT_ASSERT_EQ(test, 0, kunit_platform_device_add(test, pdev));

        pdrv = kunit_kzalloc(test, sizeof(*pdrv), GFP_KERNEL);
        KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdrv);

        pdrv->probe = kunit_platform_driver_probe;
        pdrv->driver.name = "kunit-platform";
        pdrv->driver.owner = THIS_MODULE;

        KUNIT_ASSERT_EQ(test, 0, kunit_platform_device_prepare_wait_for_probe(test, pdev, &comp));
        KUNIT_ASSERT_EQ(test, 0, kunit_platform_driver_register(test, pdrv));

        KUNIT_EXPECT_NE(test, 0, wait_for_completion_timeout(&comp, 3 * HZ));
}
```

**Return**

0 on success, negative errno on failure.

int kunit\_platform\_driver\_register(struct [kunit](test.html#c.kunit "kunit") \*test, struct platform\_driver \*drv)
:   Register a KUnit test managed platform driver

**Parameters**

`struct kunit *test`
:   test context

`struct platform_driver *drv`
:   platform driver to register

**Description**

Register a test managed platform driver. This allows callers to embed the
**drv** in a container structure and use [`container_of()`](../../../driver-api/basics.html#c.container_of "container_of") in the probe function
to pass information to KUnit tests.

Example

```
struct kunit_test_context {
        struct platform_driver pdrv;
        const char *data;
};

static inline struct kunit_test_context *
to_test_context(struct platform_device *pdev)
{
        return container_of(to_platform_driver(pdev->dev.driver),
                            struct kunit_test_context,
                            pdrv);
}

static int kunit_platform_driver_probe(struct platform_device *pdev)
{
        struct kunit_test_context *ctx;

        ctx = to_test_context(pdev);
        ctx->data = "test data";

        return 0;
}

static void kunit_platform_driver_test(struct kunit *test)
{
        struct kunit_test_context *ctx;

        ctx = kunit_kzalloc(test, sizeof(*ctx), GFP_KERNEL);
        KUNIT_ASSERT_NOT_ERR_OR_NULL(test, ctx);

        ctx->pdrv.probe = kunit_platform_driver_probe;
        ctx->pdrv.driver.name = "kunit-platform";
        ctx->pdrv.driver.owner = THIS_MODULE;

        KUNIT_EXPECT_EQ(test, 0, kunit_platform_driver_register(test, &ctx->pdrv));
        <... wait for driver to probe ...>
        KUNIT_EXPECT_STREQ(test, ctx->data, "test data");
}
```

**Return**

0 on success, negative errno on failure.
