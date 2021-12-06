### About

This folder contains files to run JerryScript on
[STM32F4-Discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html) with
[Zephyr](https://zephyrproject.org/).
The document had been validated on Ubuntu 20.04 operating system.

#### 1. Setup the build environment

Clone the necessary projects into a `jerry-zephyr` directory.
The latest tested working version of Zephyr is `v2.7.0`.

```sh
mkdir jerry-zephyr && cd jerry-zephyr

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://github.com/zephyrproject-rtos/zephyr -b v2.7.0

# Zephyr requires its toolchain.
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.13.2/zephyr-toolchain-arm-0.13.2-linux-x86_64-setup.run
```

The following directory structure has been created:

```
jerry-zephyr
  + jerryscript
  |  + targets
  |      + os
  |        + zephyr
  + zephyr
  + zephyr-toolchain-arm-0.13.2-linux-x86_64-setup.run
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jerry-zephyr folder.
jerryscript/tools/apt-get-install-deps.sh

# Tool dependencies of Zephyr.
sudo apt install --no-install-recommends \
  git cmake ninja-build gperf ccache dfu-util device-tree-compiler \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev

# Install Python dependencies of Zephyr.
pip3 install --user -r zephyr/scripts/requirements.txt

# Install Zephyr toolchain.
chmod +x zephyr-toolchain-arm-0.13.2-linux-x86_64-setup.run
./zephyr-toolchain-arm-0.13.2-linux-x86_64-setup.run -- -y -d ${PWD}/zephyr-toolchain-0.13.2
```

Note: CMake 3.20 is required. If the installed CMake is older, upgrade it for example [this way](https://apt.kitware.com/).

#### 3. Initialize west meta-tool for Zephyr

```
# Assuming you are in jerry-zephyr folder.
west init -l zephyr
west update hal_stm32 cmsis
west zephyr-export
```

#### 4. Build Zephyr (with JerryScript)

```
# Assuming you are in jerry-zephyr folder.
west build -p auto -b stm32f4_disco jerryscript/targets/os/zephyr/
```

The created binary is a `zephyr.elf` named file located in `jerry-zephyr/build/zephyr/bin/` folder.

#### 5. Flash

Install `udev` rules which allows to flash STM32F4-Discovery as a regular user:

```
# Assuming you are in jerry-zephyr folder.
sudo cp zephyr-toolchain-0.13.2/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d
sudo udevadm control --reload
```

Connect Mini-USB for charging and flasing the device.

```
# Assuming you are in jerry-zephyr folder.
west flash
```

#### 6. Connect to the device

Use `USB To TTL Serial Converter` for serial communication. STM32F4-Discovery pins are mapped by Zephyr as follows:

```
  STM32f4-Discovery PA2 pin is configured for TX.
  STM32f4-Discovery PA3 pin is configured for RX.
```

* Connect `STM32f4-Discovery` **PA2** pin to **RX** pin of `USB To TTL Serial Converter`
* Connect `STM32f4-Discovery` **PA3** pin to **TX** pin of `USB To TTL Serial Converter`
* Connect `STM32f4-Discovery` **GND** pin to **GND** pin of `USB To TTL Serial Converter`

The device should be visible as `/dev/ttyUSB0`. Use `minicom` communication program with `115200`.

* In `minicom`, set `Add Carriage Ret` to `off` in by `CTRL-A -> Z -> U` key combinations.
* In `minicom`, set `Hardware Flow Control` to `no` by `CTRL-A -> Z -> O -> Serial port setup -> F` key combinations.

```sh
sudo minicom --device=/dev/ttyUSB0 --baud=115200
```

Press `RESET` on the board to get the initial message with JerryScript command prompt:

```
*** Booting Zephyr OS build v2.7.99-1786-ga08b65ef42db  ***
JerryScript build: Nov 25 2021 14:17:17
JerryScript API 3.0.0
Zephyr version 2.7.99
js>
```

Run the following JavaScript example:

```
js> var test = 0; for (t = 100; t < 1000; t++) test += t; print ('Hello World! ' + test);
Hello World! 494550
undefined
```
