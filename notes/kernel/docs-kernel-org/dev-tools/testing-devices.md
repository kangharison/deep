# Device testing with kselftest

> 출처(원문): https://docs.kernel.org/dev-tools/testing-devices.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Device testing with kselftest

There are a few different kselftests available for testing devices generically,
with some overlap in coverage and different requirements. This document aims to
give an overview of each one.

Note: Paths in this document are relative to the kselftest folder
(`tools/testing/selftests`).

Device oriented kselftests:

* Devicetree (`dt`)

  + **Coverage**: Probe status for devices described in Devicetree
  + **Requirements**: None
* Error logs (`devices/error_logs`)

  + **Coverage**: Error (or more critical) log messages presence coming from any
    device
  + **Requirements**: None
* Discoverable bus (`devices/probe`)

  + **Coverage**: Presence and probe status of USB or PCI devices that have been
    described in the reference file
  + **Requirements**: Manually describe the devices that should be tested in a
    YAML reference file (see `devices/probe/boards/google,spherion.yaml` for
    an example)
* Exist (`devices/exist`)

  + **Coverage**: Presence of all devices
  + **Requirements**: Generate the reference (see `devices/exist/README.rst`
    for details) on a known-good kernel

Therefore, the suggestion is to enable the error log and devicetree tests on all
(DT-based) platforms, since they don’t have any requirements. Then to greatly
improve coverage, generate the reference for each platform and enable the exist
test. The discoverable bus test can be used to verify the probe status of
specific USB or PCI devices, but is probably not worth it for most cases.
