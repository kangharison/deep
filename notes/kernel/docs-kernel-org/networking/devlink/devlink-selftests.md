# Devlink Selftests

> 출처(원문): https://docs.kernel.org/networking/devlink/devlink-selftests.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Devlink Selftests

The `devlink-selftests` API allows executing selftests on the device.

## Tests Mask

The `devlink-selftests` command should be run with a mask indicating
the tests to be executed.

## Tests Description

The following is a list of tests that drivers may execute.

List of tests




|  |  |
| --- | --- |
| Name | Description |
| `DEVLINK_SELFTEST_FLASH` | Devices may have the firmware on non-volatile memory on the board, e.g. flash. This particular test helps to run a flash selftest on the device. Implementation of the test is left to the driver/firmware. |

### example usage

```
# Query selftests supported on the devlink device
$ devlink dev selftests show DEV
# Query selftests supported on all devlink devices
$ devlink dev selftests show
# Executes selftests on the device
$ devlink dev selftests run DEV id flash
```
