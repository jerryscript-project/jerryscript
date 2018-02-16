#include "esp_gpio_js.h"

DELCARE_HANDLER (gpio_pin_mode)
{
  if (args_cnt != 2)
  {
    return raise_argument_count_error (GPIO_OBJECT_NAME, GPIO_PIN_MODE, 2);
  }

  if (!jerry_value_is_number (args_p[0]))
  {
    return raise_argument_type_error (1, TYPE_NUMBER);
  }

  if (!jerry_value_is_number (args_p[1]))
  {
    return raise_argument_type_error (2, TYPE_NUMBER);
  }

  int port = (int) jerry_get_number_value (args_p[0]);
  int value = (int) jerry_get_number_value (args_p[1]);

  gpio_enable (port, value == 0 ? 0 : 1);

  return jerry_create_undefined ();
} /* gpio_pin_mode */

DELCARE_HANDLER (gpio_write)
{
  if (args_cnt != 2)
  {
    return raise_argument_count_error (GPIO_OBJECT_NAME, GPIO_WRITE, 2);
  }

  if (!jerry_value_is_number (args_p[0]))
  {
    return raise_argument_type_error (1, TYPE_NUMBER);
  }

  if (!jerry_value_is_number (args_p[1]))
  {
    return raise_argument_type_error (2, TYPE_NUMBER);
  }

  int port = (int) jerry_get_number_value (args_p[0]);
  int value = (int) jerry_get_number_value (args_p[1]);

  gpio_write (port, value);

  return jerry_create_undefined ();
} /* gpio_write */

DELCARE_HANDLER (gpio_read)
{
  if (args_cnt != 1)
  {
    return raise_argument_count_error (GPIO_OBJECT_NAME, GPIO_READ, 1);
  }

  if (!jerry_value_is_number (args_p[0]))
  {
    return raise_argument_type_error (1, TYPE_NUMBER);
  }

  int port = (int) jerry_get_number_value (args_p[0]);
  int value = gpio_read (port);

  return jerry_create_number (value);
} /* gpio_read */

void register_gpio_object (jerry_value_t global_object)
{
  jerry_value_t gpio_object = jerry_create_object ();
  register_js_value_to_object (GPIO_OBJECT_NAME, gpio_object, global_object);

  register_native_function (GPIO_PIN_MODE, gpio_pin_mode_handler, gpio_object);
  register_native_function (GPIO_READ, gpio_read_handler, gpio_object);
  register_native_function (GPIO_WRITE, gpio_write_handler, gpio_object);

  register_number_to_object (GPIO_HIGH, GPIO_HIGH_OR_OUTPUT, gpio_object);
  register_number_to_object (GPIO_LOW, GPIO_LOW_OR_INPUT, gpio_object);
  register_number_to_object (GPIO_OUTPUT, GPIO_HIGH_OR_OUTPUT, gpio_object);
  register_number_to_object (GPIO_INPUT, GPIO_LOW_OR_INPUT, gpio_object);

  jerry_release_value (gpio_object);
} /* register_gpio_object */
