# KUnit - Linux Kernel Unit Testing

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# KUnit - Linux Kernel Unit Testing

Contents:

* [Getting Started](start.html)
  + [Installing Dependencies](start.html#installing-dependencies)
  + [Running tests with kunit\_tool](start.html#running-tests-with-kunit-tool)
  + [Running Tests without the KUnit Wrapper](start.html#running-tests-without-the-kunit-wrapper)
  + [Writing Your First Test](start.html#writing-your-first-test)
  + [Next Steps](start.html#next-steps)
* [KUnit Architecture](architecture.html)
  + [In-Kernel Testing Framework](architecture.html#in-kernel-testing-framework)
  + [kunit\_tool (Command-line Test Harness)](architecture.html#kunit-tool-command-line-test-harness)
* [Running tests with kunit\_tool](run_wrapper.html)
  + [Creating a `.kunitconfig` file](run_wrapper.html#creating-a-kunitconfig-file)
  + [Configuring, building, and running tests](run_wrapper.html#configuring-building-and-running-tests)
  + [Parsing test results](run_wrapper.html#parsing-test-results)
  + [Filtering tests](run_wrapper.html#filtering-tests)
  + [Running tests on QEMU](run_wrapper.html#running-tests-on-qemu)
  + [Running command-line arguments](run_wrapper.html#running-command-line-arguments)
  + [Command-line completion](run_wrapper.html#command-line-completion)
* [Run Tests without kunit\_tool](run_manual.html)
  + [Configure the Kernel](run_manual.html#configure-the-kernel)
  + [debugfs](run_manual.html#debugfs)
  + [Retrieve Test Results](run_manual.html#retrieve-test-results)
  + [Run Tests After Kernel Has Booted](run_manual.html#run-tests-after-kernel-has-booted)
* [Writing Tests](usage.html)
  + [Test Cases](usage.html#test-cases)
  + [Customizing error messages](usage.html#customizing-error-messages)
  + [Writing Tests For Other Architectures](usage.html#writing-tests-for-other-architectures)
* [Common Patterns](usage.html#common-patterns)
  + [Isolating Behavior](usage.html#isolating-behavior)
  + [Testing Against Multiple Inputs](usage.html#testing-against-multiple-inputs)
  + [Allocating Memory](usage.html#allocating-memory)
  + [Registering Cleanup Actions](usage.html#registering-cleanup-actions)
  + [Testing Static Functions](usage.html#testing-static-functions)
  + [Injecting Test-Only Code](usage.html#injecting-test-only-code)
  + [Accessing The Current Test](usage.html#accessing-the-current-test)
  + [Failing The Current Test](usage.html#failing-the-current-test)
  + [Managing Fake Devices and Drivers](usage.html#managing-fake-devices-and-drivers)
* [API Reference](api/index.html)
  + [Core KUnit API](api/index.html#core-kunit-api)
  + [Driver KUnit API](api/index.html#driver-kunit-api)
* [Test Style and Nomenclature](style.html)
  + [Subsystems, Suites, and Tests](style.html#subsystems-suites-and-tests)
  + [Test Kconfig Entries](style.html#test-kconfig-entries)
  + [Test File and Module Names](style.html#test-file-and-module-names)
* [Frequently Asked Questions](faq.html)
  + [How is this different from Autotest, kselftest, and so on?](faq.html#how-is-this-different-from-autotest-kselftest-and-so-on)
  + [Does KUnit support running on architectures other than UML?](faq.html#does-kunit-support-running-on-architectures-other-than-uml)
  + [What is the difference between a unit test and other kinds of tests?](faq.html#what-is-the-difference-between-a-unit-test-and-other-kinds-of-tests)
  + [KUnit is not working, what should I do?](faq.html#kunit-is-not-working-what-should-i-do)
* [Tips For Running KUnit Tests](running_tips.html)
  + [Using `kunit.py run` (“kunit tool”)](running_tips.html#using-kunit-py-run-kunit-tool)
  + [Running tests manually](running_tips.html#running-tests-manually)
  + [Test Attributes and Filtering](running_tips.html#test-attributes-and-filtering)

This section details the kernel unit testing framework.

## Introduction

KUnit (Kernel unit testing framework) provides a common framework for
unit tests within the Linux kernel. Using KUnit, you can define groups
of test cases called test suites. The tests either run on kernel boot
if built-in, or load as a module. KUnit automatically flags and reports
failed test cases in the kernel log. The test results appear in
[KTAP (Kernel - Test Anything Protocol) format](../ktap.html).
It is inspired by JUnit, Python’s unittest.mock, and GoogleTest/GoogleMock
(C++ unit testing framework).

KUnit tests are part of the kernel, written in the C (programming)
language, and test parts of the Kernel implementation (example: a C
language function). Excluding build time, from invocation to
completion, KUnit can run around 100 tests in less than 10 seconds.
KUnit can test any kernel component, for example: file system, system
calls, memory management, device drivers and so on.

KUnit follows the white-box testing approach. The test has access to
internal system functionality. KUnit runs in kernel space and is not
restricted to things exposed to user-space.

In addition, KUnit has kunit\_tool, a script (`tools/testing/kunit/kunit.py`)
that configures the Linux kernel, runs KUnit tests under QEMU or UML
([User Mode Linux](../../virt/uml/user_mode_linux_howto_v2.html)),
parses the test results and
displays them in a user friendly manner.

### Features

* Provides a framework for writing unit tests.
* Runs tests on any kernel architecture.
* Runs a test in milliseconds.

### Prerequisites

* Any Linux kernel compatible hardware.
* For Kernel under test, Linux kernel version 5.5 or greater.

## Unit Testing

A unit test tests a single unit of code in isolation. A unit test is the finest
granularity of testing and allows all possible code paths to be tested in the
code under test. This is possible if the code under test is small and does not
have any external dependencies outside of the test’s control like hardware.

### Write Unit Tests

To write good unit tests, there is a simple but powerful pattern:
Arrange-Act-Assert. This is a great way to structure test cases and
defines an order of operations.

* Arrange inputs and targets: At the start of the test, arrange the data
  that allows a function to work. Example: initialize a statement or
  object.
* Act on the target behavior: Call your function/code under test.
* Assert expected outcome: Verify that the result (or resulting state) is as
  expected.

### Unit Testing Advantages

* Increases testing speed and development in the long run.
* Detects bugs at initial stage and therefore decreases bug fix cost
  compared to acceptance testing.
* Improves code quality.
* Encourages writing testable code.

Read also [What is the difference between a unit test and other kinds of tests?](faq.html#kinds-of-tests).

## How do I use it?

You can find a step-by-step guide to writing and running KUnit tests in
[Getting Started](start.html)

Alternatively, feel free to look through the rest of the KUnit documentation,
or to experiment with tools/testing/kunit/kunit.py and the example test under
lib/kunit/kunit-example-test.c

Happy testing!
