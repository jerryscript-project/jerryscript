### About

This folder contains files to run JerryScript on
[FRDM-K64F board](https://os.mbed.com/platforms/frdm-k64f/) with
[Mbed OS](https://os.mbed.com/).
The document had been validated on Ubuntu 20.04 operating system.

#### 1. Setup the build environment

Clone the necessary projects into a `jerry-mbedos` directory.
The latest tested working version of Mbed is `mbed-os-6.15.0`.

```sh
mkdir jerry-mbedos && cd jerry-mbedos

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://github.com/ARMmbed/mbed-os.git -b mbed-os-6.15.0
```

The following directory structure has been created:

```
jerry-mbedos
  + jerryscript
  |  + targets
  |      + os
  |        + mbedos
  + mbed-os
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jerry-mbedos folder.
jerryscript/tools/apt-get-install-deps.sh

sudo apt install stlink-tools
pip install --user mbed-cli
# Install Python dependencies of Mbed OS.
pip install --user -r mbed-os/requirements.txt
```

#### 3. Build Mbed OS (with JerryScript)

```
# Assuming you are in jerry-mbedos folder.
make -C jerryscript/targets/os/mbedos MBED_OS_DIR=${PWD}/mbed-os
```

The created binary is a `mbedos.bin` named file located in `jerryscript/build/mbed-os` folder.

#### 4. Flash

Connect Micro-USB for charging and flashing the device.

```
# Assuming you are in jerry-mbedos folder.
make -C jerryscript/targets/os/mbedos MBED_OS_DIR=${PWD}/mbed-os flash
```

#### 5. Connect to the device

The device should be visible as `/dev/ttyACM0` on the host.
You can use `minicom` communication program with `115200` baud rate.

```sh
sudo minicom --device=/dev/ttyACM0 --baud=115200
```

Set `Add Carriage Ret` option in `minicom` by `CTRL-A -> Z -> U` key combinations.
Press `RESET` on the board to get the output of JerryScript application:

```
This test run the following script code: [print ('Hello, World!');]

Hello, World!
```
