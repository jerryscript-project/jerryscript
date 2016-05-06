This folder contains files to run JerryScript in mbed / for:

* Freedom-K64F (k64)
* Discovery-STM32F4 (stm32f4)
* Discovery-STM32F429ZI (stm32f429i)
* Nucleo-F401RE (nucleo)

####Yotta
You need to install yotta before proceeding. Please visit [Yotta docs page](http://yottadocs.mbed.com/#installing-on-linux).

####Cross-compiler
For cross-compilation the GCC 5.2.1 is suggested to be used. All the supported targets were tested with this version. If you don't have any GCC compiler installed, please visit [this](https://launchpad.net/gcc-arm-embedded/+download) page to download GCC 5.2.1.

####How to build a target
Navigate to your JerryScript root folder (after you cloned this repository into the targets folder) and use the following command:

```
make -f targets/mbed/Makefile.mbed board=$(TARGET)
```
Where the `$(TARGET)` is one of the following options: `k64f`, `stm32f4`, `stm32f429i` or `nucleo`.
This command will create a new folder for your target and build the jerryscript and mbed OS into that folder.

####How to build a completely new target
If you want to build a new target (which is not available in this folder) you have to modify the makefile.
You have to add the new board name to the `TARGET_LIST` and you have to add a new branch with the new `YOTTA_TARGET` and a new `TARGET_DIR` path (if it neccessary) to the if at the top in the Makefile (just as you see right now).
There is a little code snippet: 

```
ifeq ($(TARGET), k64f)
  YOTTA_TARGET  = frdm-k64f-gcc
  TARGET_DIR   ?= /media/$(USER)/MBED
else ifeq ($(TARGET), stm32f4)
  YOTTA_TARGET  = 
```

Basically, you can create a new target in this way (If the mbed OS support your board).

#####Let's get into the details!
1. The next rule is the `jerry` rule. This rule builds the JerryScript and copy the output files into the target libjerry folder. Two files will be generated at `targets/mbed/libjerry`:
  * libjerrycore.a
  * libfdlibm.a

  You can run this rule with the following command: 
  - `make -f targets/mbed/Makefile.mbed board=$(TARGET) jerry`

2. The next rule is the `js2c`. This rule calls a `js2c.py` python script from the `jerryscript/targets/tools` and creates the JavaScript builtin file into the `targets/mbed/source/` folder. This file is the `jerry_targetjs.h`. You can run this rule with the follwoing command:

  - `make -f targets/mbed/Makefile.mbed board=$(TARGET) js2c`

3. The last rule is the `yotta`. This rule sets the yotta target and install the mbed-drivers module, install the dependencies for the mbed OS and finaly creates the mbed binary file. The binary file will be genrated at `targets/mbed/build/$(YOTTA_TARGET)/source/jerry.bin`. You can run this rule with the following command: 

  - `make -f targets/mbed/Makefile.mbed board=$(TARGET) yotta`

4. Optional rule: `clean`. It removes the build folder from the mbed and jerry. You can run this rule with this command:

  - `make -f targets/mbed/Makefile.mbed board=$(TARGET) clean`

#####Flashing
When the build is finished you can flash the binary into your board if you want. In case of ST boards you have to install the `st-link` software. Please visit [this page](https://github.com/texane/stlink) to install STLink-v2.
You can flash your binary into your board with the following command:
```
make -f targets/mbed/Makefile.mbed board=$(TARGET) flash
```
The flash rule grabs the binary and copies it to the mounted board or use the STLink-v2 to flash.
When the status LED of the board stops blinking, press RESET button on the board to execute JerryScript led flashing sample program in js folder.

###Note
If you use an STM32F4 board your build will stop with missing header errors. To fix this error please visit to [this page](http://browser.sed.hu/blog/20160407/how-run-javascripts-jerryscript-mbed) and read about the fix in the `New target for STM32F4` block.
