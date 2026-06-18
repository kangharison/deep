# Device Tree (OF) API

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/api/of.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Device Tree (OF) API

The KUnit device tree API is used to test device tree (of\_\*) dependent code.

int \_\_of\_overlay\_apply\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, u8 \*overlay\_begin, const u8 \*overlay\_end)
:   Test managed [`of_overlay_fdt_apply()`](../../../devicetree/kernel-api.html#c.of_overlay_fdt_apply "of_overlay_fdt_apply") variant

**Parameters**

`struct kunit *test`
:   test context

`u8 *overlay_begin`
:   start address of overlay to apply

`const u8 *overlay_end`
:   end address of overlay to apply

**Description**

This is mostly internal API. See [`of_overlay_apply_kunit()`](#c.of_overlay_apply_kunit "of_overlay_apply_kunit") for the wrapper
that makes this easier to use.

Similar to [`of_overlay_fdt_apply()`](../../../devicetree/kernel-api.html#c.of_overlay_fdt_apply "of_overlay_fdt_apply"), except the overlay is managed by the test
case and is automatically removed with [`of_overlay_remove()`](../../../devicetree/kernel-api.html#c.of_overlay_remove "of_overlay_remove") after the test
case concludes.

**Return**

0 on success, negative errno on failure

of\_overlay\_apply\_kunit

`of_overlay_apply_kunit (test, overlay_name)`

> Test managed [`of_overlay_fdt_apply()`](../../../devicetree/kernel-api.html#c.of_overlay_fdt_apply "of_overlay_fdt_apply") for built-in overlays

**Parameters**

`test`
:   test context

`overlay_name`
:   name of overlay to apply

**Description**

This macro is used to apply a device tree overlay built with the
cmd\_dt\_S\_dtbo rule in scripts/Makefile.lib that has been compiled into the
kernel image or KUnit test module. The overlay is automatically removed when
the test is finished.

Unit tests that need device tree nodes should compile an overlay file with
**overlay\_name**.dtbo.o in their Makefile along with their unit test and then
load the overlay during their test. The **overlay\_name** matches the filename
of the overlay without the dtbo filename extension. If CONFIG\_OF\_OVERLAY is
not enabled, the **test** will be skipped.

In the Makefile

```
obj-$(CONFIG_OF_OVERLAY_KUNIT_TEST) += overlay_test.o kunit_overlay_test.dtbo.o
```

In the test

```
static void of_overlay_kunit_of_overlay_apply(struct kunit *test)
{
        struct device_node *np;

        KUNIT_ASSERT_EQ(test, 0,
                        of_overlay_apply_kunit(test, kunit_overlay_test));

        np = of_find_node_by_name(NULL, "test-kunit");
        KUNIT_EXPECT_NOT_ERR_OR_NULL(test, np);
        of_node_put(np);
}
```

**Return**

0 on success, negative errno on failure.

void of\_root\_kunit\_skip(struct [kunit](test.html#c.kunit "kunit") \*test)
:   Skip test if the root node isn’t populated

**Parameters**

`struct kunit *test`
:   test to skip if the root node isn’t populated

int of\_overlay\_fdt\_apply\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, void \*overlay\_fdt, u32 overlay\_fdt\_size, int \*ovcs\_id)
:   Test managed [`of_overlay_fdt_apply()`](../../../devicetree/kernel-api.html#c.of_overlay_fdt_apply "of_overlay_fdt_apply")

**Parameters**

`struct kunit *test`
:   test context

`void *overlay_fdt`
:   device tree overlay to apply

`u32 overlay_fdt_size`
:   size in bytes of **overlay\_fdt**

`int *ovcs_id`
:   identifier of overlay, used to remove the overlay

**Description**

Just like [`of_overlay_fdt_apply()`](../../../devicetree/kernel-api.html#c.of_overlay_fdt_apply "of_overlay_fdt_apply"), except the overlay is managed by the test
case and is automatically removed with [`of_overlay_remove()`](../../../devicetree/kernel-api.html#c.of_overlay_remove "of_overlay_remove") after the test
case concludes.

**Return**

0 on success, negative errno on failure

void of\_node\_put\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct device\_node \*node)
:   Test managed [`of_node_put()`](../../../devicetree/kernel-api.html#c.of_node_put "of_node_put")

**Parameters**

`struct kunit *test`
:   test context

`struct device_node *node`
:   node to pass to [`of_node_put()`](../../../devicetree/kernel-api.html#c.of_node_put "of_node_put")

**Description**

Just like [`of_node_put()`](../../../devicetree/kernel-api.html#c.of_node_put "of_node_put"), except the node is managed by the test case and is
automatically put with [`of_node_put()`](../../../devicetree/kernel-api.html#c.of_node_put "of_node_put") after the test case concludes.
