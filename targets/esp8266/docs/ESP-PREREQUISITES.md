#### Preparation

##### Accessories

You need,
* 3.3V power supply. You can use bread board power (+5V, +3.3V). I used LM317 like this;
  * Use [LM317](http://www.ti.com/lit/ds/symlink/lm317.pdf)
  * R1 = 330 Ohm, R2 = 545 Ohm (1K + 1.2K in parallel)
  * 5V 2A adaptor
* USB to RS-232 Serial + RS-232 Serial to Digital or USB-to-RS232 TTL converter

#### Toolchain

Reference [Toolchain](https://github.com/esp8266/esp8266-wiki/wiki/Toolchain) page.

I've slightly changed the step to use SDK from Espressif official SDK
(https://github.com/espressif/esp_iot_rtos_sdk)

##### Toolchain:

dependencies
```
sudo apt-get install git autoconf build-essential gperf \
    bison flex texinfo libtool libtool-bin libncurses5-dev wget \
    gawk python-serial libexpat-dev
sudo mkdir /opt/Espressif
sudo chown $USER /opt/Espressif/

```

dependency specific to x86:
```
sudo apt-get install libc6-dev-i386
```

dependency specific to x64:
```
sudo apt-get install libc6-dev-amd64
```

crosstool-NG
```
cd /opt/Espressif
git clone -b lx106-g++-1.21.0 git://github.com/jcmvbkbc/crosstool-NG.git
cd crosstool-NG
./bootstrap && ./configure --prefix=`pwd` && make && make install
./ct-ng xtensa-lx106-elf
./ct-ng build
```
add path to environment file such as `.profile`
```
PATH=$PWD/builds/xtensa-lx106-elf/bin:$PATH
```

##### Espressif SDK: use Espressif official

```
cd /opt/Esprissif
git clone https://github.com/espressif/ESP8266_RTOS_SDK.git ESP8266_RTOS_SDK.git
ln -s ESP8266_RTOS_SDK.git ESP8266_SDK
git checkout -b jerry 2fab9e23d779cdd6e5900b8ba2b588e30d9b08c4
```

This verison is tested and works properly.

set two environment variables such as in .profile
```
export SDK_PATH=/opt/Espressif/ESP8266_SDK
export BIN_PATH=(to output folder path)
```

##### Xtensa libraries and headers:
```
cd /opt/Espressif/ESP8266_SDK
wget -O lib/libhal.a https://github.com/esp8266/esp8266-wiki/raw/master/libs/libhal.a
```

##### ESP image tool
```
cd /opt/Espressif
wget -O esptool_0.0.2-1_i386.deb https://github.com/esp8266/esp8266-wiki/raw/master/deb/esptool_0.0.2-1_i386.deb
sudo dpkg -i esptool_0.0.2-1_i386.deb
```

##### ESP upload tool
```
cd /opt/Espressif
git clone https://github.com/themadinventor/esptool esptool-py
sudo ln -s $PWD/esptool-py/esptool.py crosstool-NG/builds/xtensa-lx106-elf/bin/esptool.py
```

#### Test writing with Blinky example

##### Get the source

found one example that works with SDK V1.2 (which is based on FreeRTOS, as of writing)

* https://github.com/mattcallow/esp8266-sdk/tree/master/rtos_apps/01blinky


##### Compile

Read `2A-ESP8266__IOT_SDK_User_Manual_EN` document in
[this](http://bbs.espressif.com/viewtopic.php?f=51&t=1024) link.

It's configured 2048KB flash
```
BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=5 make
BOOT=new APP=2 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=5 make
```

or old way... this works not sure this is ok.
```
make BOOT=new APP=0 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=2
```

##### Flashing

* power off ESP8266 board
* connect GPIO0 to GND, connect GPIO2 to VCC
* power on
* write

```
sudo /opt/Espressif/esptool-py/esptool.py \
  --port /dev/ttyUSB0 write_flash \
  0x00000 $SDK_PATH/bin/"boot_v1.7.bin" \
  0x01000 $BIN_PATH/upgrade/user1.2048.new.5.bin \
  0x101000 $BIN_PATH/upgrade/user2.2048.new.5.bin \
  0x3FE000 $SDK_PATH/bin/blank.bin \
  0x3FC000 $SDK_PATH/bin/esp_init_data_default.bin
```
_change `/dev/ttyUSB1` to whatever you have._

or the old way...this works not sure this is ok.
```
cd $BIN_PATH
sudo /opt/Espressif/esptool-py/esptool.py \
--port /dev/ttyUSB0 write_flash \
0x00000 eagle.flash.bin 0x40000 eagle.irom0text.bin
```

* power off
* disconnect GPIO0 so that it is floating
* connect GPIO2 with serial of 470 Ohm + LED and to GND
* power On
