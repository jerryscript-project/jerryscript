### About

Files in this folder (embedding/esp8266) are copied from
`examples/project_template` of `esp_iot_rtos_sdk` and modified for JerryScript.
You can view online from
[this](https://github.com/espressif/esp_iot_rtos_sdk/tree/master/examples/project_template) page.


### How to build JerryScript for ESP8266

#### 1. SDK

Follow [this](./docs/ESP-PREREQUISITES.md) page to setup build environment


#### 2. Patch ESP-SDK for JerryScript

Follow [this](./docs/ESP-PATCHFORJERRYSCRIPT.md) page to patch for JerryScript building.
Below is a summary after SDK patch is applied.

#### 3. Building JerryScript

```
cd ~/harmony/jerryscript
# clean build
make -f ./targets/esp8266/Makefile.esp8266 clean
# or just normal build
make -f ./targets/esp8266/Makefile.esp8266
```

Output files should be placed at $BIN_PATH

#### 4. Flashing for ESP8266 ESP-01 board (WiFi Module)

Steps are for `ESP8266 ESP-01(WiFi)` board. Others may vary.
Refer http://www.esp8266.com/wiki/doku.php?id=esp8266-module-family page.

##### 4.1 GPIO0 and GPIO2

Before flashing you need to follow the steps.

1. Power off ESP8266
2. Connect GPIO0 to GND and GPIO2 to VCC
3. Power on ESP8266
4. Flash

##### 4.2 Flashing

```
make -f ./targets/esp8266/Makefile.esp8266 flash
```

Default USB device is `/dev/ttyUSB0`. If you have different one, give with `USBDEVICE`, like;

```
USBDEVICE=/dev/ttyUSB1 make -f ./targets/esp8266/Makefile.esp8266 flash
```


### 5. Running

1. Power off
2. Disonnect(float) both GPIO0 and GPIO2
3. Power on

Sample program here works with LED and a SW with below connection.
* Connect GPIO2 to a LED > 4K resistor > GND
* Connect GPIO0 between VCC > 4K resistor and GND

If GPIO0 is High then LED is turned on longer. If L vice versa.

### 6. Optimizing initial RAM usage (ESP8266 specific)
The existing open source gcc compiler with Xtensa support stores const(ants) in
the same limited RAM where our code needs to run. 

It is possible to force the compiler to 1)store a constant into ROM and also 2) read it from there thus saving 1.1) RAM.
It will require two things though:
1. To add the attribute JERRY_CONST_DATA to your constant. For example

```C
static const lit_magic_size_t lit_magic_string_sizes[] =
```

can be modified to

```C
static const lit_magic_size_t lit_magic_string_sizes[] JERRY_CONST_DATA =
```

That is already done to some constants in jerry-core. 

1.1) Below is a short list:


Bytes  | Name
-------- | ---------
928 | magic_strings$2428
610 | vm_decode_table
424 | unicode_letter_interv_sps
235 | cbc_flags
232 | lit_magic_string_sizes
212 | unicode_letter_interv_len
196 | unicode_non_letter_ident_
112 | unicode_letter_chars

Which frees 2949 bytes in RAM. 

2. To compile your code with compiler that supports the `-mforce-l32` parameter. You can check if your compiler is 
supporting that parameter by calling:
```bash
xtensa-lx106-elf-gcc --help=target | grep mforce-l32
```
If the command above does not provide a result then you will need to upgrade your compiler.