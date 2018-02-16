### How to build JerryScript for ESP8266

#### 1. SDK

Follow [this](./docs/ESP-PREREQUISITES.md) page to setup build environment


#### 2. Building JerryScript

```
# assume you are in jerryscript folder
make -f ./targets/esp8266/Makefile.esp8266
```

#### 3. Flashing for ESP8266 12E
Follow
[this](http://www.kloppenborg.net/images/blog/esp8266/esp8266-esp12e-specs.pdf) page to get details about this board.

```
make -f ./targets/esp8266/Makefile.esp8266 flash
```

Default USB device is `/dev/ttyUSB0`. If you have different one, give with `USBDEVICE`, like:

```
USBDEVICE=/dev/ttyUSB1 make -f ./targets/esp8266/Makefile.esp8266 flash
```

Jerry-debugger is disabled by default. To enable is use the following command:

```
JERRY_DEBUGGER=ON make -f ./targets/esp8266/Makefile.esp8266 flash
```

### 4. Running

* power off
* connect GPIO2 with serial of 470 Ohm + LED and to GND
* power On

LED should blink on and off every second

#### 5. Cleaning

To clean the build result:

```
make -f ./targets/esp8266/Makefile.esp8266 clean
```

To clean the board's flash memory:
```
make -f ./targets/esp8266/Makefile.esp8266 erase_flash
```


### 6. Standard output

While the program is running the output can be read by the following command:
```
minicom --D /dev/ttyUSB0 --b 115200
```

The output can also be accessible using the SDK's own tool. Not only does this tool show the output but in case of an exception (stack overflow, invalid memory accessing, etc.) dumps the content of the stack and uses `xtensa-lx106-elf-addr2line` to find out which function caused it.

```
python $RTOS_PATH/utils/filteroutput.py -b 115200 -e targets/esp8266/build/USER.out -p /dev/ttyUSB0

```

### 7. Standard output redirection

The `stdout` is available at UART0 by default that is connected to the board `RX`-`TX` pin. This prevents connecting sensors to the board which use UART interface for communication.

To overcome this limitation `GPIO2` pin can be used as a TX pin which is ideal for sending (debug) output. In this case connect a USB-TTL to e.g. `/dev/ttyUSB1` and connect `GPIO2` to the TTL `RX` pin.

The following options can be used for redirection and in both cases `RX`-`TX` can be used as a serial communication interface:

  * In this case GPIO2 cannot be used as a general GPIO pin, but `stdout` can be accessible at `/dev/ttyUSB1`

```
STDOUT_REDIRECT=ON make -f ./targets/esp8266/Makefile.esp8266 flash

```

  * If the output is unrelevant `stdout` can be redirected to void and GPIO2 can be used as a general GPIO pin as well

```
STDOUT_REDIRECT=DISCARD make -f ./targets/esp8266/Makefile.esp8266 flash
```
