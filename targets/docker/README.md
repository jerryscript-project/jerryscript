# `Dockerfile`

# run build

Run from the top-level directory:

```sh
jerryscript $ ls -ah
.               .gitignore      CONTRIBUTING.md LICENSE         README.md       docs            jerry-libm      tests
..              .travis.yml     DCO.md          LOGO.png        cmake           jerry-core      jerry-main      third-party
.git            CMakeLists.txt  Doxyfile        LOGO.svg        docker          jerry-libc      targets         tools
jerryscript $ ./targets/docker/build.sh
```

## output

```sh
=== Summary ===
 - Ran 11552 tests
 - Passed 11546 tests (99.9%)
 - Failed 6 tests (0.1%)

Failed tests
  ch15/15.9/15.9.3/S15.9.3.1_A5_T1 in non-strict mode
  ch15/15.9/15.9.3/S15.9.3.1_A5_T2 in non-strict mode
  ch15/15.9/15.9.3/S15.9.3.1_A5_T3 in non-strict mode
  ch15/15.9/15.9.3/S15.9.3.1_A5_T4 in non-strict mode
  ch15/15.9/15.9.3/S15.9.3.1_A5_T5 in non-strict mode
  ch15/15.9/15.9.3/S15.9.3.1_A5_T6 in non-strict mode

/root/builds/jerryscript/tools/runners/run-test-suite-test262.sh: see /root/builds/jerryscript/build/tests/test262_tests/bin/test262.report for details about failures
The command '/bin/sh -c python ./tools/run-tests.py   --check-vera   --check-license   --buildoption-test   --jerry-tests   --jerry-test-suite   --unittests   --test262' returned a non-zero code: 1
---------
-------
running: 
root@b47e173e56d1:~/builds/jerryscript# 
```