### About

This folder contains files to run JerryScript on
[STM32F4-Discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html) with
[NuttX](https://nuttx.apache.org/).
The document had been validated on Ubuntu 20.04 operating system.

### How to build

#### 1. Setup the build environment for STM32F4-Discovery board

Clone the necessary projects into a `jerry-nuttx` directory.
The latest tested working version of NuttX is 10.2.

```sh
# Create a base folder for all the projects.
mkdir jerry-nuttx && cd jerry-nuttx

git clone https://github.com/jerryscript-project/jerryscript.git
git clone https://github.com/apache/incubator-nuttx.git nuttx -b releases/10.2
git clone https://github.com/apache/incubator-nuttx-apps.git apps -b releases/10.2
git clone https://bitbucket.org/nuttx/tools.git tools
git clone https://github.com/texane/stlink.git -b v1.5.1-patch
```

The following directory structure is created after these commands:

```
jerry-nuttx
  + jerryscript
  |  + targets
  |      + os
  |        + nuttx
  + nuttx
  + apps
  + tools
  + stlink
```

#### 2. Install dependencies of the projects

```sh
# Assuming you are in jerry-nuttx folder.
jerryscript/tools/apt-get-install-deps.sh

# Toolchain dependencies of NuttX.
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# Tool dependencies of NuttX.
sudo apt install \
    bison flex gettext texinfo libncurses5-dev libncursesw5-dev \
    gperf automake libtool pkg-config build-essential gperf genromfs \
    libgmp-dev libmpc-dev libmpfr-dev libisl-dev binutils-dev libelf-dev \
    libexpat-dev gcc-multilib g++-multilib picocom u-boot-tools util-linux

# ST-Link and serial communication dependencies.
sudo apt install libusb-1.0-0-dev minicom
```

#### 3. Copy JerryScript's application files to NuttX

Move JerryScript application files to `apps/interpreters/jerryscript` folder.

```sh
# Assuming you are in jerry-nuttx folder.
mkdir -p apps/interpreters/jerryscript
cp jerryscript/targets/os/nuttx/* apps/interpreters/jerryscript/

# Or more simply:
# ln -s jerryscript/targets/os/nuttx apps/interpreters/jerryscript
```

#### 4. Build kconfig-frontend to configure NuttX

Skip this section if `kconfig-frontends` had alrady been installed by package manager.

```sh
# Assuming you are in jerry-nuttx folder.
cd tools/kconfig-frontends

./configure \
    --disable-nconf \
    --disable-gconf \
    --disable-qconf \
    --disable-shared \
    --enable-static \
    --prefix=${PWD}/install

make
make install
# Add install folder to PATH
PATH=${PWD}/install/bin:$PATH
```

#### 5. Configure NuttX

Configure NuttX for serial communication. A `.config` file contains all the options for the build.

```sh
# Assuming you are in jerry-nuttx folder.
cd nuttx/tools

# Configure NuttX to use USB console shell.
./configure.sh stm32f4discovery:usbnsh
```

By default, JerryScript is disabled so it is needed to modify the configuration file. It can
be done by using menuconfig (section 5.1) or modifying the `.config` file directly (section 5.2).

##### 5.1 Enable JerryScript using menuconfig

```sh
# Assuming you are in jerry-nuttx folder.
# Might be required to run `make menuconfig` twice.
make -C nuttx menuconfig
```

* Enable `System Type -> FPU support`
* Enable `Application Configuration -> Interpreters -> JerryScript`

[Optional] Enabling ROMFS helps to flash JavaScript input files to the device's flash memory.

* Enable `File systems -> ROMFS file system`
* Enable `Application configuration -> NSH library -> Scripting Support -> Support ROMFS start-up script`

[Optional] Enabling MMCSD helps to mount sd card on the device.
Note: These options are for the micro-sd card slot of STM32F4-Discovery base-board.

* Enable `System Type -> STM32 Peripheral Support -> SDIO`
* Enable `RTOS Features -> Work queue support -> High priority (kernel) worker thread`
* Enable `RTOS Features -> RTOS hooks -> Custom board late initialization`
* Enable `RTOS Features -> RTOS hooks -> Enable on_exit() API`
* Enable `Driver Support -> MMC/SD Driver Support`
* Disable `Driver Support -> MMC/SD Driver Support -> MMC/SD write protect pin`
* Disable `Driver Support -> MMC/SD Driver Support -> MMC/SD SPI transfer support`
* Enable `Driver Support -> MMC/SD Driver Support -> MMC/SD SDIO transfer support`
* Enable `File systems -> FAT file system`
* Enable `File systems -> FAT file system -> FAT upper/lower names`
* Enable `File systems -> FAT file system -> FAT long file names`

##### 5.2 Enable JerryScript without user interaction

A simpler solution is to append the necessary content to the `.config` configuration file:

```sh
# Assuming you are in jerry-nuttx folder.
cat >> nuttx/.config << EOL
CONFIG_ARCH_FPU=y
CONFIG_SCHED_ONEXIT=y
CONFIG_INTERPRETERS_JERRYSCRIPT=y
CONFIG_INTERPRETERS_JERRYSCRIPT_PRIORITY=100
CONFIG_INTERPRETERS_JERRYSCRIPT_STACKSIZE=16384
EOL
```

[Optional] Enable ROM File System

```sh
# Assuming you are in jerry-nuttx folder.
cat >> nuttx/.config << EOL
CONFIG_BOARDCTL_ROMDISK=y
CONFIG_FS_ROMFS=y
CONFIG_NSH_ROMFSETC=y
CONFIG_NSH_ROMFSMOUNTPT="/etc"
CONFIG_NSH_INITSCRIPT="init.d/rcS"
CONFIG_NSH_ROMFSDEVNO=0
CONFIG_NSH_ROMFSSECTSIZE=64
CONFIG_NSH_DEFAULTROMFS=y
EOL
```

[Optional] Enable MMCSD driver and FAT File System

Note: These options are for the micro-sd card slot of STM32F4-Discovery base-board.

```sh
# Assuming you are in jerry-nuttx folder.
cat >> nuttx/.config << EOL
CONFIG_STM32_SDIO=y
CONFIG_STM32_SDIO_DMAPRIO=0x00010000
CONFIG_MMCSD_HAVE_CARDDETECT=y
CONFIG_BOARD_LATE_INITIALIZE=y
CONFIG_BOARD_INITTHREAD_STACKSIZE=2048
CONFIG_BOARD_INITTHREAD_PRIORITY=240
CONFIG_SIG_SIGWORK=17
CONFIG_SCHED_WORKQUEUE=y
CONFIG_SCHED_HPWORK=y
CONFIG_SCHED_HPNTHREADS=1
CONFIG_SCHED_HPWORKPRIORITY=224
CONFIG_SCHED_HPWORKSTACKSIZE=2048
CONFIG_BCH=y
CONFIG_ARCH_HAVE_SDIO=y
CONFIG_ARCH_HAVE_SDIOWAIT_WRCOMPLETE=y
CONFIG_ARCH_HAVE_SDIO_PREFLIGHT=y
CONFIG_MMCSD=y
CONFIG_MMCSD_NSLOTS=1
CONFIG_MMCSD_MMCSUPPORT=y
CONFIG_MMCSD_SDIO=y
CONFIG_FS_FAT=y
CONFIG_FAT_LCNAMES=y
CONFIG_FAT_LFN=y
CONFIG_FAT_MAXFNAME=32
CONFIG_FAT_LFN_ALIAS_TRAILCHARS=0
CONFIG_FSUTILS_MKFATFS=y
CONFIG_NSH_MMCSDSLOTNO=0
EOL
```

#### 6. Provide JavaScript files for STM32F4 device

##### 6.1. Create ROMFS image from a custom folder

Skip this section if MMCSD is used. Otherwise, generate a C header file from a custom folder.
Try to minimize the size of the folder due to the limited capacity of flash memory.

```sh
# Assuming you are in jerry-nuttx folder.
mkdir jerry-example
# Let hello.js be a possible JavaScript input for JerryScript.
cp jerryscript/tests/hello.js jerry-example

# Generate ROMFS image from a custom folder.
genromfs -f romfs_img -d jerry-example

# Dump image as C header file and override NuttX's default ROMFS file.
xxd -i romfs_img apps/nshlib/nsh_romfsimg.h

# Add const modifier to place the content to flash memory.
sed -i "s/unsigned/const\ unsigned/g" apps/nshlib/nsh_romfsimg.h
```

##### 6.2. Copy files to memory card

Skip this section if ROMFS is used. Otherwise, copy your files to a FAT32 formatted memory card.

#### 7. Build NuttX (with JerryScript)

```sh
# Assuming you are in jerry-nuttx folder.
make -C nuttx
```

#### 8. Flash the device

Connect Mini-USB for charging and flasing the device.

```sh
# Assuming you are in jerry-nuttx folder.
make -C stlink release

sudo stlink/build/Release/st-flash write nuttx/nuttx.bin 0x8000000
```

#### 9. Connect to the device

Connect Micro-USB for serial communication. The device should be visible as `/dev/ttyACM0` on the host.
You can use `minicom` communication program with `115200` baud rate.

```sh
sudo minicom --device=/dev/ttyACM0 --baud=115200
```

Set `Add Carriage Ret` option in `minicom` by `CTRL-A -> Z -> U` key combinations.
You may have to press `RESET` on the board and press `Enter` key on the console several times to make `nsh` prompt visible.
NuttShell (NSH) prompt looks like as follows:

```
NuttShell (NSH) NuttX-10.2.0
nsh>
```

#### 10. Run JerryScript

##### 10.1 Run `jerry` without input

```
NuttShell (NSH) NuttX-10.2.0
nsh> jerry
No input files, running a hello world demo:
Hello world 5 times from JerryScript
```

##### 10.2 Run `jerry` with files from ROMFS

```
NuttShell (NSH) NuttX-10.2.0
nsh> jerry /etc/hello.js
```

##### 10.3 Run `jerry` with files from memory card

After NuttX has initialized, the card reader should be visible as `/dev/mmcsd0` on the device.
Mount it to be JavaScript files reachable.

```
NuttShell (NSH) NuttX-10.2.0
nsh> mount -t vfat /dev/mmcsd0 /mnt
nsh> jerry /mnt/hello.js
```
