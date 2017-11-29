### About

This folder contains files to build and run JerryScript on [TizenRT](https://github.com/Samsung/TizenRT) with Artik053 board.

### How to build

#### TL; DR

If you are in a hurry, run following commands:

```
$ sudo apt-add-repository -y "ppa:team-gcc-arm-embedded/ppa"
$ sudo apt-get update 
$ sudo apt-get install gcc-arm-embedded
$ git clone https://github.com/jerryscript-project/jerryscript.git jerryscript
$ cd jerryscript
$ make -f ./targets/tizenrt-artik053/Makefile.travis install script
```
Next, go to [step 7](#7-generate-romfs)


#### Build steps in detail

#### 1. Set up build environment

* Install toolchain

Get the build in binaries and libraries, [gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar](https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update).


Untar the gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar and export the path like

```
$ tar xvf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar
$ export PATH=<Your Toolchain PATH>:$PATH
```

* Get the jerryscript and TizenRT sources

```
$ mkdir jerry-tizenrt
$ cd jerry-tizenrt
$ git clone https://github.com/jerryscript-project/jerryscript.git
$ git clone https://github.com/Samsung/TizenRT.git -b 1.1_Public_Release
```

The following directory structure is created after these commands

```
jerry-tizenrt
  ├── jerryscript
  └── TizenRT
```

#### 2. Add jerryscript configuration for TizenRT

```
$ cp -r jerryscript/targets/tizenrt-artik053/apps/jerryscript/ TizenRT/apps/system/
$ cp -r jerryscript/targets/tizenrt-artik053/configs/jerryscript/ TizenRT/build/configs/artik053/
$ cp jerryscript/targets/tizenrt-artik053/romfs-1.1.patch TizenRT/
```

#### 3. Configure TizenRT

```
$ cd TizenRT/os/tools
$ ./configure.sh artik053/jerryscript
```

#### 4. Configure TizenRT

```
$ cd ../../
$ patch -p1 < romfs-1.1.patch
$ cd build/output/
$ mkdir res
# You can add files in res folder
# The res folder is later flashing into the target's /rom folder
# CAUTION: You must not exceed 400kb
```

#### 5. Build JerryScript for TizenRT

```
# assuming you are in jerry-tizenrt folder
$ cd jerryscript
$ make -f targets/tizenrt-artik053/Makefile.tizenrt
```

#### 6. Build TizenRT binary

```
# assuming you are in jerry-tizenrt folder
$ cd TizenRT/os
$ make
```
Binaries are available in TizenRT/build/output/bin

#### 7. Generate romfs

```
$ genromfs -f ../build/output/bin/rom.img -d ../build/output/res/ -V "NuttXBootVol"
```

#### 8. Flash binary

```
make download ALL
```

Reboot the device.

For more information, see [How to program a binary](https://github.com/Samsung/TizenRT/blob/master/build/configs/artik053/README.md).


#### 9. Run JerryScript

You can use `minicom` for terminal program, or any other you may like, but set
baud rate to `115200`.

(Note: Device path may differ like /dev/ttyUSB1.)

```
sudo minicom --device=/dev/ttyUSB0 --baud=115200
```

Run `jerry` with javascript file(s)
```
TASH>>jerry hello.js                                                               
Hello JerryScript!
```

Without argument it prints:
```
TASH>>jerry                                                                        
No input files, running a hello world demo:                                        
Hello World from JerryScript
```