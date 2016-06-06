#!/bin/bash

# Copyright © 2016 Intel Corporation
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

# clone zephyr repo
git clone https://gerrit.zephyrproject.org/r/zephyr
cd zephyr
git checkout tags/v1.4.0

# download the zephyr_sdk installer script
wget https://nexus.zephyrproject.org/content/repositories/releases/org/\
zephyrproject/zephyr-sdk/0.8-i686/zephyr-sdk-0.8-i686-setup.run

# extract the zephyr-sdk installer
chmod +x zephyr-sdk-0.8-i686-setup.run
mkdir zephyr_sdk_installer && cd zephyr_sdk_installer
../zephyr-sdk-0.8-i686-setup.run --target $(pwd) --noexec

# install the zephyr_sdk
mkdir ../zephyr_sdk && cd ../zephyr_sdk
export ZEPHYR_SDK_INSTALL_DIR=$(pwd)
cd ../zephyr_sdk_installer
./setup.sh -d $ZEPHYR_SDK_INSTALL_DIR

# source the zephyr required environment variables
source ../zephyr-env.sh
export ZEPHYR_GCC_VARIANT=zephyr

# back to jerryscript dir
cd ../..

# link asm-generic to asm (otherwise there will be errors with include headers)
sudo ln -s /usr/include/asm-generic /usr/include/asm

# build jerryscript for qemu
make -f ./targets/arduino_101/Makefile.arduino_101 BOARD=qemu_x86

# create 2 named pipes to input jerryscript commands and evaluate the output
mkfifo path.in path.out

# run the qemu executable in background using the named pipes
./zephyr/zephyr_sdk/sysroots/i686-pokysdk-linux/usr/bin/qemu-system-i386 -m 32 \
    -cpu qemu32 -no-reboot -nographic -vga none -display none -net none \
    -clock dynticks -no-acpi -balloon none \
    -L ./zephyr/zephyr_sdk/sysroots/i686-pokysdk-linux/usr/share/qemu \
    -bios bios.bin -machine type=pc-0.14 -serial pipe:path \
    -kernel ./build/qemu_x86/zephyr/zephyr.elf & sleep 5

# listen for any output and direct it to log for further checking
cat path.out > log.txt & sleep 5

# write the jerryscript "test" command in the input named pipe
printf "test\r\n" > path.in
sleep 5

# kill qemu executable process (which is marked as first job that runs in background)
kill %1

# show the log output and check if the string "Hi JS World!" is present there
cat log.txt
if grep "Hi JS World!" log.txt > /dev/null
then
    echo -e "\n\nQapla'!    (it means \"Success\" in Klingon)"
else
    echo -e "\n\nScript has failed"
fi

# run the grep command again, so that in the case of not finding the string, the
# grep command will return a non 0 value, which will make travis fail the build
grep "Hi JS World!" log.txt > /dev/null
