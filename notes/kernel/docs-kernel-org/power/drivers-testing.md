# Testing suspend and resume support in device drivers

> 출처(원문): https://docs.kernel.org/power/drivers-testing.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Testing suspend and resume support in device drivers

> 3. 2007 Rafael J. Wysocki <[rjw@sisk.pl](mailto:rjw%40sisk.pl)>, GPL

## 1. Preparing the test system

Unfortunately, to effectively test the support for the system-wide suspend and
resume transitions in a driver, it is necessary to suspend and resume a fully
functional system with this driver loaded. Moreover, that should be done
several times, preferably several times in a row, and separately for hibernation
(aka suspend to disk or STD) and suspend to RAM (STR), because each of these
cases involves slightly different operations and different interactions with
the machine’s BIOS.

Of course, for this purpose the test system has to be known to suspend and
resume without the driver being tested. Thus, if possible, you should first
resolve all suspend/resume-related problems in the test system before you start
testing the new driver. Please see [Debugging hibernation and suspend](basic-pm-debugging.html)
for more information about the debugging of suspend/resume functionality.

## 2. Testing the driver

Once you have resolved the suspend/resume-related problems with your test system
without the new driver, you are ready to test it:

1. Build the driver as a module, load it and try the test modes of hibernation
   (see: [Debugging hibernation and suspend](basic-pm-debugging.html), 1).
2. Load the driver and attempt to hibernate in the “reboot”, “shutdown” and
   “platform” modes (see: [Debugging hibernation and suspend](basic-pm-debugging.html), 1).
3. Compile the driver directly into the kernel and try the test modes of
   hibernation.
4. Attempt to hibernate with the driver compiled directly into the kernel
   in the “reboot”, “shutdown” and “platform” modes.
5. Try the test modes of suspend (see:
   [Debugging hibernation and suspend](basic-pm-debugging.html), 2). [As far as the STR tests are
   concerned, it should not matter whether or not the driver is built as a
   module.]
6. Attempt to suspend to RAM using the s2ram tool with the driver loaded
   (see: [Debugging hibernation and suspend](basic-pm-debugging.html), 2).

Each of the above tests should be repeated several times and the STD tests
should be mixed with the STR tests. If any of them fails, the driver cannot be
regarded as suspend/resume-safe.
