### About

This folder contains files to build and run JerryScript on [TizenRT](https://github.com/Samsung/TizenRT) with Artik05x board.

### How to build

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
$ git clone https://github.com/Samsung/TizenRT.git tizenrt
```

The following directory structure is created after these commands

```
jerry-tizenrt
├── jerryscript
└── tizenrt
```

#### 2. Add jerryscript configuration for TizenRT

```
$ cp -r jerryscript/targets/tizenrt-artik05x/apps/jerryscript/ tizenrt/apps/system/
$ cp -r jerryscript/targets/tizenrt-artik05x/configs/jerryscript/ tizenrt/build/configs/artik053/
```

Apply following diff in jerry-tizenrt/tizenrt/os/FlatLibs.mk.
(The line number may differ since tizenrt is under developing.)

```diff
--- a/os/FlatLibs.mk
+++ b/os/FlatLibs.mk
@@ -142,6 +142,12 @@ endif
# Add library for Framework
TINYARALIBS += $(LIBRARIES_DIR)$(DELIM)libframework$(LIBEXT)
 
+# Add library for Jerryscript
+ifeq ($(CONFIG_JERRYSCRIPT),y)
+TINYARALIBS += $(LIBRARIES_DIR)$(DELIM)libjerry-core$(LIBEXT)
+TINYARALIBS += $(LIBRARIES_DIR)$(DELIM)libjerry-ext$(LIBEXT)
+TINYARALIBS += $(LIBRARIES_DIR)$(DELIM)libjerry-libm$(LIBEXT)
+endif
+
# Export all libraries
```

#### 3. Configure TizenRT

```
$ cd tizenrt/os/tools
$ ./configure.sh artik053/jerryscript
```

#### 4. Build JerryScript for TizenRT

```
# assuming you are in jerry-tizenrt folder
$ cd jerryscript
$ make -f targets/tizenrt-artik05x/Makefile.tizenrt
```

#### 5. Build TizenRT binary

```
# assuming you are in jerry-tizenrt folder
$ cd tizenrt/os
$ make
```
Binaries are available in tizenrt/build/output/bin

#### 6. Flash binary

```
make download ALL
```

For more information, see [How to program a binary](https://github.com/Samsung/TizenRT/blob/master/build/configs/artik053/README.md).


#### 7. Run JerryScript

You can use `minicom` for terminal program, or any other you may like, but set
baud rate to `115200`.

(Note: Device path may differ like /dev/ttyUSB1.)

```
sudo minicom --device=/dev/ttyUSB0 --baud=115200
```
