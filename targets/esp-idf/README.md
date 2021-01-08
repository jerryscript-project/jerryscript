This is a port for espressif's esp-idf (esp32). The MATH, LTO and STRIP options should be disabled, so to build under the IDF toolchain, just run the following command

```
python tools\build.py --toolchain=cmake/toolchain-esp32.cmake --cmake-param "-GUnix Makefiles" --jerry-cmdline=OFF --jerry-port-default=OFF --lto=OFF --strip=OFF
```

NB: the MATH, STRIP and LTO might be disabled by platform as well. I strongly suggest limiting heap memorry with '--mem-heap=128' but that really depends on the SRAM avaiulable on your esp32.

Then copy the artefacts 'build/lib/\*.a' in an esp-idf component named 'jerryscript' (eg) and use a 'CMakeLists.txt' like this one

```
# assumes there is a component with this the following
# - set the JERRY_DIR wherever the jerryscript source code (the include files) is
# - a "lib" directory with the 2 libraries below

set(JERRY_DIR ${PROJECT_DIR}/../../jerryscript/)

idf_component_register(
	SRC_DIRS ${JERRY_DIR}/targets/esp-idf
	INCLUDE_DIRS ${JERRY_DIR}/jerry-core/include ${JERRY_DIR}/jerry-ext/include
)

add_prebuilt_library(libjerry-core lib/libjerry-core.a REQUIRES newlib PRIV_REQUIRES ${COMPONENT_NAME})
add_prebuilt_library(libjerry-ext  lib/libjerry-ext.a PRIV_REQUIRES ${COMPONENT_NAME})

target_link_libraries(${COMPONENT_LIB} INTERFACE libjerry-core)
target_link_libraries(${COMPONENT_LIB} INTERFACE libjerry-ext)
```
