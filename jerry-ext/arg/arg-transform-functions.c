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

#include "jerryscript-ext/arg.h"
#include "jerryscript.h"

/**
 * The common function to deal with optional arguments.
 * The core transform function is provided by argument `func`.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_optional (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                               const jerryx_arg_t *c_arg_p, /**< native arg */
                               jerryx_arg_transform_func_t func) /**< the core transform function */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_peek (js_arg_iter_p);

  if (jerry_value_is_undefined (js_arg))
  {
    return js_arg;
  }

  return func (js_arg_iter_p, c_arg_p);
} /* jerryx_arg_transform_optional */

/**
 * Tranform a JS argument to a double. Type coercion is not allowed.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_number_strict (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                    const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jerry_value_is_number (js_arg))
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It is not a number.");
  }

  double *dest = c_arg_p->dest;
  *dest = jerry_get_number_value (js_arg);

  return jerry_create_undefined ();
} /* jerryx_arg_transform_number_strict */

/**
 * Tranform a JS argument to a double. Type coercion is allowed.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_number (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                             const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  jerry_value_t to_number = jerry_value_to_number (js_arg);

  if (jerry_value_has_error_flag (to_number))
  {
    jerry_release_value (to_number);

    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It can not be converted to a number.");
  }

  double *dest = c_arg_p->dest;
  *dest = jerry_get_number_value (to_number);
  jerry_release_value (to_number);

  return jerry_create_undefined ();
} /* jerryx_arg_transform_number */

/**
 * Tranform a JS argument to a boolean. Type coercion is not allowed.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_boolean_strict (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                     const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jerry_value_is_boolean (js_arg))
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It is not a boolean.");
  }

  bool *dest = c_arg_p->dest;
  *dest = jerry_get_boolean_value (js_arg);

  return jerry_create_undefined ();
} /* jerryx_arg_transform_boolean_strict */

/**
 * Tranform a JS argument to a boolean. Type coercion is allowed.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_boolean (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                              const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  bool to_boolean = jerry_value_to_boolean (js_arg);

  bool *dest = c_arg_p->dest;
  *dest = to_boolean;

  return jerry_create_undefined ();
} /* jerryx_arg_transform_boolean */

/**
 * The common routine for string transformer.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
static jerry_value_t
jerryx_arg_string_common_routine (jerry_value_t js_arg, /**< JS arg */
                                  const jerryx_arg_t *c_arg_p) /**< native arg */
{
  jerry_char_t *target_p = (jerry_char_t *) c_arg_p->dest;
  jerry_size_t target_buf_size = (jerry_size_t) c_arg_p->extra_info;
  jerry_size_t size = jerry_string_to_char_buffer (js_arg,
                                                   target_p,
                                                   target_buf_size);

  if (size == 0 || size == target_buf_size)
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "The size of the buffer is not large enough.");
  }

  target_p[size] = '\0';

  return jerry_create_undefined ();
} /* jerryx_arg_string_common_routine */

/**
 * Tranform a JS argument to a char array. Type coercion is not allowed.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_string_strict (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                    const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jerry_value_is_string (js_arg))
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It is not a string.");
  }

  return jerryx_arg_string_common_routine (js_arg, c_arg_p);
} /* jerryx_arg_transform_string_strict */

/**
 * Tranform a JS argument to a char array. Type coercion is allowed.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_string (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                             const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  jerry_value_t to_string = jerry_value_to_string (js_arg);

  if (jerry_value_has_error_flag (to_string))
  {
    jerry_release_value (to_string);

    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It can not be converted to a string.");
  }

  jerry_value_t ret = jerryx_arg_string_common_routine (to_string, c_arg_p);
  jerry_release_value (to_string);

  return ret;
} /* jerryx_arg_transform_string */

/**
 * Check whether the JS argument is jerry function, if so, assign to the native argument.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_function (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                               const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jerry_value_is_function (js_arg))
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It is not a function.");
  }

  jerry_value_t *func_p = c_arg_p->dest;
  *func_p = jerry_acquire_value (js_arg);

  return jerry_create_undefined ();
} /* jerryx_arg_transform_function */

/**
 * Check whether the native pointer has the expected type info.
 * If so, assign it to the native argument.
 *
 * @return jerry undefined: the transformer passes,
 *         jerry error: the transformer fails.
 */
jerry_value_t
jerryx_arg_transform_native_pointer (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                     const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jerry_value_is_object (js_arg))
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It is not a object.");
  }

  const jerry_object_native_info_t *expected_info_p;
  const jerry_object_native_info_t *out_info_p;
  expected_info_p = (const jerry_object_native_info_t *) c_arg_p->extra_info;
  void **ptr_p = (void **) c_arg_p->dest;
  bool is_ok = jerry_get_object_native_pointer (js_arg, ptr_p, &out_info_p);

  if (!is_ok || out_info_p != expected_info_p)
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "The object has no native pointer or type does not match.");
  }

  return jerry_create_undefined ();
} /* jerryx_arg_transform_native_pointer */

/**
 * Define transformer for optional argument.
 */
#define JERRYX_ARG_TRANSFORM_OPTIONAL(type) \
jerry_value_t \
jerryx_arg_transform_ ## type ## _optional (jerryx_arg_js_iterator_t *js_arg_iter_p, \
                                            const jerryx_arg_t *c_arg_p) \
{ \
  return jerryx_arg_transform_optional (js_arg_iter_p, c_arg_p, jerryx_arg_transform_ ## type); \
}

JERRYX_ARG_TRANSFORM_OPTIONAL (number)
JERRYX_ARG_TRANSFORM_OPTIONAL (number_strict)
JERRYX_ARG_TRANSFORM_OPTIONAL (boolean)
JERRYX_ARG_TRANSFORM_OPTIONAL (boolean_strict)
JERRYX_ARG_TRANSFORM_OPTIONAL (string)
JERRYX_ARG_TRANSFORM_OPTIONAL (string_strict)
JERRYX_ARG_TRANSFORM_OPTIONAL (function)
JERRYX_ARG_TRANSFORM_OPTIONAL (native_pointer)

/**
 * Ignore the JS argument.
 *
 * @return jerry undefined
 */
jerry_value_t
jerryx_arg_transform_ignore (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                             const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  (void) js_arg_iter_p; /* unused */
  (void) c_arg_p; /* unused */

  return jerry_create_undefined ();
} /* jerryx_arg_transform_ignore */
