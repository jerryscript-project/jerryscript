### About

This folder contains files to run JerryScript in mbed / FRDM-K64F board.

#### Installing yotta

You need to install yotta before proceeding.
Please visit http://yottadocs.mbed.com/#installing-on-linux

##### Cross-compiler for FRDM-K64F

If you don't have any GCC compiler installed, please visit [this]
(https://launchpad.net/gcc-arm-embedded/+download) page to download GCC 4.8.4
which was tested for building JerryScript for K64F.

#### How to build JerryScript

```
make -f targets/mbedk64f/Makefile.mbedk64f clean
make -f targets/mbedk64f/Makefile.mbedk64f jerry
```

#### JerryScript output files

Two files will be generated at `targets/mbedk64f/libjerry`
* libjerrycore.a
* libfdlibm.a

#### Building mbed binary

```
make -f targets/mbedk64f/Makefile.mbedk64f js2c yotta
```

Or, cd to `targets/mbedk64f` where `Makefile.mbedk64f` exist

```
cd targets/mbedk64f
yotta target frdm-k64f-gcc
yotta build
```

#### Build at once

Omit target name to build both jerry library and mbed binary.

```
make -f targets/mbedk64f/Makefile.mbedk64f
```

#### Flashing to k64f


```
make -f targets/mbedk64f/Makefile.mbedk64f flash
```

It assumes default mound folder is `/media/(user)/MBED`

If its mounted to other path, give it with `TARGET_DIR` variable, for example,
```
TARGET_DIR=/mnt/media/MBED make -f targets/mbedk64f/Makefile.mbedk64f flash
```

Or you can just copy `targets/mbedk64f/build/frdm-k64f-gcc/source/jerry.bin`
file to mounted folder.

When LED near the USB port flashing stops and `MBED` folder shows up on the
desktop(if you are using GUI), press RESET button on the board to execute
JerryScript led flashing sample program in js folder.

#### How blink sample program works

All `.js` files in `js` folder are executed, with `main.js` in first order.
`sysloop()` function in main.js is called periodically in every 100msec by
below code in `main.cpp` `jerry_loop()`, which calls `js_loop()` in
`jerry_mbedk64f.cpp`

```
  minar::Scheduler::postCallback(jerry_loop)
                      .period(minar::milliseconds(100))
```

`sysloop()` then calls `blink()` in `blink.js` which blinks the `LED` in
every second.
