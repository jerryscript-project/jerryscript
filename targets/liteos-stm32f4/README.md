### About

This folder contains files to run JerryScript on LiteOS with STM32F4-Discovery board.

### How to build

#### 1. Preface

1, Directory structure

Assume `jerry-liteos` as the path to the projects to build.
The folder tree would look like this.

```
jerry-liteos
  + jerryscript
  |  + targets
  |      + liteos-stm32f4
  + LiteOS_Kernel
```

2, Target board

Assume [STM32F4-Discovery with BB](http://www.st.com/web/en/catalog/tools/FM116/SC959/SS1532/LN1199/PF255417)
as the target board.

#### 2. Prepare LiteOS

Follow [this](https://github.com/LITEOS/LiteOS_Kernel) page to get the LiteOS source.



#### 3. Build JerryScript for LiteOS

```
# Assume you are in folder `jerry-liteos/jerryscript`.Then run:
make -f ./targets/liteos-stm32f4/Makefile.liteos
```
This step will generate the library `jerry-liteos/jerryscript/build/liteosstm32f4/lib/libjerry-core.a`
and make a copy of it as `jerry-liteos/jerryscript/targets/liteos-stm32f4/bin/libjerrycore.a`.
Finally, it will also create a bin file containing LiteOS, JerryScript, and the test application
at `jerry-liteos/jerryscript/LiteOS_STM32F4.bin`.


#### 4. Cleaning

To clean the build result:
```
make -f ./targets/liteos-stm32f4/Makefile.liteos clean
```


### 5. Running JerryScript LED blink example

You may have to press `RESET` on the board after the flash.
