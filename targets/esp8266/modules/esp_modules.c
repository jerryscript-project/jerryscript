/**
 * All the implemented modules must be included here
 */
#include "esp_gpio_js.h"

void register_modules (jerry_value_t global_object)
{
  register_gpio_object (global_object);
} /* register_modules */
