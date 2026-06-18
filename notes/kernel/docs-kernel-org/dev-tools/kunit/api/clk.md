# Clk API

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/api/clk.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Clk API

The KUnit clk API is used to test clk providers and clk consumers.

int clk\_prepare\_enable\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct [clk](#c.clk_prepare_enable_kunit "clk") \*clk)
:   Test managed `clk_prepare_enable()`

**Parameters**

`struct kunit *test`
:   The test context

`struct clk *clk`
:   clk to prepare and enable

**Return**

0 on success, or negative errno on failure.

struct clk \*clk\_get\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct [device](../../../driver-api/infrastructure.html#c.device "device") \*dev, const char \*con\_id)
:   Test managed [`clk_get()`](../../../core-api/kernel-api.html#c.clk_get "clk_get")

**Parameters**

`struct kunit *test`
:   The test context

`struct device *dev`
:   device for clock “consumer”

`const char *con_id`
:   clock consumer ID

**Description**

Just like [`clk_get()`](../../../core-api/kernel-api.html#c.clk_get "clk_get"), except the clk is managed by the test case and is
automatically put with [`clk_put()`](../../../core-api/kernel-api.html#c.clk_put "clk_put") after the test case concludes.

**Return**

new clk consumer or ERR\_PTR on failure.

struct clk \*of\_clk\_get\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct device\_node \*np, int index)
:   Test managed `of_clk_get()`

**Parameters**

`struct kunit *test`
:   The test context

`struct device_node *np`
:   device\_node for clock “consumer”

`int index`
:   index in ‘clocks’ property of **np**

**Description**

Just like `of_clk_get()`, except the clk is managed by the test case and is
automatically put with [`clk_put()`](../../../core-api/kernel-api.html#c.clk_put "clk_put") after the test case concludes.

**Return**

new clk consumer or ERR\_PTR on failure.

struct clk \*clk\_hw\_get\_clk\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct clk\_hw \*hw, const char \*con\_id)
:   Test managed `clk_hw_get_clk()`

**Parameters**

`struct kunit *test`
:   The test context

`struct clk_hw *hw`
:   clk\_hw associated with the clk being consumed

`const char *con_id`
:   connection ID string on device

**Description**

Just like `clk_hw_get_clk()`, except the clk is managed by the test case and
is automatically put with [`clk_put()`](../../../core-api/kernel-api.html#c.clk_put "clk_put") after the test case concludes.

**Return**

new clk consumer or ERR\_PTR on failure.

struct clk \*clk\_hw\_get\_clk\_prepared\_enabled\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct clk\_hw \*hw, const char \*con\_id)
:   Test managed `clk_hw_get_clk()` + `clk_prepare_enable()`

**Parameters**

`struct kunit *test`
:   The test context

`struct clk_hw *hw`
:   clk\_hw associated with the clk being consumed

`const char *con_id`
:   connection ID string on device

**Description**

Just like

```
struct clk *clk = clk_hw_get_clk(...);
clk_prepare_enable(clk);
```

except the clk is managed by the test case and is automatically disabled and
unprepared with `clk_disable_unprepare()` and put with [`clk_put()`](../../../core-api/kernel-api.html#c.clk_put "clk_put") after the
test case concludes.

**Return**

new clk consumer that is prepared and enabled or ERR\_PTR on failure.

int clk\_hw\_register\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct [device](../../../driver-api/infrastructure.html#c.device "device") \*dev, struct clk\_hw \*hw)
:   Test managed `clk_hw_register()`

**Parameters**

`struct kunit *test`
:   The test context

`struct device *dev`
:   device that is registering this clock

`struct clk_hw *hw`
:   link to hardware-specific clock data

**Description**

Just like `clk_hw_register()`, except the clk registration is managed by the
test case and is automatically unregistered after the test case concludes.

**Return**

0 on success or a negative errno value on failure.

int of\_clk\_hw\_register\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct device\_node \*node, struct clk\_hw \*hw)
:   Test managed `of_clk_hw_register()`

**Parameters**

`struct kunit *test`
:   The test context

`struct device_node *node`
:   device\_node of device that is registering this clock

`struct clk_hw *hw`
:   link to hardware-specific clock data

**Description**

Just like `of_clk_hw_register()`, except the clk registration is managed by
the test case and is automatically unregistered after the test case
concludes.

**Return**

0 on success or a negative errno value on failure.

int of\_clk\_add\_hw\_provider\_kunit(struct [kunit](test.html#c.kunit "kunit") \*test, struct device\_node \*np, struct clk\_hw \*(\*get)(struct of\_phandle\_args \*clkspec, void \*data), void \*data)
:   Test managed `of_clk_add_hw_provider()`

**Parameters**

`struct kunit *test`
:   The test context

`struct device_node *np`
:   Device node pointer associated with clock provider

`struct clk_hw *(*get)(struct of_phandle_args *clkspec, void *data)`
:   Callback for decoding clk\_hw

`void *data`
:   Context pointer for **get** callback.

**Description**

Just like `of_clk_add_hw_provider()`, except the clk\_hw provider is managed by
the test case and is automatically unregistered after the test case
concludes.

**Return**

0 on success or a negative errno value on failure.
