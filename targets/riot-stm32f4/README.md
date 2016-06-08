### About

This folder contains files to run JerryScript on RIOT-OS with STM32F4-Discovery board.

### How to build

#### 1. Preface

1, Directory structure

Assume `harmony` as the path to the projects to build.
The folder tree related would look like this.

```
harmony
  + jerryscript
  |  + targets
  |      + riot-stm32f4
  + RIOT
```

2, Target board

Assume [STM32F4-Discovery with BB](http://www.st.com/web/en/catalog/tools/FM116/SC959/SS1532/LN1199/PF255417)
as the target board.

#### 2. Prepare RIOT-OS

Follow [this](https://www.riot-os.org/#download) page to get the RIOT-OS source.

Follow the [Inroduction](https://github.com/RIOT-OS/RIOT/wiki/Introduction) wiki site and also check that you can flash the stm32f4-board.


#### 3. Build JerryScript for RIOT-OS

```
# assume you are in harmony folder
cd jerryscript
make -f ./targets/riot-stm32f4/Makefile.riot
```

This will generate the following libraries:
```
/build/bin/release.riotstm32f4/librelease.jerry-core.a
/build/bin/release.riotstm32f4/librelease.jerry-libm.lib.a
```

This will copy one library files to `targets/riot-stm32f4/bin` folder:
```
libjerrycore.a
```

This will create a hex file in the `targets/riot-stm32f4/bin` folder:
```
riot_jerry.elf
```

#### 4. Flashing

```
make -f ./targets/riot-stm32f4/Makefile.riot flash
```

For how to flash the image with other alternative way can be found here:
[Alternative way to flash](https://github.com/RIOT-OS/RIOT/wiki/Board:-STM32F4discovery#alternative-way-to-flash)

#### 5. Cleaning

To clean the build result:
```
make -f ./targets/riot-stm32f4/Makefile.riot clean
```


### 5. Running JerryScript Hello World! example

You may have to press `RESET` on the board after the flash.

You can use `minicom` for terminal program, and if the prompt shows like this:
```
main(): This is RIOT! (Version: ****)
                                     You are running RIOT on a(n) stm32f4discovery board.
                                                                                         This board features a(n) stm32f4 MCU.
```
please set `Add Carriage Ret` option by `CTRL-A` > `Z` > `U` at the console, if you're using `minicom`.


Help will provide a list of commands:
```
> help
```

The `test` command will run the test example, which contains the following script code:
```
print ('Hello, World!');
```
