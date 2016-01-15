### About

This folder contains files to run JerryScript on NuttX with
[STM32F4-Discovery with BB](http://www.st.com/web/en/catalog/tools/FM116/SC959/SS1532/LN1199/PF255417)


### How to build

#### 1. Preface

1, Directory structure

Assume `harmony` as the root folder to the projects to build.
The folder tree related would look like this.

```
harmony
  + jerryscript
  |  + targets
  |      + nuttx-stm32f4
  + nuttx
  |  + nuttx
  |      + lib
  + st-link
```


2, Target board

Assume [STM32F4-Discovery with BB](http://www.st.com/web/en/catalog/tools/FM116/SC959/SS1532/LN1199/PF255417)
as the target board.


3, Micro SD-Card memory for Script source files


#### 2. Prepare NuttX

Follow [this](https://bitbucket.org/seanshpark/nuttx/wiki/Home) page to get
NuttX source and do the first build. When it stops with and error,
change default project from `IoT.js` to `JerryScript` as follows;

2-1) run menuconfig
```
# assume you are in nuttx folder where .config exist
make menuconfig
```
2-2) Select `Application Configuration` --> `Interpreters`

2-3) Check `[*] JerryScript interpreter` (Move cursor to the line and press `Space`)

2-4) `< Exit >` once on the bottom of the sceen (Press `Right arrow` and `Enter`)

2-5) Select `System Libraries and NSH Add-Ons`

2-6) Un-Check `[ ] iotjs program` (Move cursor to the line and press `Space`)

2-7) `< Exit >` till `menuconfig` ends. Save new configugation when asked.

2-7) `make` again
```
make
```

It'll show the last error but it's ok. Nows the time to build JerryScript.


#### 3. Build JerryScript for NuttX

```
# assume you are in harmony folder
cd jerryscript
make -f ./targets/nuttx-stm32f4/Makefile.nuttx
```

If you have NuttX at another path than described above, you can give the
absolute path with `NUTTX` variable , for example,
```
NUTTX=/home/user/work/nuttx make -f ./targets/nuttx-stm32f4/Makefile.nuttx
```

Make will copy three library files to `nuttx/nuttx/lib` folder
```
libjerryentry.a
libjerrycore.a
libfdlibm.a
```

In NuttX, if you run `make clean`, above library files are also removed so you
may have to build JerryScript again.

#### 4. Continue build NuttX

```
# asssume you are in harmony folder
cd nuttx/nuttx
make
```


#### 5. Flashing

Connect Mini-USB for power supply and connect Micro-USB for `NSH` console.

Please refer [this](https://github.com/Samsung/iotjs/wiki/Build-for-NuttX#prepare-flashing-to-target-board)
link to prepare `stlink` utility.


To flash with `Makefile.nuttx`,
```
# assume you are in jerryscript folder
make -f ./targets/nuttx-stm32f4/Makefile.nuttx flash
```

#### 6. Cleaning

To clean the build result,
```
make -f ./targets/nuttx-stm32f4/Makefile.nuttx clean
```


### Running JerryScript

Prepare a micro SD-card and prepare `hello.js` like this in the root directory of SD-card.

```
print("Hello JerryScript!");
```

Power Off(unplug both USB cables), plug the memory card to BB, and power on again.

You can use `minicom` for terminal program, or any other you may like, but match
baud rate to `115200`.

```
minicom --device=/dev/ttyACM0 --baud=115200
```


You may have to press `RESET` on the board and press `Enter` keys on the console
several times to make `nsh` prompt to appear.

If the prompt shows like this,
```
NuttShell (NSH)
               nsh>
                    nsh>
                         nsh>
```
please set `Add Carriage Ret` option by `CTRL-A` > `Z` > `U` at the console,
if you're using `minicom`.


Run `jerryscript` with `hello.js`

```
NuttShell (NSH)
nsh>
nsh>
nsh> jerryscript /mnt/sdcard/hello.js
PARAM 1 : [/mnt/sdcard/hello.js]
Hello JerryScript!
```

Please give absolute path of the script file or may get an error like this.
```
nsh> cd /mnt/sdcard
nsh> jerryscript ./hello.js
PARAM 1 : [./hello.js]
Failed to fopen [./hello.js]
JERRY_STANDALONE_EXIT_CODE_FAIL
nsh>
nsh>
nsh> jerryscript /mnt/sdcard/hello.js
PARAM 1 : [/mnt/sdcard/hello.js]
Hello JerryScript!
```
