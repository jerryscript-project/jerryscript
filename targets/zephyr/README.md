### About

This folder contains files to integrate JerryScript with Zephyr RTOS
(https://zephyrproject.org/).

### How to build

#### 1. Preface

1. Directory structure

Assume `harmony` as the path to the projects to build.
The folder tree related would look like this.

```
harmony
  + jerryscript
  |  + targets
  |      + zephyr
  + zephyr-project
```


2. Target boards/emulations

Following Zephyr boards are known to work: qemu_x86, qemu_cortex_m3,
frdm_k64f. But generally, any board supported by Zephyr should work,
as long as it has enough Flash and RAM (boards which don't have
enough of those, will simply have link-time errors of ROM/RAM
overflow).


#### 2. Prepare Zephyr

Follow [this](https://www.zephyrproject.org/doc/getting_started/getting_started.html) page to get
the Zephyr source and configure the environment.

If you just start with Zephyr, you may want to follow "Building a Sample
Application" section in the doc above and check that you can flash your
target board.

Remember to source the Zephyr environment as explained in the zephyr documentation:

```
cd zephyr-project
source zephyr-env.sh

export ZEPHYR_GCC_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=<sdk installation directory>
```

#### 3. Build JerryScript for Zephyr QEMU

The easiest way is to build and run on a QEMU emulator:

For x86 architecture:

```
make -f ./targets/zephyr/Makefile.zephyr BOARD=qemu_x86 run
```

For ARM (Cortex-M) architecture:

```
make -f ./targets/zephyr/Makefile.zephyr BOARD=qemu_cortex_m3 run
```

#### 4. Build for a real board

Below, we build for NXP FRDM-K64F board (`frdm_k64f` Zephyr board
identifier). Building for other boards is similar. You are expected
to read [Supported Boards](https://docs.zephyrproject.org/latest/boards/index.html)
section in the Zephyr documentation for more information about
Zephyr's support for a particular board, means to flash binaries,
etc.

```
# assuming you are in top-level folder
cd jerryscript
make -f ./targets/zephyr/Makefile.zephyr BOARD=frdm_k64f
```

At the end of the build, you will be given a path to and stats about
the ELF binary:

```
...
Finished
   text	   data	    bss	    dec	    hex	filename
 117942	    868	  24006	 142816	  22de0	build/frdm_k64f/zephyr/zephyr/zephyr.elf
```

Flashing the binary depends on a particular board used (see links above).
For the FRDM-K64F used as the example, you should copy the *raw* binary
file corresponding to the ELF file above (at
`build/frdm_k64f/zephyr/zephyr/zephyr.bin`) to the USB drive appearing
after connecting the board to a computer:

```
cp build/frdm_k64f/zephyr/zephyr/zephyr.bin /media/pfalcon/DAPLINK/
```

To interact with JerryScript, connect to a board via serial connection
and press Reset button (you first should wait while LEDs on the board
stop blinking). For `frdm_k64f`:

```
picocom -b115200 /dev/ttyACM0
```

You should see something similar to this:
```
JerryScript build: Aug 11 2021 16:03:07
JerryScript API 3.0.0
Zephyr version 2.6.99
js>
```

Run the example javascript command test function
```
js> var test=0; for (t=100; t<1000; t++) test+=t; print ('Hi JS World! '+test);
Hi JS World! 494550
```

Try a more complex function:
```
js> function hello(t) {t=t*10;return t}; print("result"+hello(10.5));
```
