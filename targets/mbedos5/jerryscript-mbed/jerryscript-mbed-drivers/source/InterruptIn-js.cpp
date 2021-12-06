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
#include "jerryscript-mbed-event-loop/EventLoop.h"
#include "jerryscript-mbed-library-registry/wrap_tools.h"
#include "jerryscript-mbed-util/logging.h"
#include "mbed.h"

/**
 * InterruptIn#destructor
 *
 * Called if/when the InterruptIn object is GC'ed.
 */
void
NAME_FOR_CLASS_NATIVE_DESTRUCTOR (InterruptIn) (void *void_ptr, jerry_object_native_info_t *info_p)
{
  (void) info_p;
  InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

  native_ptr->rise (0);
  native_ptr->fall (0);
  delete native_ptr;
}

/**
 * Type infomation of the native InterruptIn pointer
 *
 * Set InterruptIn#destructor as the free callback.
 */
static const jerry_object_native_info_t native_obj_type_info = { .free_cb =
                                                                   NAME_FOR_CLASS_NATIVE_DESTRUCTOR (InterruptIn) };

/**
 * InterruptIn#rise (native JavaScript method)
 *
 * Register a rise callback for an InterruptIn
 *
 * @param cb Callback function, or null to detach previously attached callback.
 */
DECLARE_CLASS_FUNCTION (InterruptIn, rise)
{
  CHECK_ARGUMENT_COUNT (InterruptIn, rise, (args_count == 1));

  // Detach the rise callback when InterruptIn::rise(null) is called
  if (jerry_value_is_null (args[0]))
  {
    void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

    if (void_ptr == NULL)
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
    }

    InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

    jerry_value_t property_name = jerry_string_sz ("cb_rise");
    jerry_value_t cb_func = jerry_object_get (call_info_p->this_value, property_name);
    jerry_value_free (property_name);

    // Only drop the callback if it exists
    if (jerry_value_is_function (cb_func))
    {
      // Ensure that the EventLoop frees memory used by the callback.
      mbed::js::EventLoop::getInstance ().dropCallback (cb_func);
    }
    jerry_value_free (cb_func);

    native_ptr->rise (0);

    return jerry_undefined ();
  }

  // Assuming we actually have a callback now...
  CHECK_ARGUMENT_TYPE_ALWAYS (InterruptIn, rise, 0, function);

  void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

  if (void_ptr == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
  }

  InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

  jerry_value_t f = args[0];

  // Pass the function to EventLoop.
  mbed::Callback<void ()> cb = mbed::js::EventLoop::getInstance ().wrapFunction (f);
  native_ptr->rise (cb);

  // Keep track of our callback internally.
  jerry_value_t property_name = jerry_string_sz ("cb_rise");
  jerry_value_free (jerry_object_set (call_info_p->this_value, property_name, f));
  jerry_value_free (property_name);

  return jerry_undefined ();
}

/**
 * InterruptIn#fall (native JavaScript method)
 *
 * Register a fall callback for an InterruptIn
 *
 * @param cb Callback function, or null to detach previously attached callback.
 */
DECLARE_CLASS_FUNCTION (InterruptIn, fall)
{
  CHECK_ARGUMENT_COUNT (InterruptIn, fall, (args_count == 1));

  // Detach the fall callback when InterruptIn::fall(null) is called
  if (jerry_value_is_null (args[0]))
  {
    void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

    if (void_ptr == NULL)
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
    }

    InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

    jerry_value_t property_name = jerry_string_sz ("cb_fall");
    jerry_value_t cb_func = jerry_object_get (call_info_p->this_value, property_name);
    jerry_value_free (property_name);

    // Only drop the callback if it exists
    if (jerry_value_is_function (cb_func))
    {
      // Ensure that the EventLoop frees memory used by the callback.
      mbed::js::EventLoop::getInstance ().dropCallback (cb_func);
    }
    jerry_value_free (cb_func);

    native_ptr->fall (0);

    return jerry_undefined ();
  }

  // Assuming we actually have a callback now...
  CHECK_ARGUMENT_TYPE_ALWAYS (InterruptIn, fall, 0, function);

  void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

  if (void_ptr == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
  }

  InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

  jerry_value_t f = args[0];

  // Pass the function to EventLoop.
  mbed::Callback<void ()> cb = mbed::js::EventLoop::getInstance ().wrapFunction (f);
  native_ptr->fall (cb);

  // Keep track of our callback internally.
  jerry_value_t property_name = jerry_string_sz ("cb_fall");
  jerry_value_free (jerry_object_set (call_info_p->this_value, property_name, f));
  jerry_value_free (property_name);

  return jerry_undefined ();
}

/**
 * InterruptIn#mode (native JavaScript method)
 *
 * Set the mode of the InterruptIn pin.
 *
 * @param mode PullUp, PullDown, PullNone
 */
DECLARE_CLASS_FUNCTION (InterruptIn, mode)
{
  CHECK_ARGUMENT_COUNT (InterruptIn, mode, (args_count == 1));
  CHECK_ARGUMENT_TYPE_ALWAYS (InterruptIn, mode, 0, number);

  void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

  if (void_ptr == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
  }

  InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

  int pull = jerry_value_as_number (args[0]);
  native_ptr->mode ((PinMode) pull);

  return jerry_undefined ();
}

/**
 * InterruptIn#disable_irq (native JavaScript method)
 *
 * Disable IRQ. See InterruptIn.h in mbed-os sources for more details.
 */
DECLARE_CLASS_FUNCTION (InterruptIn, disable_irq)
{
  CHECK_ARGUMENT_COUNT (InterruptIn, disable_irq, (args_count == 0));

  void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

  if (void_ptr == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
  }

  InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

  native_ptr->disable_irq ();
  return jerry_undefined ();
}

/**
 * InterruptIn#enable_irq (native JavaScript method)
 *
 * Enable IRQ. See InterruptIn.h in mbed-os sources for more details.
 */
DECLARE_CLASS_FUNCTION (InterruptIn, enable_irq)
{
  CHECK_ARGUMENT_COUNT (InterruptIn, enable_irq, (args_count == 0));

  void *void_ptr = jerry_object_get_native_ptr (call_info_p->this_value, &native_obj_type_info);

  if (void_ptr == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to get native InterruptIn pointer");
  }

  InterruptIn *native_ptr = static_cast<InterruptIn *> (void_ptr);

  native_ptr->enable_irq ();
  return jerry_undefined ();
}

/**
 * InterruptIn (native JavaScript constructor)
 *
 * @param pin PinName
 *
 * @returns JavaScript object wrapping InterruptIn native object
 */
DECLARE_CLASS_CONSTRUCTOR (InterruptIn)
{
  CHECK_ARGUMENT_COUNT (InterruptIn, __constructor, (args_count == 1));
  CHECK_ARGUMENT_TYPE_ALWAYS (InterruptIn, __constructor, 0, number);
  int pin = jerry_value_as_number (args[0]);

  InterruptIn *native_ptr = new InterruptIn ((PinName) pin);
  jerry_value_t js_object = jerry_object ();

  jerry_object_set_native_ptr (js_object, &native_obj_type_info, native_ptr);

  ATTACH_CLASS_FUNCTION (js_object, InterruptIn, rise);
  ATTACH_CLASS_FUNCTION (js_object, InterruptIn, fall);
  ATTACH_CLASS_FUNCTION (js_object, InterruptIn, mode);
  ATTACH_CLASS_FUNCTION (js_object, InterruptIn, enable_irq);
  ATTACH_CLASS_FUNCTION (js_object, InterruptIn, disable_irq);

  return js_object;
}
