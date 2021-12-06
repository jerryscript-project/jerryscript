/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "application.h"
#include "jerryscript.h"

SYSTEM_MODE (MANUAL);

/**
 * Set led value
 */
static jerry_value_t
set_led  (const jerry_value_t func_value, /**< function object */
          const jerry_value_t this_value, /**< this arg */
          const jerry_value_t *args_p, /**< function arguments */
          const jerry_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt != 2)
  {
    Serial.println ("Wrong arguments count in 'test.setLed' function.");
    return jerry_boolean (false);
  }

  int ledPin = jerry_value_as_number (args_p[0]);
  bool value = jerry_value_is_true (args_p[1]);

  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, value);

  return jerry_boolean (true);
} /* set_led */

/**
 * Delay function
 */
static jerry_value_t
js_delay (const jerry_value_t func_value, /**< function object */
          const jerry_value_t this_value, /**< this arg */
          const jerry_value_t *args_p, /**< function arguments */
          const jerry_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt != 1)
  {
    Serial.println ("Wrong arguments count in 'test.delay' function.");
    return jerry_boolean (false);
  }

  int millisec = jerry_value_as_number (args_p[0]);

  delay (millisec);

  return jerry_boolean (true);
} /* js_delay */

/*
 * Init available js functions
 */
static void
init_jerry ()
{
  jerry_init (JERRY_INIT_EMPTY);

  /* Create an empty JS object */
  jerry_value_t object = jerry_object ();

  jerry_value_t func_obj;
  jerry_value_t prop_name;

  func_obj = jerry_function_external (set_led);
  prop_name = jerry_string_sz ("setLed");
  jerry_value_free (jerry_object_set (object, prop_name, func_obj));
  jerry_value_free (prop_name);
  jerry_value_free (func_obj);

  func_obj = jerry_function_external (js_delay);
  prop_name = jerry_string_sz ("delay");
  jerry_value_free (jerry_object_set (object, prop_name, func_obj));
  jerry_value_free (prop_name);
  jerry_value_free (func_obj);

  /* Wrap the JS object (not empty anymore) into a jerry api value */
  jerry_value_t global_object = jerry_current_realm ();

  /* Add the JS object to the global context */
  prop_name = jerry_string_sz ("test");
  jerry_value_free (jerry_object_set (global_object, prop_name, object));
  jerry_value_free (prop_name);
  jerry_value_free (object);
  jerry_value_free (global_object);
} /* init_jerry */

/**
 * Jerryscript simple test
 */
static void
test_jerry ()
{
  const jerry_char_t script[] = " \
    test.setLed(7, true); \
    test.delay(250); \
    test.setLed(7, false); \
    test.delay(250); \
  ";

  jerry_value_t eval_ret = jerry_eval (script, sizeof (script) - 1, JERRY_PARSE_NO_OPTS);

  /* Free JavaScript value, returned by eval */
  jerry_value_free (eval_ret);
} /* test_jerry */

/**
 * Setup code for particle firmware
 */
void
setup ()
{
  Serial.begin (9600);
  delay (2000);
  Serial.println ("Beginning Listening mode test!");
} /* setup */

/**
 * Loop code for particle firmware
 */
void
loop ()
{
  init_jerry ();

  /* Turn on and off the D7 LED */
  test_jerry ();

  jerry_cleanup ();
} /* loop */
