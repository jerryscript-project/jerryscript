### About

This folder contains files to run JerryScript on
[STM32F4-Discovery board](http://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-eval-tools/stm32-mcu-eval-tools/stm32-mcu-discovery-kits/stm32f4discovery.html) with [NuttX](http://nuttx.org/)

### How to build

#### 1. Setup the build environment for STM32F4-Discovery board

Clone the necessary projects into a `jerry-nuttx` directory. The last tested working version of NuttX is 7.28.

```sh
# Create a base folder for all the projects.
mkdir jerry-nuttx && cd jerry-nuttx

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://bitbucket.org/nuttx/nuttx.git -b nuttx-7.28
git clone https://bitbucket.org/nuttx/apps.git -b nuttx-7.28
git clone https://github.com/texane/stlink.git -b v1.5.1
```

The following directory structure is created after these commands:

```
jerry-nuttx
  + apps
  + jerryscript
  |  + targets
  |      + nuttx-stm32f4
  + nuttx
  + stlink
```

#### 2. Build JerryScript for NuttX

Build JerryScript as a static library using the NuttX folder as sysroot. The created static libraries will be used later by NuttX.

```sh
# Assuming you are in jerry-nuttx folder.
jerryscript/tools/build.py \
    --clean \
    --lto=OFF \
    --jerry-cmdline=OFF \
    --jerry-libm=ON \
    --all-in-one=ON \
    --mem-heap=70 \
    --profile=es2015-subset \
    --compile-flag="--sysroot=${PWD}/nuttx" \
    --toolchain=${PWD}/jerryscript/cmake/toolchain_mcu_stm32f4.cmake
```

#### 3. Copy JerryScript's application files to NuttX

After creating the static libs (see previous step), it is needed to move the JerryScript application files to the NuttX's interpreter path.

```sh
# Assuming you are in jerry-nuttx folder.
mkdir -p apps/interpreters/jerryscript
cp jerryscript/targets/nuttx-stm32f4/* apps/interpreters/jerryscript/

# Or more simply:
# ln -s jerryscript/targets/nuttx-stm32f4 apps/interpreters/jerryscript
```

#### 4. Configure NuttX

NuttX requires configuration first. The configuration creates a `.config` file in the root folder of NuttX that has all the necessary options for the build.

```sh
# Assuming you are in jerry-nuttx folder.
cd nuttx/tools

# Configure NuttX to use USB console shell.
./configure.sh stm32f4discovery/usbnsh
```

By default, JerryScript is not enabled, so it is needed to modify the configuration file.

##### 4.1 Enable JerryScript without user interaction

```sh
# Assuming you are in jerry-nuttx folder.
sed --in-place "s/CONFIG_HOST_WINDOWS/# CONFIG_HOST_WINDOWS/g" nuttx/.config
sed --in-place "s/CONFIG_WINDOWS_CYGWIN/# CONFIG_WINDOWS_CYGWIN/g" nuttx/.config
sed --in-place "s/CONFIG_TOOLCHAIN_WINDOWS/# CONFIG_TOOLCHAIN_WINDOWS/g" nuttx/.config

cat >> nuttx/.config << EOL
CONFIG_HOST_LINUX=y
CONFIG_ARCH_FPU=y
CONFIG_JERRYSCRIPT=y
CONFIG_JERRYSCRIPT_PRIORITY=100
CONFIG_JERRYSCRIPT_STACKSIZE=16384
EOL
```

##### 4.2 Enable JerryScript using kconfig-frontend

`kconfig-frontend` could be useful if there are another options that should be enabled or disabled in NuttX.

###### 4.2.1 Install kconfig-frontend

```sh
# Assuming you are in jerry-nuttx folder.
git clone https://bitbucket.org/nuttx/tools.git nuttx-tools
cd nuttx-tools/kconfig-frontends

./configure \
    --disable-nconf \
    --disable-gconf \
    --disable-qconf \
    --disable-utils \
    --disable-shared \
    --enable-static \
    --prefix=${PWD}/install

make
sudo make install

# Add the install folder to PATH
PATH=$PATH:${PWD}/install/bin
```

###### 4.2.2 Enable JerryScript
```sh
# Assuming you are in jerry-nuttx folder.
# Might be required to run `make menuconfig` twice.
make -C nuttx menuconfig
```

* Change `Build Setup -> Build Host Platform` to Linux
* Enable `System Type -> FPU support`
* Enable JerryScript `Application Configuration -> Interpreters -> JerryScript`

#### 5. Build NuttX

```sh
# Assuming you are in jerry-nuttx folder.
make -C nuttx
```

#### 6. Flash the device

Connect Mini-USB for power supply and connect Micro-USB for `NSH` console.

```sh
# Assuming you are in jerry-nuttx folder.
make -C stlink release

sudo stlink/build/Release/st-flash write nuttx/nuttx.bin 0x8000000
```

### Running JerryScript

You can use `minicom` for terminal program, or any other you may like, but set
baud rate to `115200`.

```sh
sudo minicom --device=/dev/ttyACM0 --baud=115200
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


Run `jerry` with javascript file(s)

```
NuttShell (NSH)
nsh> jerry full_path/any.js
```

Without argument it prints:
```
nsh> jerry
No input files, running a hello world demo:
Hello world 5 times from JerryScript
```
