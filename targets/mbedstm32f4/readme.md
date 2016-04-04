This folder contains files to run JerryScript in mbed / DISCO-F407VG board (STM32F4).

#### yotta

You need to install yotta before proceeding.
Please visit http://yottadocs.mbed.com/#installing-on-linux

#### Yotta module dependecies

This board not supported yet. But there is a way to run mbed OS v3.0 and JerryScript on this board.

- Select the yotta target
```
yotta target stm32f4-disco-gcc
```
This is not an offical target for this board (yet). But you can able to build it.

- Install mbed Drivers
```
yotta install mbed-drivers
```

- Modify the yotta module dependecies
There are two missing target dependecies in the module tree.
First module: mbed-hal-st-stm32f407vg
Second module: cmsis-core-stm32f407xg
You have to add these modules manually.
```
cd yotta_modules/mbed-hal-st-stm32f4
open module.json in an editor
add these lines to the targetDependencies:

"stm32f407vg": {
	"mbed-hal-st-stm32f407vg": "^0.0.1"
}

cd ../cmsis-core-st
open module.json in an editor
add these lines to tha targetDependencies:

"stm32f407xg": {
	"cmsis-core-stm32f407xg": "^0.0.1"
}
```

- Update yotta
After the changes, you have to update or build yotta
```
yotta build
```

Note: if you run into a module version error after modified the modules tree, you have to check the `mbed-hal-st-stm32f407vg module.json` file’s dependencies block. The correct uvisor-lib version specification is “>=1.0.2”. Should looks like this:

```
"dependencies": {
    "cmsis-core": "^1.0.0",
    "uvisor-lib": ">=1.0.2"
 }
```

- Check the yotta "module tree"
```
yotta ls
```
If everything is good, you will get this output:
```
jerry 0.0.0
┗━ mbed-drivers 0.11.8
  ┣━ mbed-hal 1.2.1 yotta_modules/mbed-hal
  ┃ ┗━ mbed-hal-st 1.0.0 yotta_modules/mbed-hal-st
  ┃   ┗━ mbed-hal-st-stm32f4 1.1.2 yotta_modules/mbed-hal-st-stm32f4
  ┃     ┣━ uvisor-lib 1.0.12 yotta_modules/uvisor-lib
  ┃     ┣━ mbed-hal-st-stm32cubef4 1.0.2 yotta_modules/mbed-hal-st-stm32cubef4
  ┃     ┗━ mbed-hal-st-stm32f407vg 0.0.1 yotta_modules/mbed-hal-st-stm32f407vg
  ┣━ cmsis-core 1.1.1 yotta_modules/cmsis-core
  ┃ ┗━ cmsis-core-st 1.0.0 yotta_modules/cmsis-core-st
  ┃   ┣━ cmsis-core-stm32f4 1.0.5 yotta_modules/cmsis-core-stm32f4
  ┃   ┗━ cmsis-core-stm32f407xg 0.0.1 yotta_modules/cmsis-core-stm32f407xg
  ┣━ ualloc 1.0.3 yotta_modules/ualloc
  ┃ ┗━ dlmalloc 1.0.0 yotta_modules/dlmalloc
  ┣━ minar 1.0.4 yotta_modules/minar
  ┃ ┗━ minar-platform 1.0.0 yotta_modules/minar-platform
  ┃   ┗━ minar-platform-mbed 1.0.0 yotta_modules/minar-platform-mbed
  ┣━ core-util 1.1.5 yotta_modules/core-util
  ┗━ compiler-polyfill 1.2.1 yotta_modules/compiler-polyfill
```

##### Cross-compiler for DISCO-F4

If you don't have any GCC compiler installed, please visit [this]
(https://launchpad.net/gcc-arm-embedded/+download) page to download GCC 5.2.1
which was tested for building JerryScript for STM32F4.

#### How to build JerryScript
It assumes the current folder is the JerryScript root folder.

```
make -f targets/mbedstm32f4/Makefile.mbedstm32f4 clean
make -f targets/mbedstm32f4/Makefile.mbedstm32f4 jerry
```

#### JerryScript output files

Two files will be generated at `targets/mbedstm32f4/libjerry`
* libjerrycore.a
* libfdlibm.a

#### Building mbed binary

```
make -f targets/mbedstm32f4/Makefile.mbedstm32f4 js2c yotta
```

#### The binary file

The file will be generated at targets/mbedstm32f4/build/stm32f4-disco-gcc/source/jerry.bin

#### Flashing to STM32F4

This borad does not support the drag&drop and Virtual Comm Port features. You will have to use an external tool (for example the STM32 STLink utility) to program your code .bin file.

If you don't have STlink installed, please visit [this]
(https://github.com/texane/stlink) page to install STLink-v2.

```
make -f targets/mbedstm32f4/Makefile.mbedstm32f4 flash
```
OR
```
cd targets/mbedstm32f4/build/stm32f4-disco-gcc/source
st-flash write jerry.bin 0x08000000
```
It assumes the st-flash there is in your /usr/bin folder.

When LED near the USB port flashing stops, press RESET button on the board to execute
JerryScript led flashing sample program in js folder.

#### How blink sample program works

All `.js` files in `js` folder are executed, with `main.js` in first order.
`sysloop()` function in main.js is called periodically in every 100msec by
below code in `main.cpp` `jerry_loop()` (after the `jerrry_init()` finished), which calls `js_loop()`.

```
  minar::Scheduler::postCallback(jerry_loop).period(minar::milliseconds(100))
```

`sysloop()` then calls `blink()` in `blink.js` which blinks the `LED` in
every second.

#### printf

There is no the possibility to use the printf directly in your code.
If you want to use it, you have to use a TTL to USB adapter and a serial communcation program,
like [this program](http://www.cyberciti.biz/tips/connect-soekris-single-board-computer-using-minicom.html) and [this adapter](http://www.cpmspectrepi.webspace.virginmedia.com/raspberry_pi/MoinMoinExport/USBtoTtlSerialAdapters.html).

Connect the adapter to the board with the USBTX, USBRX pin (PA10, PA9 in this case), then the board will appear in your host /dev folder.

```
/dev/ttyUSB0
```

After you connected the board to the host over the adapter and configurated your serial communicate program,
you can able use the printf the following way:
(note: the default baud rate is 9600)

```
Serial pc(USBTX, USBRX); //tx, rx
pc.printf("%s\n\r", str);
```
