# JerryScript for OpenWrt build guide

This document describes the steps required to compile the JerryScript
for OpenWrt. For target device the TP-Link WR1043ND v1.x router is
used. Please be advised, that if you have a different one minor
modifications to this document could be required.

IMPORTANT!

As the TP-Link WR1043ND is a mips based device and mips is a big-endian
architecture a JerryScipt snapshot which was built on an little-endian
system will not work correctly. Thus it is advised that the
snapshot functionally should be used with caution, that is
DO NOT run snapshots generated on little-endian system(s) on
a big-endian system.

## OpenWrt notes

In 2018 ~January the OpenWrt and LEDE project merged into one
and thus the old OpenWrt parts are now usable only from
an archived repository: https://github.com/openwrt/archive

## OpenWrt toolchain setup

To build the JerryScript for OpenWrt a toolchain is required for
the target router/device. The toolchain setup in this document was
tested on an Ubuntu 16.04.3 LTS Linux.

Steps required for toolchain creation:

### 0. Install OpenWrt build requirements
```sh
$ sudo apt-get install git-core build-essential libssl-dev libncurses5-dev unzip gawk zlib1g-dev subversion mercurial
```

### 1. Clone OpenWrt (Chaos Calmer version)

```sh
$ git clone https://github.com/openwrt/archive openwrt -b chaos_calmer
$ cd openwrt
```

### 2. Run Menuconfig and configure the OpenWrt

```sh
$ make menuconfig
```

Options which should be set:
* Set "Target System" to "Atheros AR7xxx/AR9xxx".
* Set "Target Profile" to "TP-LINK TL-WR1043N/ND".

Save the configuration (as .config) and exit from the menuconfig.

### 3. Configure the environment variables

```sh
$ export BUILDROOT=$(pwd) # where the openwrt root dir is
$ export STAGING_DIR=${BUILDROOT}/staging_dir/ # required by the compiler
$ export PATH=$PATH:${STAGING_DIR}/host/bin:${STAGING_DIR}/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/
```

The name `toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2` is created based on the menuconfig.
This changes depending on the target device!

### 4. Build the OpenWrt

```sh
$ make
```

### 5. Check if the compiler was built

```sh
$ mips-openwrt-linux-gcc --version # running this should print out the version information
```

At this point we have the required compiler for OpenWrt.

## Build JerryScript for OpenWrt

### 0. Check environment

Please check if the `STAGING_DIR` is configured correctly and that the toolchain binary is on the `PATH`.

### 1. Run the build with the OpenWrt toolchain file

```
$ ./tools/build.py --toolchain cmake/toolchain_openwrt_mips.cmake \
                   --lto OFF
```

### 2. Copy the binary

After a successful build the `build/bin/jerry` binary file can be copied to the target device.
On how to copy a binary file to an OpenWrt target device please see the OpenWrt manual(s).
