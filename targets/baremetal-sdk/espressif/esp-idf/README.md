### About

This folder contains files to run JerryScript on
[ESP32 board](https://www.espressif.com/en/products/socs/esp32) with
[ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/).
The document had been validated on Ubuntu 20.04 operating system.

### How to build

#### 1. Setup the build environment

Clone the necessary projects into a `jerry-esp` directory.
The latest tested working version of ESP-IDF SDK is `release/v4.4`.

```sh
# Create a base folder for all the projects.
mkdir jerry-esp && cd jerry-esp

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://github.com/espressif/esp-idf.git -b release/v4.4

# SDK requires Xtensa toolchain.
wget https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_4_0-esp-2021r2-linux-amd64.tar.gz
```

The following directory structure has been created:

```
jerry-esp
  + jerryscript
  |  + targets
  |      + baremetal-sdk
  |          + espressif
  |
  + esp-idf
  + xtensa-esp32-elf-gcc8_4_0-esp-2021r2-linux-amd64.tar.gz
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jerry-esp folder.
jerryscript/tools/apt-get-install-deps.sh

# Install dependencies of esp-idf.
python -m pip install --user -r esp-idf/requirements.txt

# Extract the downloaded toolchain.
tar -xvf xtensa-esp32-elf-gcc8_4_0-esp-2021r2-linux-amd64.tar.gz
```

#### 3. Build ESP-IDF (with JerryScript)

```sh
# Assuming you are in jerry-esp folder.
PATH=${PWD}/xtensa-esp32-elf/bin:$PATH esp-idf/tools/idf.py \
  --project-dir jerryscript/targets/baremetal-sdk/espressif \
  --build-dir jerryscript/build/esp-idf \
  set-target esp32 \
  all
```

The created binary is a `jerrycript.bin` named file located in `jerryscript/build/esp-idf` folder.

#### 4. Flash the device

Connect Micro-USB for charging and flashing the device. The device should be visible as /dev/ttyUSB0.

```sh
# Assuming you are in jerry-esp folder.
sudo python esp-idf/components/esptool_py/esptool/esptool.py \
  --chip esp32 \
  --port /dev/ttyUSB0 \
  --baud 460800 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  --flash_mode dio \
  --flash_size detect \
  --flash_freq 40m \
  0x1000 jerryscript/build/esp-idf/bootloader/bootloader.bin \
  0x8000 jerryscript/build/esp-idf/partition_table/partition-table.bin \
  0x10000 jerryscript/build/esp-idf/jerryscript.bin
```

#### 5. Connect to the device

esp-idf provides a Python script for serial communication.
Other programs also can be used such as minicom, picocom, etc.

* The device should be visible as `/dev/ttyUSB0`
* The device uses `115200` baud rate for serial communication
* Press `CTRL` + `ALT` + `]` to exit monitor program

```sh
# Assuming you are in jerry-esp folder.
PATH=${PWD}/xtensa-esp32-elf/bin:$PATH esp-idf/tools/idf_monitor.py \
  --port /dev/ttyUSB0 \
  --baud 115200 \
  --toolchain-prefix=xtensa-esp32-elf- \
  jerryscript/build/esp-idf/jerryscript.elf
```

The following output should be captured:

```
I (0) cpu_start: App cpu up.
I (326) cpu_start: Pro cpu start user code
I (326) cpu_start: cpu freq: 160000000
I (326) cpu_start: Application information:
I (331) cpu_start: Project name:     jerryscript
I (336) cpu_start: App version:      v2.4.0-278-ge815e540-dirty
I (342) cpu_start: Compile time:     Jan 21 2022 11:38:44
I (349) cpu_start: ELF file SHA256:  2e874acb5a45c0cb...
I (355) cpu_start: ESP-IDF:          v4.4-beta1-284-gd83021a6e8-dirt
I (362) heap_init: Initializing. RAM available for dynamic allocation:
I (369) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (375) heap_init: At 3FFB8510 len 00027AF0 (158 KiB): DRAM
I (381) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (387) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (394) heap_init: At 4008B3E8 len 00014C18 (83 KiB): IRAM
I (401) spi_flash: detected chip: generic
I (405) spi_flash: flash io: dio
I (410) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (420) JS: This test run the following script code: 
I (430) JS: print ('Hello, World!');
Hello, World!
```
