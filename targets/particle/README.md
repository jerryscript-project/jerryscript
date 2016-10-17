### About

This folder contains files to run JerryScript beside Particle Device Firmware on Photon board.
It  runs a mini example, blinking an LED which is the "Hello World" example of the microcontroller universe.

### How to build

#### 1. Preface / Directory structure

Assume `root` as the path to the projects to build.
The folder tree related would look like this.

```
root
  + jerryscript
  |  + targets
  |      + particle
  + particle
  |  + firmware
```


#### 2, Update/Install the Particle Firmware

For detailed descriptions please visit the official website of the firmware: https://www.particle.io/

You can checkout the firmware with the following command:

```
# assume you are in root folder
git clone https://github.com/spark/firmware.git particle/firmware particle/firmware
````

The Photon’s latest firmware release is hosted in the latest branch of the firmware repo.

```
# assume you are in root folder
cd particle/firmware
git checkout latest
```

Before you type any commands, put your Photon in DFU mode: hold down both the SETUP and RESET buttons. Then release RESET, but hold SETUP until you see the RGB blink yellow. That means the Photon is in DFU mode. To verify that the Photon is in DFU mode and dfu-util is installed properly, try the dfu-util -l command. You should see the device, and the important parts there are the hex values inside the braces – the USB VID and PID, which should be 2b04 and d006 for the Photon.

To build and flash the firmware: switch to the modules directory then call make with a few parameters to build and upload the firmware:

```
cd modules
make PLATFORM=photon clean all program-dfu
```

#### 3. Build JerryScript

```
# assume you are in root folder
cd jerryscript
make -f ./targets/particle/Makefile.particle
```

This will create a binary file in the `/build/particle/` folder:
```
jerry_main.bin
```

That’s the binary what we’ll be flashing with dfu-util.


#### 4. Flashing

Make sure you put your Photon in DFU mode.
Alternatively, you can make your life a bit easier by using the make command to invoke dfu-util:

```
make -f targets/particle/Makefile.particle flash
```

You can also use this dfu-util command directly to upload your BIN file to the Photon’s application memory:

```
dfu-util -d 2b04:d006 -a 0 -i 0 -s 0x80A0000:leave -D build/particle/jerry_main.bin
```

#### 5. Cleaning

To clean the build result:
```
make -f targets/particle/Makefile.particle clean
```

### Running the example

The example program will blinks the D7 led periodically after the flash.