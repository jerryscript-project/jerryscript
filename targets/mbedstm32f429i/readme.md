This folder contains files to run JerryScript in mbed / DISCO-F429ZI board.

#### IMPORTANT
You have to clone this repository into the JerryScript targets folder! This is a new target,
not an independent application!

#### yotta

You need to install yotta before proceeding.
Please visit http://yottadocs.mbed.com/#installing-on-linux

##### Cross-compiler for DISCO-F429ZI

If you don't have any GCC compiler installed, please visit [this]
(https://launchpad.net/gcc-arm-embedded/+download) page to download GCC 4.8.4
which was tested for building JerryScript for STM32F429ZI.

#### How to build JerryScript
It assumes the current folder is the JerryScript root folder.

```
make -f targets/mbedstm32f429i/Makefile.mbedstm32f429i clean
make -f targets/mbedstm32f429i/Makefile.mbedstm32f429i jerry
```

#### JerryScript output files

Two files will be generated at `targets/mbedstm32f429i/libjerry`
* libjerrycore.a
* libfdlibm.a

#### Building mbed binary

```
make -f targets/mbedstm32f429i/Makefile.mbedstm32f429i js2c yotta
```

Or, cd to `targets/mbedstm32f429i` where `Makefile.mbedstm32f429i` exist

```
cd targets/mbedstm32f429i
yotta target stm32f429i-disco-gcc
yotta build
```

#### The binary file

The file will be generated at targets/mbedstm32f429i/build/stm32f429i-disco-gcc/source/jerry.bin

#### Flashing to STM32F429ZI

This borad does not support the drag&drop and Virtual Comm Port features. You will have to use an external tool (for example the STM32 STLink utility) to program your code .bin file.

If you don't have STlink installed, please visit [this]
(https://github.com/texane/stlink) page to install STLink-v2.

```
make -f targets/mbedstm32f429i/Makefile.mbedstm32f429i flash
```
OR
```
cd targets/mbedstm32f429i/build/stm32f429i-disco-gcc/source
st-flash write jerry.bin 0x08000000
```
It assumes the st-flash there is in your /usr/bin folder.

When LED near the USB port flashing stops, press RESET button on the board to execute
JerryScript led flashing sample program in js folder.

#### How blink sample program works

All `.js` files in `js` folder are executed, with `main.js` in first order.
`sysloop()` function in main.js is called periodically in every 100msec by
below code in `main.cpp` `jerry_loop()`, which calls `js_loop()`.

```
  minar::Scheduler::postCallback(jerry_loop).period(minar::milliseconds(100))
```

`sysloop()` then calls `blink()` in `blink.js` which blinks the `LED` in
every second.

#### printf

There is no the possibility to use the printf directly in your code.
If you want to use it, you have to use a TTL to USB adapter and a serial communcation program,
like [this](http://www.cyberciti.biz/tips/connect-soekris-single-board-computer-using-minicom.html).

Connect the adapter to the board with the USBTX, USBRX pin (PA10, PA9 in this case), then the board will appear in your host /dev folder.

```
/dev/ttyUSB0
```

After you connected the board to the host over the adapter and configurated your serial communicate program,
you can use the printf the following way:

```
Serial pc(USBTX, USBRX); //tx, rx
pc.printf("%s\n\r", str);
```