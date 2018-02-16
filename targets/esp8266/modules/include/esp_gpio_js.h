#ifndef ESP_GPIO_JS_H
#define ESP_GPIO_JS_H

#include "jerry_extapi.h"
#include <esp/gpio.h>

#define GPIO_OBJECT_NAME "GPIO"
#define GPIO_READ "read"
#define GPIO_WRITE "write"
#define GPIO_PIN_MODE "pinMode"
#define GPIO_HIGH "HIGH"
#define GPIO_LOW "LOW"
#define GPIO_OUTPUT "OUTPUT"
#define GPIO_INPUT "INPUT"

#define GPIO_HIGH_OR_OUTPUT 1
#define GPIO_LOW_OR_INPUT 0

void register_gpio_object (jerry_value_t global_object);

#endif /* ESP_GPIO_JS_H */
