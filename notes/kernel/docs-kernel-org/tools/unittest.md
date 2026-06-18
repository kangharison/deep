# Python unittest

> 출처(원문): https://docs.kernel.org/tools/unittest.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Python unittest

Checking consistency of python modules can be complex. Sometimes, it is
useful to define a set of unit tests to help checking them.

While the actual test implementation is usecase dependent, Python already
provides a standard way to add unit tests by using `import unittest`.

Using such class, requires setting up a test suite. Also, the default format
is a little bit ackward. To improve it and provide a more uniform way to
report errors, some unittest classes and functions are defined.

## Unittest helper module

Provides helper functions and classes execute python unit tests.

Those help functions provide a nice colored output summary of each
executed test and, when a test fails, it shows the different in diff
format when running in verbose mode, like:

```
$ tools/unittests/nested_match.py -v
...
Traceback (most recent call last):
File "/new_devel/docs/tools/unittests/nested_match.py", line 69, in test_count_limit
    self.assertEqual(replaced, "bar(a); bar(b); foo(c)")
    ~~~~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AssertionError: 'bar(a) foo(b); foo(c)' != 'bar(a); bar(b); foo(c)'
- bar(a) foo(b); foo(c)
?       ^^^^
+ bar(a); bar(b); foo(c)
?       ^^^^^
...
```

It also allows filtering what tests will be executed via `-k` parameter.

Typical usage is to do:

```
from unittest_helper import run_unittest
...

if __name__ == "__main__":
    run_unittest(__file__)
```

If passing arguments is needed, on a more complex scenario, it can be
used like on this example:

```
from unittest_helper import TestUnits, run_unittest
...
env = {'sudo': ""}
...
if __name__ == "__main__":
    runner = TestUnits()
    base_parser = runner.parse_args()
    base_parser.add_argument('--sudo', action='store_true',
                            help='Enable tests requiring sudo privileges')

    args = base_parser.parse_args()

    # Update module-level flag
    if args.sudo:
        env['sudo'] = "1"

    # Run tests with customized arguments
    runner.run(__file__, parser=base_parser, args=args, env=env)
```

*class* lib.python.unittest\_helper.Summary(*\*args*, *\*\*kwargs*)
:   Bases: `TestResult`

    Overrides `unittest.TestResult` class to provide a nice colored
    summary. When in verbose mode, displays actual/expected difference in
    unified diff format.

    addError(*test*, *err*)
    :   Called when an error has occurred. ‘err’ is a tuple of values as
        returned by sys.`exc_info()`.

    addFailure(*test*, *err*)
    :   Called when an error has occurred. ‘err’ is a tuple of values as
        returned by sys.`exc_info()`.

    addSkip(*test*, *reason*)
    :   Called when a test is skipped.

    addSuccess(*test*)
    :   Called when a test has completed successfully

    max\_name\_length
    :   max length of the test names.

    printResults(*verbose*)
    :   Print results using colors if tty.

    startTest(*test*)
    :   Called when the given test is about to be run

    test\_results
    :   Dictionary to store organized test results.

*class* lib.python.unittest\_helper.TestUnits
:   Bases: `object`

    Helper class to set verbosity level.

    This class discover test files, import its unittest classes and
    executes the test on it.

    parse\_args()
    :   Returns a parser for command line arguments.

    run(*caller\_file=None*, *pattern=None*, *suite=None*, *parser=None*, *args=None*, *env=None*)
    :   Execute all tests from the unity test file.

        It contains several optional parameters:

        `caller_file`:
        :   * name of the file that contains test.

              typical usage is to place \_\_file\_\_ at the caller test, e.g.:

              ```
              if __name__ == "__main__":
                  TestUnits().run(__file__)
              ```

        `pattern`:
        :   * optional pattern to match multiple file names. Defaults
              to basename of `caller_file`.

        `suite`:
        :   * an unittest suite initialized by the caller using
              `unittest.TestLoader().discover()`.

        `parser`:
        :   * an argparse parser. If not defined, this helper will create
              one.

        `args`:
        :   * an `argparse.Namespace` data filled by the caller.

        `env`:
        :   * environment variables that will be passed to the test suite

        At least `caller_file` or `suite` must be used, otherwise a
        `TypeError` will be raised.

lib.python.unittest\_helper.flatten\_suite(*suite*)
:   Flatten test suite hierarchy.

lib.python.unittest\_helper.run\_unittest(*fname*)
:   Basic usage of TestUnits class.

    Use it when there’s no need to pass any extra argument to the tests
    with. The recommended way is to place this at the end of each
    unittest module:

    ```
    if __name__ == "__main__":
        run_unittest(__file__)
    ```
