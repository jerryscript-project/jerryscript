### About

This folder contains files to run JerryScript on
[STM32F4-Discovery board](http://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-eval-tools/stm32-mcu-eval-tools/stm32-mcu-discovery-kits/stm32f4discovery.html) with [NuttX](http://nuttx.org/)

### How to build

#### 1. Setting up the build environment for STM32F4-Discovery board

Clone JerryScript and NuttX into jerry-nuttx directory

```
mkdir jerry-nuttx
cd jerry-nuttx
git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://bitbucket.org/nuttx/nuttx.git
git clone https://bitbucket.org/nuttx/apps.git
git clone https://github.com/texane/stlink.git
```

The following directory structure is created after these commands

```
jerry-nuttx
  + apps
  + jerryscript
  |  + targets
  |      + nuttx-stm32f4
  + nuttx
  + stlink
```

#### 2. Adding JerryScript as an interpreter for NuttX

```
cd apps/interpreters
mkdir jerryscript
cp ../../jerryscript/targets/nuttx-stm32f4/* ./jerryscript/
```

#### 3. Configure NuttX

```
# assuming you are in jerry-nuttx folder
cd nuttx/tools

# configure NuttX USB console shell
./configure.sh stm32f4discovery/usbnsh

cd ..
# might require to run "make menuconfig" twice
make menuconfig
```

We must set the following options:

* Change `Build Setup -> Build Host Platform` from _Windows_ to _Linux_
* Enable `System Type -> FPU support`
* Enable `Application Configuration -> Interpreters -> JerryScript`

If you get `kconfig-mconf: not found` error when you run `make menuconfig` you may have to install kconfig-frontends:

```
# assume you are in jerry-nuttx folder
sudo apt-get install gperf flex bison libncurses-dev
git clone https://github.com/jameswalmsley/kconfig-frontends.git
cd kconfig-frontends
./bootstrap
./configure --enable-mconf
make
sudo make install
sudo ldconfig
```

#### 4. Build JerryScript for NuttX

```
# assuming you are in jerry-nuttx folder
cd nuttx/
make
```

#### 5. Flashing

Connect Mini-USB for power supply and connect Micro-USB for `NSH` console.

To configure `stlink` utility for flashing, follow the instructions in the official [Stlink repository](https://github.com/texane/stlink).

To flash,
```
# assuming you are in nuttx folder
sudo ../stlink/build/Release/st-flash write nuttx.bin 0x8000000
```

### Running JerryScript

You can use `minicom` for terminal program, or any other you may like, but set
baud rate to `115200`.

```
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
