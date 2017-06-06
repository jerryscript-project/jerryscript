# JerryScript with mbed OS 5

TL;DR? jump straight to [quickstart](#quick-start)

## Introduction

This directory contains the necessary code to build JerryScript for devices
capable of running mbed OS 5. It has been tested with the following boards
so far:

- [Nordic Semiconductor NRF52 Development Kit](https://developer.mbed.org/platforms/Nordic-nRF52-DK/)
- [NXP Freedom K64F](https://developer.mbed.org/platforms/FRDM-K64F/)
- [STM NUCLEO F401RE](https://developer.mbed.org/platforms/ST-Nucleo-F401RE/)
- [Silicon Labs EFM32 Giant Gecko](https://developer.mbed.org/platforms/EFM32-Giant-Gecko/)

## Features

### Peripheral Drivers

Peripheral Drivers are intended as a 1-to-1 mapping to mbed C++ APIs, with a few
differences (due to differences between JavaScript and C++ like lack of operator
overloading).

- [DigitalOut](https://docs.mbed.com/docs/mbed-os-api-reference/en/5.1/APIs/io/DigitalOut/)
- [InterruptIn](https://docs.mbed.com/docs/mbed-os-api-reference/en/5.1/APIs/io/InterruptIn/)
- [I2C](https://docs.mbed.com/docs/mbed-os-api-reference/en/5.1/APIs/interfaces/digital/I2C/)
- setInterval and setTimeout using [mbed-event](https://github.com/ARMmbed/mbed-events)

## Dependencies

### mbed CLI

mbed CLI is used as the build tool for mbed OS 5. You can find out how to install
it in the [official documentation](https://docs.mbed.com/docs/mbed-os-handbook/en/5.1/dev_tools/cli/#installing-mbed-cli).

### arm-none-eabi-gcc

arm-none-eabi-gcc is the only currently tested compiler for jerryscript on mbed,
and instructions for building can be found as part of the mbed-cli installation
instructions above.

### make

make is used to automate the process of fetching dependencies, and making sure that
mbed-cli is called with the correct arguments.

### nodejs

npm is used to install the dependencies in the local node_modules folder.

### gulp

gulp is used to automate tasks, like cloning repositories or generate source files.
If you create an own project, for more info see [mbed-js-gulp](https://github.com/ARMmbed/mbed-js-gulp).

### (optional) jshint

jshint is used to statically check your JavaScript code, as part of the build process.
This ensures that pins you are using in your code are available on your chosen target
platform.

## Quick Start

Once you have all of your dependencies installed, you can build the example project as follows:

```bash
git clone https://github.com/ARMmbed/mbed-js-example
cd mbed-js-example
npm install
gulp --target=YOUR_TARGET_NAME
```

The produced file (in build/out/YOUR_TARGET_NAME) can then be uploaded to your board, and will
run when you press reset.
