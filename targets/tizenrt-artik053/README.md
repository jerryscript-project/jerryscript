### About

This folder contains files to build and run JerryScript on [TizenRT](https://github.com/Samsung/TizenRT) with Artik053 board.

### How to build

#### TL; DR

If you are in a hurry, run the following commands:

```
$ sudo apt-add-repository -y "ppa:team-gcc-arm-embedded/ppa"
$ sudo apt-get update
$ sudo apt-get install gcc-arm-embedded genromfs
$ git clone https://github.com/jerryscript-project/jerryscript.git
$ cd jerryscript
$ make -f targets/tizenrt-artik053/Makefile.travis install
$ make -f targets/tizenrt-artik053/Makefile.travis script
```

Next, go to [step 7](#7-flash-binary)


#### Build steps in detail

#### 1. Set up build environment

* Install toolchain

Get [gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar](https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update).
Untar the archive and export the path.

```
$ tar xvf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar
$ export PATH=<Your Toolchain PATH>:$PATH
```

* Get jerryscript and TizenRT sources.

```
$ mkdir jerry-tizenrt
$ cd jerry-tizenrt
$ git clone https://github.com/jerryscript-project/jerryscript.git
$ git clone https://github.com/Samsung/TizenRT.git -b 2.0_Public_M2
```

The following directory structure is created after these commands:

```
jerry-tizenrt
  ├── jerryscript
  └── TizenRT
```

#### 2. Add jerryscript configuration for TizenRT

```
$ cp -r jerryscript/targets/tizenrt-artik053/apps/jerryscript/ TizenRT/apps/system/
$ cp -r jerryscript/targets/tizenrt-artik053/configs/jerryscript/ TizenRT/build/configs/artik053/
```

#### 3. Configure TizenRT

```
$ cd TizenRT/os/tools
$ ./configure.sh artik053/jerryscript
```

#### 4. Build JerryScript for TizenRT

```
# assuming you are in jerry-tizenrt folder
jerryscript/tools/build.py \
    --clean \
    --lto=OFF \
    --jerry-cmdline=OFF \
    --all-in-one=OFF \
    --mem-heap=70 \
    --profile=es2015-subset \
    --compile-flag="--sysroot=${PWD}/TizenRT/os" \
    --toolchain=${PWD}/jerryscript/cmake/toolchain_mcu_artik053.cmake
```

Alternatively, there is a Makefile in the `targets/tizenrt-artik053/` folder that also helps to build JerryScript for TizenRT.

```
# assuming you are in jerry-tizenrt folder
$ cd jerryscript
$ make -f targets/tizenrt-artik053/Makefile.tizenrt
```

#### 5. Add your JavaScript program to TizenRT (optional)

If you have script files for JerryScript, you can add them to TizenRT.
These files will be flashed into the target's `/rom` folder.
Note that your content cannot exceed 1200 KB.

```
# assuming you are in jerry-tizenrt folder
cp jerryscript/tests/hello.js TizenRT/tools/fs/contents/
```

#### 6. Build TizenRT binary

```
# assuming you are in jerry-tizenrt folder
$ cd TizenRT/os
$ make
```

Binaries are available in `TizenRT/build/output/bin`.

#### 7. Flash binary

```
make download ALL
```

Reboot the device.

For more information, see [How to program a binary](https://github.com/Samsung/TizenRT/blob/master/build/configs/artik053/README.md).


#### 8. Run JerryScript

Use a terminal program (e.g., `minicom`) with baud rate of `115200`.
(Note: Actual device path may vary, e.g., `/dev/ttyUSB1`.)

```
sudo minicom --device=/dev/ttyUSB0 --baud=115200
```

Run `jerry` with javascript file(s):

```
TASH>>jerry /rom/hello.js
Hello JerryScript!
```

Running the program without argument executes a built-in demo:

```
TASH>>jerry                                                                        
No input files, running a hello world demo:                                        
Hello World from JerryScript
```
