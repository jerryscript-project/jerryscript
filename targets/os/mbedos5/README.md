### About

This folder contains files to run JerryScript on
[STM32F4-Discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html) with
[Mbed OS 5](https://os.mbed.com/).
The document had been validated on Ubuntu 20.04 operating system.

#### 1. Setup the build environment

Clone the necessary projects into a `jerry-mbedos` directory.
The latest tested working version of Mbed is `5.15`.

```sh
mkdir jerry-mbedos && cd jerry-mbedos

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://github.com/ARMmbed/mbed-os.git -b mbed-os-5.15
```

The following directory structure has been created:

```
jerry-mbedos
  + jerryscript
  |  + targets
  |      + os
  |        + mbedos5
  + mbed-os
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jerry-mbedos folder.
jerryscript/tools/apt-get-install-deps.sh

sudo apt install stlink-tools
pip install mbed-cli
# Install Python dependencies of Mbed OS.
pip install --user -r mbed-os/requirements.txt
```

#### 4. Build Mbed OS (with JerryScript)

```
# Assuming you are in jerry-mbedos folder.
make -C jerryscript/targets/os/mbedos5 MBED_OS_DIR=${PWD}/mbed-os
```

The created binary is a `mbed-os.bin` named file located in `jerryscript/build/mbed-os` folder.

#### 5. Flash

Connect Mini-USB for charging and flashing the device.

```
# Assuming you are in jerry-riot folder.
sudo st-flash write jerryscript/build/mbed-os/mbed-os.bin 0x8000000
```

#### 6. Connect to the device

Use `USB To TTL Serial Converter` for serial communication. STM32F4-Discovery pins are mapped by Mbed OS as follows:

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

Press `RESET` on the board to get the output of JerryScript application:

```
This test run the following script code: [print ('Hello, World!');]

Hello, World!
```
