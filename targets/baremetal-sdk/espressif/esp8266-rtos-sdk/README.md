### About

This folder contains files to run JerryScript on
[ESP8666 board](https://www.espressif.com/en/products/socs/esp8266) with
[ESP8266 RTOS SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/).
The document had been validated on Ubuntu 20.04 operating system.

### How to build

#### 1. Setup the build environment

Clone the necessary projects into a `jerry-esp` directory.
The latest tested working version of ESP8266 RTOS SDK is `release/v3.4`.

```sh
# Create a base folder for all the projects.
mkdir jerry-esp && cd jerry-esp

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://github.com/espressif/ESP8266_RTOS_SDK -b release/v3.4

# SDK requires Xtensa toolchain.
wget https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
```

The following directory structure has been created:

```
jerry-esp
  + jerryscript
  |  + targets
  |      + baremetal-sdk
  |          + espressif
  |
  + ESP8266_RTOS_SDK
  + xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jerry-esp folder.
jerryscript/tools/apt-get-install-deps.sh

# Install dependencies of ESP8266-RTOS-SDK.
python -m pip install --user -r ESP8266_RTOS_SDK/requirements.txt

# Extract the downloaded toolchain.
tar -xvf xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
```

#### 3. Build ESP8266-RTOS-SDK (with JerryScript)

```sh
# Assuming you are in jerry-esp folder.
PATH=${PWD}/xtensa-lx106-elf/bin:$PATH ESP8266_RTOS_SDK/tools/idf.py \
  --project-dir jerryscript/targets/baremetal-sdk/espressif \
  --build-dir jerryscript/build/esp8266-rtos-sdk \
  all
```

The created binary is a `jerryscript.bin` named file located in `jerryscript/build/esp8266-rtos-sdk` folder.

#### 4. Flash the device

Connect Micro-USB for charging and flashing the device. The device should be visible as /dev/ttyUSB0.

```sh
# Assuming you are in jerry-esp folder.
sudo python ESP8266_RTOS_SDK/components/esptool_py/esptool/esptool.py \
  --port /dev/ttyUSB0 \
  --baud 460800 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  --flash_mode dio \
  --flash_freq 40m \
  --flash_size 2MB \
  0x0 jerryscript/build/esp8266-rtos-sdk/bootloader/bootloader.bin \
  0x8000 jerryscript/build/esp8266-rtos-sdk/partition_table/partition-table.bin \
  0x10000 jerryscript/build/esp8266-rtos-sdk/jerryscript.bin
```

#### 5. Connect to the device

ESP8266-RTOS-SDK provides a Python script for serial communication.
Other programs also can be used such as minicom, picocom, etc.

* The device should be visible as `/dev/ttyUSB0`
* The device uses `74880` baud rate for serial communication
* Press `CTRL` + `ALT` + `]` to exit monitor program

```sh
# Assuming you are in jerry-esp folder.
sudo PATH=${PWD}/xtensa-lx106-elf/bin:$PATH ESP8266_RTOS_SDK/tools/idf_monitor.py \
  --port /dev/ttyUSB0 \
  --baud 78880 \
  --toolchain-prefix=xtensa-lx106-elf- \
  jerryscript/build/esp8266-rtos-sdk/jerryscript.elf
```

The following output should be captured:

```
I (43) boot: ESP-IDF v3.4-43-ge9516e4c 2nd stage bootloader
I (43) boot: compile time 11:14:06
I (43) qio_mode: Enabling default flash chip QIO
I (51) boot: SPI Speed      : 40MHz
I (57) boot: SPI Mode       : QIO
I (63) boot: SPI Flash Size : 2MB
I (69) boot: Partition Table:
I (75) boot: ## Label            Usage          Type ST Offset   Length
I (86) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (97) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (108) boot:  2 factory          factory app      00 00 00010000 000f0000
I (120) boot: End of partition table
I (126) esp_image: segment 0: paddr=0x00010010 vaddr=0x40210010 size=0x5c540 (378176) map
0x40210010: _stext at ??:?

I (140) esp_image: segment 1: paddr=0x0006c558 vaddr=0x4026c550 size=0x0baa8 ( 47784) map
I (153) esp_image: segment 2: paddr=0x00078008 vaddr=0x3ffe8000 size=0x0042c (  1068) load
I (167) esp_image: segment 3: paddr=0x0007843c vaddr=0x40100000 size=0x00080 (   128) load
I (181) esp_image: segment 4: paddr=0x000784c4 vaddr=0x40100080 size=0x0484c ( 18508) load
I (194) boot: Loaded app from partition at offset 0x10000
I (215) JS: This test run the following script code: 
I (216) JS: print ('Hello, World!');
Hello, World!
```
