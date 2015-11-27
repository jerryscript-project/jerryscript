## Development
### Setting Up Prerequisites
Currently, only Ubuntu 14.04+ officially supported as primary development environment.

There are several dependencies, that should be installed manually. The following list is required for building:
- `gcc` or `g++` higher than `4.8.2`
  - native
  - arm-none-eabi
- `cmake` higher than `2.8.12.2`
- `make` higher than `3.81`
- `bash` higher than `4.3.11`

```bash
sudo apt-get install gcc g++ gcc-arm-none-eabi cmake
```

These tools are required for development:
- `cppcheck` requires `libpcre`
- `vera++` requires `tcl`, `tk` and `boost`

```bash
sudo apt-get install libpcre3 libpcre3-dev
sudo apt-get install tcl8.6 tcl8.6-dev tk8.6-dev libboost-all-dev
```

Upon first build, `make` would try to setup prerequisites, required for further development and pre-commit testing:
- STM32F3 and STM32F4 libraries
- cppcheck 1.66
- vera++ 1.2.1

```bash
make prerequisites -j
```
It may take time, so go grab some coffee:

```bash
Setting up prerequisites... (log file: ./build/prerequisites/prerequisites.log)
```

### Building Debug Version
To build debug version for Linux:
```bash
make debug.linux -j
```

To build debug version for Linux without LTO (Link Time Optimization):
```bash
LTO=OFF make debug.linux -j
```

### Checking Patch
```bash
make precommit -j
```
If some style guidelines, build or test runs fail during precommit, then this is indicated with a message like this:
```
Build failed. See ./build/bin/unittests/make.log for details.
```
