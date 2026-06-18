# 31.In-Field Scan

> 출처(원문): https://docs.kernel.org/arch/x86/ifs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

**In-Field Scan**

# 31. In-Field Scan

## 31.1. Introduction

In Field Scan (IFS) is a hardware feature to run circuit level tests on
a CPU core to detect problems that are not caught by parity or ECC checks.
Future CPUs will support more than one type of test which will show up
with a new platform-device instance-id.

## 31.2. IFS Image

Intel provides firmware files containing the scan tests via the webpage [[1]](#f1).
Look under “In-Field Scan Test Images Download” section towards the
end of the page. Similar to microcode, there are separate files for each
family-model-stepping. IFS Images are not applicable for some test types.
Wherever applicable the sysfs directory would provide a “current\_batch” file
(see below) for loading the image.

[[1](#id2)]

<https://intel.com/InFieldScan>

## 31.3. IFS Image Loading

The driver loads the tests into memory reserved BIOS local to each CPU
socket in a two step process using writes to MSRs to first load the
SHA hashes for the test. Then the tests themselves. Status MSRs provide
feedback on the success/failure of these steps.

The test files are kept in a fixed location: /lib/firmware/intel/ifs\_<n>/
For e.g if there are 3 test files, they would be named in the following
fashion:
ff-mm-ss-01.scan
ff-mm-ss-02.scan
ff-mm-ss-03.scan
(where ff refers to family, mm indicates model and ss indicates stepping)

A different test file can be loaded by writing the numerical portion
(e.g 1, 2 or 3 in the above scenario) into the curent\_batch file.
To load ff-mm-ss-02.scan, the following command can be used:

```
# echo 2 > /sys/devices/virtual/misc/intel_ifs_<n>/current_batch
```

The above file can also be read to know the currently loaded image.

## 31.4. Running tests

Tests are run by the driver synchronizing execution of all threads on a
core and then writing to the ACTIVATE\_SCAN MSR on all threads. Instruction
execution continues when:

1. All tests have completed.
2. Execution was interrupted.
3. A test detected a problem.

Note that ALL THREADS ON THE CORE ARE EFFECTIVELY OFFLINE FOR THE
DURATION OF THE TEST. This can be up to 200 milliseconds. If the system
is running latency sensitive applications that cannot tolerate an
interruption of this magnitude, the system administrator must arrange
to migrate those applications to other cores before running a core test.
It may also be necessary to redirect interrupts to other CPUs.

In all cases reading the corresponding test’s STATUS MSR provides details on what
happened. The driver makes the value of this MSR visible to applications
via the “details” file (see below). Interrupted tests may be restarted.

The IFS driver provides sysfs interfaces via /sys/devices/virtual/misc/intel\_ifs\_<n>/
to control execution:

Test a specific core:

```
# echo <cpu#> > /sys/devices/virtual/misc/intel_ifs_<n>/run_test
```

when HT is enabled any of the sibling cpu# can be specified to test
its corresponding physical core. Since the tests are per physical core,
the result of testing any thread is same. All siblings must be online
to run a core test. It is only necessary to test one thread.

For e.g. to test core corresponding to cpu5

> # echo 5 > /sys/devices/virtual/misc/intel\_ifs\_<n>/run\_test

Results of the last test is provided in /sys:

```
$ cat /sys/devices/virtual/misc/intel_ifs_<n>/status
pass
```

Status can be one of pass, fail, untested

Additional details of the last test is provided by the details file:

```
$ cat /sys/devices/virtual/misc/intel_ifs_<n>/details
0x8081
```

The details file reports the hex value of the test specific status MSR.
Hardware defined error codes are documented in volume 4 of the Intel
Software Developer’s Manual but the error\_code field may contain one of
the following driver defined software codes:

|  |  |
| --- | --- |
| 0xFD | Software timeout |
| 0xFE | Partial completion |

## 31.5. Driver design choices

1) The ACTIVATE\_SCAN MSR allows for running any consecutive subrange of
available tests. But the driver always tries to run all tests and only
uses the subrange feature to restart an interrupted test.

2) Hardware allows for some number of cores to be tested in parallel.
The driver does not make use of this, it only tests one core at a time.

## 31.6. Structural Based Functional Test at Field (SBAF):

SBAF is a new type of testing that provides comprehensive core test
coverage complementing Scan at Field (SAF) testing. SBAF mimics the
manufacturing screening environment and leverages the same test suite.
It makes use of Design For Test (DFT) observation sites and features
to maximize coverage in minimum time.

Similar to the SAF test, SBAF isolates the core under test from the
rest of the system during execution. Upon completion, the core
seamlessly resets to its pre-test state and resumes normal operation.
Any machine checks or hangs encountered during the test are confined to
the isolated core, preventing disruption to the overall system.

Like the SAF test, the SBAF test is also divided into multiple batches,
and each batch test can take hundreds of milliseconds (100-200 ms) to
complete. If such a lengthy interruption is undesirable, it is
recommended to relocate the time-sensitive applications to other cores.

struct ifs\_test\_msrs
:   MSRs used in IFS tests

**Definition**:

```
struct ifs_test_msrs {
    u32 copy_hashes;
    u32 copy_hashes_status;
    u32 copy_chunks;
    u32 copy_chunks_status;
    u32 test_ctrl;
};
```

**Members**

`copy_hashes`
:   Copy test hash data

`copy_hashes_status`
:   Status of copied test hash data

`copy_chunks`
:   Copy chunks of the test data

`copy_chunks_status`
:   Status of the copied test data chunks

`test_ctrl`
:   Control the test attributes

struct ifs\_data
:   attributes related to intel IFS driver

**Definition**:

```
struct ifs_data {
    int loaded_version;
    bool loaded;
    bool loading_error;
    int valid_chunks;
    int status;
    u64 scan_details;
    u32 cur_batch;
    u32 generation;
    u32 chunk_size;
    u32 array_gen;
    u32 max_bundle;
};
```

**Members**

`loaded_version`
:   stores the currently loaded ifs image version.

`loaded`
:   If a valid test binary has been loaded into the memory

`loading_error`
:   Error occurred on another CPU while loading image

`valid_chunks`
:   number of chunks which could be validated.

`status`
:   it holds simple status pass/fail/untested

`scan_details`
:   opaque scan status code from h/w

`cur_batch`
:   number indicating the currently loaded test file

`generation`
:   IFS test generation enumerated by hardware

`chunk_size`
:   size of a test chunk

`array_gen`
:   test generation of array test

`max_bundle`
:   maximum bundle index
