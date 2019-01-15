#### Preparation

##### Accessories

You need,
* 3.3V power supply. You can use bread board power (+5V, +3.3V). I used LM317 like this;
  * Use [LM317](http://www.ti.com/lit/ds/symlink/lm317.pdf)
  * R1 = 330 Ohm, R2 = 545 Ohm (1K + 1.2K in parallel)
  * 5V 2A adaptor
* USB to RS-232 Serial + RS-232 Serial to Digital or USB-to-RS232 TTL converter

#### Tools

The rest of this document will assume that you download all necessary tools into
a common directory structure. For the sake of simplicity, `$HOME/Espressif` will
be used as the root of this structure. Feel free to deviate from this but then
adapt all the paths accordingly.

```
mkdir $HOME/Espressif
```

##### Toolchain

Download the [toolchain](https://github.com/espressif/ESP8266_RTOS_SDK/tree/v3.0.1#get-toolchain)
pre-built for your development platform to `$HOME/Espressif` and untar it. E.g.,
on Linux/x86-64:

```
cd $HOME/Espressif
tar xvfz xtensa-lx106-elf-linux64-1.22.0-88-gde0bdc1-4.8.5.tar.gz
```

##### Espressif SDK: use Espressif official

```
cd $HOME/Esprissif
git clone https://github.com/espressif/ESP8266_RTOS_SDK.git -b v2.1.0
```

This version is tested and works properly.

##### ESP image tool
```
cd $HOME/Espressif
wget -O esptool_0.0.2-1_i386.deb https://github.com/esp8266/esp8266-wiki/raw/master/deb/esptool_0.0.2-1_i386.deb
sudo dpkg -i esptool_0.0.2-1_i386.deb
```

##### ESP upload tool
```
cd $HOME/Espressif
git clone https://github.com/themadinventor/esptool
```

##### Set environment variables

Set environment variables, might also go in your `.profile`:
```
export PATH=$HOME/Espressif/xtensa-lx106-elf/bin:$PATH
export SDK_PATH=$HOME/Espressif/ESP8266_RTOS_SDK
export ESPTOOL_PATH=$HOME/Espressif/esptool
export BIN_PATH=(to output folder path)
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
sudo $HOME/Espressif/esptool/esptool.py \
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
sudo $HOME/Espressif/esptool/esptool.py \
--port /dev/ttyUSB0 write_flash \
0x00000 eagle.flash.bin 0x40000 eagle.irom0text.bin
```

* power off
* disconnect GPIO0 so that it is floating
* connect GPIO2 with serial of 470 Ohm + LED and to GND
* power On
