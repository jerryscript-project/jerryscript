---
name: Bug report
about: Create a report to help us improve

---

First of all, when reporting a bug, give the issue a descriptive title.

In the body of the issue, optionally give a free-form text description of the
bug. Give the context necessary for others to understand the problem.

Then, provide information necessary to reproduce the bug.
Omit sections that are irrelevant for the bug report, but note that information
like git revision, build platform, build command, and test case are required in
almost all cases.

###### JerryScript revision
Identify the git hash(es) or tag(s) where the issue was observed.

###### Build platform
Name the build platform. E.g., copy the output of
`echo "$(lsb_release -ds) ($(uname -mrs))"` (on Linux),
`echo "$(sw_vers -productName) $(sw_vers -productVersion) ($(uname -mrs))"` (on macOS), or
`python -c "import platform; print(platform.platform())"` (should work everywhere).

###### Build steps
Describe how to build JerryScript. Give all the necessary details of the build
(e.g., environment variables, command(s), profile, command line options, etc.).

E.g.:
```sh
tools/build.py --clean --debug
```
Or:
```sh
mkdir build && cmake -H. -Bbuild && make -C build
```

Even if the bug was originally observed when JerryScript was integrated into a
larger project, try to reproduce it with as few external code as possible,
preferably by building the `jerry` command line tool.

###### Build log
Copy the build log if the reported issue is a build problem. Do a verbose build
if necessary. Try and trim the log to relevant parts.

###### Test case
Give the JavaScript input that should be passed to the engine to trigger the
bug. Try and post a reduced test case that is minimally necessary to reproduce
the issue. As a rule of thumb, use Markdown's fenced code block syntax for the
test case. Attach the file (renamed to .txt) if the test case contains
'problematic' bytes that cannot be copied in the bug report directly.

###### Execution platform
Unnecessary if the same as the build platform.

###### Execution steps
List the steps that trigger the bug.

E.g., if a bug is snapshot-specific:
```sh
build/bin/jerry-snapshot generate -o testcase.js.snapshot testcase.js
build/bin/jerry --exec-snapshot testcase.js.snapshot
```

Unnecessary if trivial (i.e., `build/bin/jerry testcase.js`).

###### Output
Copy relevant output from the standard output and/or error channels.

###### Backtrace
In case of a crash (assertion failure, etc.), try to copy the backtrace from a
debugger at the point of failure.

###### Expected behavior
Describe what should happen instead of current behavior. Unnecessary if trivial
(e.g., in case of a crash, the trivial expected behavior is not to crash).
