# Linux Kernel Selftests

> 출처(원문): https://docs.kernel.org/dev-tools/kselftest.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Kernel Selftests

The kernel contains a set of “self tests” under the tools/testing/selftests/
directory. These are intended to be small tests to exercise individual code
paths in the kernel. Tests are intended to be run after building, installing
and booting a kernel.

Kselftest from mainline can be run on older stable kernels. Running tests
from mainline offers the best coverage. Several test rings run mainline
kselftest suite on stable releases. The reason is that when a new test
gets added to test existing code to regression test a bug, we should be
able to run that test on an older kernel. Hence, it is important to keep
code that can still test an older kernel and make sure it skips the test
gracefully on newer releases.

You can find additional information on Kselftest framework, how to
write new tests using the framework on Kselftest wiki:

<https://kselftest.wiki.kernel.org/>

On some systems, hot-plug tests could hang forever waiting for cpu and
memory to be ready to be offlined. A special hot-plug target is created
to run the full range of hot-plug tests. In default mode, hot-plug tests run
in safe mode with a limited scope. In limited mode, cpu-hotplug test is
run on a single cpu as opposed to all hotplug capable cpus, and memory
hotplug test is run on 2% of hotplug capable memory instead of 10%.

kselftest runs as a userspace process. Tests that can be written/run in
userspace may wish to use the [Test Harness](#test-harness). Tests that need to be
run in kernel space may wish to use a [Test Module](#test-module).

## Documentation on the tests

For documentation on the kselftests themselves, see:

* [Device testing with kselftest](testing-devices.html)

## Running the selftests (hotplug tests are run in limited mode)

To build the tests:

```
$ make headers
$ make -C tools/testing/selftests
```

To run the tests:

```
$ make -C tools/testing/selftests run_tests
```

To build and run the tests with a single command, use:

```
$ make kselftest
```

Note that some tests will require root privileges.

Kselftest supports saving output files in a separate directory and then
running tests. To locate output files in a separate directory two syntaxes
are supported. In both cases the working directory must be the root of the
kernel src. This is applicable to “Running a subset of selftests” section
below.

To build, save output files in a separate directory with O=

```
$ make O=/tmp/kselftest kselftest
```

To build, save output files in a separate directory with KBUILD\_OUTPUT

```
$ export KBUILD_OUTPUT=/tmp/kselftest; make kselftest
```

The O= assignment takes precedence over the KBUILD\_OUTPUT environment
variable.

The above commands by default run the tests and print full pass/fail report.
Kselftest supports “summary” option to make it easier to understand the test
results. Please find the detailed individual test results for each test in
/tmp/testname file(s) when summary option is specified. This is applicable
to “Running a subset of selftests” section below.

To run kselftest with summary option enabled

```
$ make summary=1 kselftest
```

## Running a subset of selftests

You can use the “TARGETS” variable on the make command line to specify
single test to run, or a list of tests to run.

To run only tests targeted for a single subsystem:

```
$ make -C tools/testing/selftests TARGETS=ptrace run_tests
```

You can specify multiple tests to build and run:

```
$  make TARGETS="size timers" kselftest
```

To build, save output files in a separate directory with O=

```
$ make O=/tmp/kselftest TARGETS="size timers" kselftest
```

To build, save output files in a separate directory with KBUILD\_OUTPUT

```
$ export KBUILD_OUTPUT=/tmp/kselftest; make TARGETS="size timers" kselftest
```

Additionally you can use the “SKIP\_TARGETS” variable on the make command
line to specify one or more targets to exclude from the TARGETS list.

To run all tests but a single subsystem:

```
$ make -C tools/testing/selftests SKIP_TARGETS=ptrace run_tests
```

You can specify multiple tests to skip:

```
$  make SKIP_TARGETS="size timers" kselftest
```

You can also specify a restricted list of tests to run together with a
dedicated skiplist:

```
$  make TARGETS="breakpoints size timers" SKIP_TARGETS=size kselftest
```

See the top-level tools/testing/selftests/Makefile for the list of all
possible targets.

## Running the full range hotplug selftests

To build the hotplug tests:

```
$ make -C tools/testing/selftests hotplug
```

To run the hotplug tests:

```
$ make -C tools/testing/selftests run_hotplug
```

Note that some tests will require root privileges.

## Install selftests

You can use the “install” target of “make” (which calls the kselftest\_install.sh
tool) to install selftests in the default location (tools/testing/selftests/kselftest\_install),
or in a user specified location via the INSTALL\_PATH “make” variable.

To install selftests in default location:

```
$ make -C tools/testing/selftests install
```

To install selftests in a user specified location:

```
$ make -C tools/testing/selftests install INSTALL_PATH=/some/other/path
```

## Running installed selftests

Found in the install directory, as well as in the Kselftest tarball,
is a script named run\_kselftest.sh to run the tests.

You can simply do the following to run the installed Kselftests. Please
note some tests will require root privileges:

```
$ cd kselftest_install
$ ./run_kselftest.sh
```

To see the list of available tests, the -l option can be used:

```
$ ./run_kselftest.sh -l
```

The -c option can be used to run all the tests from a test collection, or
the -t option for specific single tests. Either can be used multiple times:

```
$ ./run_kselftest.sh -c size -c seccomp -t timers:posix_timers -t timer:nanosleep
```

For other features see the script usage output, seen with the -h option.

## Timeout for selftests

Selftests are designed to be quick and so a default timeout is used of 45
seconds for each test. Tests can override the default timeout by adding
a settings file in their directory and set a timeout variable there to the
configured a desired upper timeout for the test. Only a few tests override
the timeout with a value higher than 45 seconds, selftests strives to keep
it that way. Timeouts in selftests are not considered fatal because the
system under which a test runs may change and this can also modify the
expected time it takes to run a test. If you have control over the systems
which will run the tests you can configure a test runner on those systems to
use a greater or lower timeout on the command line as with the -o or
the --override-timeout argument. For example to use 165 seconds instead
one would use:

```
$ ./run_kselftest.sh --override-timeout 165
```

You can look at the TAP output to see if you ran into the timeout. Test
runners which know a test must run under a specific time can then optionally
treat these timeouts then as fatal.

## Packaging selftests

In some cases packaging is desired, such as when tests need to run on a
different system. To package selftests, run:

```
$ make -C tools/testing/selftests gen_tar
```

This generates a tarball in the INSTALL\_PATH/kselftest-packages directory. By
default, .gz format is used. The tar compression format can be overridden by
specifying a FORMAT make variable. Any value recognized by [tar’s auto-compress](https://www.gnu.org/software/tar/manual/html_node/gzip.html#auto_002dcompress)
option is supported, such as:

```
$ make -C tools/testing/selftests gen_tar FORMAT=.xz
```

make gen\_tar invokes make install so you can use it to package a subset of
tests by using variables specified in [Running a subset of selftests](#running-a-subset-of-selftests)
section:

```
$ make -C tools/testing/selftests gen_tar TARGETS="size" FORMAT=.xz
```

## Contributing new tests

In general, the rules for selftests are

> * Do as much as you can if you’re not root;
> * Don’t take too long;
> * Don’t break the build on any architecture, and
> * Don’t cause the top-level “make run\_tests” to fail if your feature is
>   unconfigured.
> * The output of tests must conform to the TAP standard to ensure high
>   testing quality and to capture failures/errors with specific details.
>   The kselftest.h and kselftest\_harness.h headers provide wrappers for
>   outputting test results. These wrappers should be used for pass,
>   fail, exit, and skip messages. CI systems can easily parse TAP output
>   messages to detect test results.

## Contributing new tests (details)

> * In your Makefile, use facilities from lib.mk by including it instead of
>   reinventing the wheel. Specify flags and binaries generation flags on
>   need basis before including lib.mk.
>
>   ```
>   CFLAGS = $(KHDR_INCLUDES)
>   TEST_GEN_PROGS := close_range_test
>   include ../lib.mk
>   ```
> * Use TEST\_GEN\_XXX if such binaries or files are generated during
>   compiling.
>
>   TEST\_PROGS, TEST\_GEN\_PROGS mean it is the executable tested by
>   default.
>
>   TEST\_GEN\_MODS\_DIR should be used by tests that require modules to be built
>   before the test starts. The variable will contain the name of the directory
>   containing the modules.
>
>   TEST\_CUSTOM\_PROGS should be used by tests that require custom build
>   rules and prevent common build rule use.
>
>   TEST\_PROGS are for test shell scripts. Please ensure shell script has
>   its exec bit set. Otherwise, lib.mk run\_tests will generate a warning.
>
>   TEST\_CUSTOM\_PROGS and TEST\_PROGS will be run by common run\_tests.
>
>   TEST\_PROGS\_EXTENDED, TEST\_GEN\_PROGS\_EXTENDED mean it is the
>   executable which is not tested by default.
>
>   TEST\_FILES, TEST\_GEN\_FILES mean it is the file which is used by
>   test.
>
>   TEST\_INCLUDES is similar to TEST\_FILES, it lists files which should be
>   included when exporting or installing the tests, with the following
>   differences:
>
>   > + symlinks to files in other directories are preserved
>   > + the part of paths below tools/testing/selftests/ is preserved when
>   >   copying the files to the output directory
>
>   TEST\_INCLUDES is meant to list dependencies located in other directories of
>   the selftests hierarchy.
> * First use the headers inside the kernel source and/or git repo, and then the
>   system headers. Headers for the kernel release as opposed to headers
>   installed by the distro on the system should be the primary focus to be able
>   to find regressions. Use KHDR\_INCLUDES in Makefile to include headers from
>   the kernel source.
> * If a test needs specific kernel config options enabled, add a config file in
>   the test directory to enable them.
>
>   e.g: tools/testing/selftests/android/config
> * Create a .gitignore file inside test directory and add all generated objects
>   in it.
> * Add new test name in TARGETS in selftests/Makefile:
>
>   ```
>   TARGETS += android
>   ```
> * All changes should pass:
>
>   ```
>   kselftest-{all,install,clean,gen_tar}
>   kselftest-{all,install,clean,gen_tar} O=abo_path
>   kselftest-{all,install,clean,gen_tar} O=rel_path
>   make -C tools/testing/selftests {all,install,clean,gen_tar}
>   make -C tools/testing/selftests {all,install,clean,gen_tar} O=abs_path
>   make -C tools/testing/selftests {all,install,clean,gen_tar} O=rel_path
>   ```

## Test Module

Kselftest tests the kernel from userspace. Sometimes things need
testing from within the kernel, one method of doing this is to create a
test module. We can tie the module into the kselftest framework by
using a shell script test runner. `kselftest/module.sh` is designed
to facilitate this process. There is also a header file provided to
assist writing kernel modules that are for use with kselftest:

* `tools/testing/selftests/kselftest_module.h`
* `tools/testing/selftests/kselftest/module.sh`

Note that test modules should taint the kernel with TAINT\_TEST. This will
happen automatically for modules which are in the `tools/testing/`
directory, or for modules which use the `kselftest_module.h` header above.
Otherwise, you’ll need to add `MODULE_INFO(test, "Y")` to your module
source. selftests which do not load modules typically should not taint the
kernel, but in cases where a non-test module is loaded, TEST\_TAINT can be
applied from userspace by writing to `/proc/sys/kernel/tainted`.

### How to use

Here we show the typical steps to create a test module and tie it into
kselftest. We use kselftests for lib/ as an example.

1. Create the test module
2. Create the test script that will run (load/unload) the module
   e.g. `tools/testing/selftests/lib/bitmap.sh`
3. Add line to config file e.g. `tools/testing/selftests/lib/config`
4. Add test script to makefile e.g. `tools/testing/selftests/lib/Makefile`
5. Verify it works:

```
# Assumes you have booted a fresh build of this kernel tree
cd /path/to/linux/tree
make kselftest-merge
make modules
sudo make modules_install
make TARGETS=lib kselftest
```

### Example Module

A bare bones test module might look like this:

```
// SPDX-License-Identifier: GPL-2.0+

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "../tools/testing/selftests/kselftest_module.h"

KSTM_MODULE_GLOBALS();

/*
 * Kernel module for testing the foobinator
 */

static int __init test_function()
{
        ...
}

static void __init selftest(void)
{
        KSTM_CHECK_ZERO(do_test_case("", 0));
}

KSTM_MODULE_LOADERS(test_foo);
MODULE_AUTHOR("John Developer <jd@fooman.org>");
MODULE_LICENSE("GPL");
MODULE_INFO(test, "Y");
```

### Example test script

```
#!/bin/bash
# SPDX-License-Identifier: GPL-2.0+
$(dirname $0)/../kselftest/module.sh "foo" test_foo
```

## Test Harness

The kselftest\_harness.h file contains useful helpers to build tests. The
test harness is for userspace testing, for kernel space testing see [Test
Module](#test-module) above.

The tests from tools/testing/selftests/seccomp/seccomp\_bpf.c can be used as
example.

### Example

```
#include "kselftest_harness.h"

TEST(standalone_test) {
  do_some_stuff;
  EXPECT_GT(10, stuff) {
     stuff_state_t state;
     enumerate_stuff_state(&state);
     TH_LOG("expectation failed with state: %s", state.msg);
  }
  more_stuff;
  ASSERT_NE(some_stuff, NULL) TH_LOG("how did it happen?!");
  last_stuff;
  EXPECT_EQ(0, last_stuff);
}

FIXTURE(my_fixture) {
  mytype_t *data;
  int awesomeness_level;
};
FIXTURE_SETUP(my_fixture) {
  self->data = mytype_new();
  ASSERT_NE(NULL, self->data);
}
FIXTURE_TEARDOWN(my_fixture) {
  mytype_free(self->data);
}
TEST_F(my_fixture, data_is_good) {
  EXPECT_EQ(1, is_my_data_good(self->data));
}

TEST_HARNESS_MAIN
```

### Helpers

TH\_LOG

`TH_LOG (fmt, ...)`

**Parameters**

`fmt`
:   format string

`...`
:   optional arguments

**Description**

```
TH_LOG(format, ...)
```

Optional debug logging function available for use in tests.
Logging may be enabled or disabled by defining TH\_LOG\_ENABLED.
E.g., #define TH\_LOG\_ENABLED 1

If no definition is provided, logging is enabled by default.

TEST

`TEST (test_name)`

> Defines the test function and creates the registration stub

**Parameters**

`test_name`
:   test name

**Description**

```
TEST(name) { implementation }
```

Defines a test by name.
Names must be unique and tests must not be run in parallel. The
implementation containing block is a function and scoping should be treated
as such. Returning early may be performed with a bare “return;” statement.

EXPECT\_\* and ASSERT\_\* are valid in a [`TEST()`](#c.TEST "TEST") { } context.

TEST\_SIGNAL

`TEST_SIGNAL (test_name, signal)`

**Parameters**

`test_name`
:   test name

`signal`
:   signal number

**Description**

```
TEST_SIGNAL(name, signal) { implementation }
```

Defines a test by name and the expected term signal.
Names must be unique and tests must not be run in parallel. The
implementation containing block is a function and scoping should be treated
as such. Returning early may be performed with a bare “return;” statement.

EXPECT\_\* and ASSERT\_\* are valid in a [`TEST()`](#c.TEST "TEST") { } context.

FIXTURE\_DATA

`FIXTURE_DATA (datatype_name)`

> Wraps the `struct name` so we have one less argument to pass around

**Parameters**

`datatype_name`
:   datatype name

**Description**

```
FIXTURE_DATA(datatype_name)
```

Almost always, you want just [`FIXTURE()`](#c.FIXTURE "FIXTURE") instead (see below).
This call may be used when the type of the fixture data
is needed. In general, this should not be needed unless
the *self* is being passed to a helper directly.

FIXTURE

`FIXTURE (fixture_name)`

> Called once per fixture to setup the data and register

**Parameters**

`fixture_name`
:   fixture name

**Description**

```
FIXTURE(fixture_name) {
  type property1;
  ...
};
```

Defines the data provided to [`TEST_F()`](#c.TEST_F "TEST_F")-defined tests as *self*. It should be
populated and cleaned up using [`FIXTURE_SETUP()`](#c.FIXTURE_SETUP "FIXTURE_SETUP") and [`FIXTURE_TEARDOWN()`](#c.FIXTURE_TEARDOWN "FIXTURE_TEARDOWN").

FIXTURE\_SETUP

`FIXTURE_SETUP (fixture_name)`

> Prepares the setup function for the fixture. *\_metadata* is included so that EXPECT\_\*, ASSERT\_\* etc. work correctly.

**Parameters**

`fixture_name`
:   fixture name

**Description**

```
FIXTURE_SETUP(fixture_name) { implementation }
```

Populates the required “setup” function for a fixture. An instance of the
datatype defined with [`FIXTURE_DATA()`](#c.FIXTURE_DATA "FIXTURE_DATA") will be exposed as *self* for the
implementation.

ASSERT\_\* are valid for use in this context and will prempt the execution
of any dependent fixture tests.

A bare “return;” statement may be used to return early.

FIXTURE\_TEARDOWN

`FIXTURE_TEARDOWN (fixture_name)`

**Parameters**

`fixture_name`
:   fixture name

**Description**

*\_metadata* is included so that EXPECT\_\*, ASSERT\_\* etc. work correctly.

```
FIXTURE_TEARDOWN(fixture_name) { implementation }
```

Populates the required “teardown” function for a fixture. An instance of the
datatype defined with [`FIXTURE_DATA()`](#c.FIXTURE_DATA "FIXTURE_DATA") will be exposed as *self* for the
implementation to clean up.

A bare “return;” statement may be used to return early.

FIXTURE\_VARIANT

`FIXTURE_VARIANT (fixture_name)`

> Optionally called once per fixture to declare fixture variant

**Parameters**

`fixture_name`
:   fixture name

**Description**

```
FIXTURE_VARIANT(fixture_name) {
  type property1;
  ...
};
```

Defines type of constant parameters provided to [`FIXTURE_SETUP()`](#c.FIXTURE_SETUP "FIXTURE_SETUP"), [`TEST_F()`](#c.TEST_F "TEST_F") and
FIXTURE\_TEARDOWN as *variant*. Variants allow the same tests to be run with
different arguments.

FIXTURE\_VARIANT\_ADD

`FIXTURE_VARIANT_ADD (fixture_name, variant_name)`

> Called once per fixture variant to setup and register the data

**Parameters**

`fixture_name`
:   fixture name

`variant_name`
:   name of the parameter set

**Description**

```
FIXTURE_VARIANT_ADD(fixture_name, variant_name) {
  .property1 = val1,
  ...
};
```

Defines a variant of the test fixture, provided to [`FIXTURE_SETUP()`](#c.FIXTURE_SETUP "FIXTURE_SETUP") and
[`TEST_F()`](#c.TEST_F "TEST_F") as *variant*. Tests of each fixture will be run once for each
variant.

TEST\_F

`TEST_F (fixture_name, test_name)`

> Emits test registration and helpers for fixture-based test cases

**Parameters**

`fixture_name`
:   fixture name

`test_name`
:   test name

**Description**

```
TEST_F(fixture, name) { implementation }
```

Defines a test that depends on a fixture (e.g., is part of a test case).
Very similar to [`TEST()`](#c.TEST "TEST") except that *self* is the setup instance of fixture’s
datatype exposed for use by the implementation.

The \_metadata object is shared (MAP\_SHARED) with all the potential forked
processes, which enables them to use EXCEPT\_\*() and ASSERT\_\*().

The *self* object is only shared with the potential forked processes if
`FIXTURE_TEARDOWN_PARENT()` is used instead of [`FIXTURE_TEARDOWN()`](#c.FIXTURE_TEARDOWN "FIXTURE_TEARDOWN").

TEST\_HARNESS\_MAIN

`TEST_HARNESS_MAIN`

> > Simple wrapper to run the test harness
>
> **Description**
>
> ```
> TEST_HARNESS_MAIN
> ```
>
> Use once to append a `main()` to the test file.

### Operators

Operators for use in [`TEST()`](#c.TEST "TEST") and [`TEST_F()`](#c.TEST_F "TEST_F").
ASSERT\_\* calls will stop test execution immediately.
EXPECT\_\* calls will emit a failure warning, note it, and continue.

ASSERT\_EQ

`ASSERT_EQ (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_EQ(expected, measured): expected == measured

ASSERT\_NE

`ASSERT_NE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_NE(expected, measured): expected != measured

ASSERT\_LT

`ASSERT_LT (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_LT(expected, measured): expected < measured

ASSERT\_LE

`ASSERT_LE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_LE(expected, measured): expected <= measured

ASSERT\_GT

`ASSERT_GT (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_GT(expected, measured): expected > measured

ASSERT\_GE

`ASSERT_GE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_GE(expected, measured): expected >= measured

ASSERT\_NULL

`ASSERT_NULL (seen)`

**Parameters**

`seen`
:   measured value

**Description**

ASSERT\_NULL(measured): NULL == measured

ASSERT\_TRUE

`ASSERT_TRUE (seen)`

**Parameters**

`seen`
:   measured value

**Description**

ASSERT\_TRUE(measured): measured != 0

ASSERT\_FALSE

`ASSERT_FALSE (seen)`

**Parameters**

`seen`
:   measured value

**Description**

ASSERT\_FALSE(measured): measured == 0

ASSERT\_STREQ

`ASSERT_STREQ (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_STREQ(expected, measured): !strcmp(expected, measured)

ASSERT\_STRNE

`ASSERT_STRNE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

ASSERT\_STRNE(expected, measured): strcmp(expected, measured)

EXPECT\_EQ

`EXPECT_EQ (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_EQ(expected, measured): expected == measured

EXPECT\_NE

`EXPECT_NE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_NE(expected, measured): expected != measured

EXPECT\_LT

`EXPECT_LT (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_LT(expected, measured): expected < measured

EXPECT\_LE

`EXPECT_LE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_LE(expected, measured): expected <= measured

EXPECT\_GT

`EXPECT_GT (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_GT(expected, measured): expected > measured

EXPECT\_GE

`EXPECT_GE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_GE(expected, measured): expected >= measured

EXPECT\_NULL

`EXPECT_NULL (seen)`

**Parameters**

`seen`
:   measured value

**Description**

EXPECT\_NULL(measured): NULL == measured

EXPECT\_TRUE

`EXPECT_TRUE (seen)`

**Parameters**

`seen`
:   measured value

**Description**

EXPECT\_TRUE(measured): 0 != measured

EXPECT\_FALSE

`EXPECT_FALSE (seen)`

**Parameters**

`seen`
:   measured value

**Description**

EXPECT\_FALSE(measured): 0 == measured

EXPECT\_STREQ

`EXPECT_STREQ (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_STREQ(expected, measured): !strcmp(expected, measured)

EXPECT\_STRNE

`EXPECT_STRNE (expected, seen)`

**Parameters**

`expected`
:   expected value

`seen`
:   measured value

**Description**

EXPECT\_STRNE(expected, measured): strcmp(expected, measured)
