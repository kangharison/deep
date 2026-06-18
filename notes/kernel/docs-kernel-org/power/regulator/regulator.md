# Regulator Driver Interface

> 출처(원문): https://docs.kernel.org/power/regulator/regulator.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Regulator Driver Interface

The regulator driver interface is relatively simple and designed to allow
regulator drivers to register their services with the core framework.

## Registration

Drivers can register a regulator by calling:

```
struct regulator_dev *regulator_register(struct regulator_desc *regulator_desc,
                                         const struct regulator_config *config);
```

This will register the regulator’s capabilities and operations to the regulator
core.

Regulators can be unregistered by calling:

```
void regulator_unregister(struct regulator_dev *rdev);
```

## Regulator Events

Regulators can send events (e.g. overtemperature, undervoltage, etc) to
consumer drivers by calling:

```
int regulator_notifier_call_chain(struct regulator_dev *rdev,
                                  unsigned long event, void *data);
```
