#!/bin/bash

# Copyright 2015-2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

git clone https://gerrit.zephyrproject.org/r/zephyr
cd zephyr
wget https://nexus.zephyrproject.org/content/repositories/releases/org/zephyrproject/zephyr-sdk/0.8-i686/zephyr-sdk-0.8-i686-setup.run
chmod +x zephyr-sdk-0.8-i686-setup.run
mkdir zephyr_uncompressed && cd zephyr_uncompressed
../zephyr-sdk-0.8-i686-setup.run --target $(pwd) --noexec
mkdir ../zephyr_sdk && cd ../zephyr_sdk
export ZEPHYR_SDK_INSTALL_DIR=$(pwd)
cd ../zephyr_uncompressed
./setup.sh -d $ZEPHYR_SDK_INSTALL_DIR
source ../zephyr-env.sh
export ZEPHYR_GCC_VARIANT=zephyr
cd ../..
sudo ln -s /usr/include/asm-generic /usr/include/asm
make -f ./targets/arduino_101/Makefile.arduino_101 BOARD=qemu_x86
mkfifo path.in path.out
./zephyr/zephyr_sdk/sysroots/i686-pokysdk-linux/usr/bin/qemu-system-i386 -m 32 -cpu qemu32 -no-reboot -nographic -vga none -display none -net none -clock dynticks -no-acpi -balloon none -L ./zephyr/zephyr_sdk/sysroots/i686-pokysdk-linux/usr/share/qemu -bios bios.bin -machine type=pc-0.14 -pidfile qemu.pid -serial pipe:path -kernel ./build/qemu_x86/zephyr/zephyr.elf & sleep 5
cat path.out > log.txt & sleep 5
printf "test\r\n" > path.in
sleep 5
kill %1
cat log.txt
if grep "Hi JS World!" log.txt > /dev/null; then echo -e "\n\nQapla'!    (it means \"Success\" in Klingon)"; else echo -e "\n\nScript has failed"; fi
grep "Hi JS World!" log.txt > /dev/null
