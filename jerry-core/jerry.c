/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include <stdio.h>

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-eval.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-init-finalize.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"
#include "jerry-snapshot.h"
#include "lit-literal.h"
#include "lit-magic-strings.h"
#include "lit-snapshot.h"
#include "js-parser.h"
#include "re-compiler.h"

#define JERRY_INTERNAL
#include "jerry-internal.h"

/**
 * Jerry engine build date
 */
const char * const jerry_build_date = JERRY_BUILD_DATE;

/**
 * Jerry engine build commit hash
 */
const char * const jerry_commit_hash = JERRY_COMMIT_HASH;

/**
 * Jerry engine build branch name
 */
const char * const jerry_branch_name = JERRY_BRANCH_NAME;

/**
 * Jerry run-time configuration flags
 */
static jerry_flag_t jerry_flags;

/**
 * Jerry API availability flag
 */
static bool jerry_api_available;

/** \addtogroup jerry_extension Jerry engine extension interface
 * @{
 */

#ifdef JERRY_ENABLE_LOG
/* TODO: Move logging-related functionality to separate module, like jerry-log.c */

/**
 * Verbosity level of logging
 */
int jerry_debug_level = 0;

/**
 * File, used for logging
 */
FILE *jerry_log_file = NULL;
#endif /* JERRY_ENABLE_LOG */

/**
 * Assert that it is correct to call API in current state.
 *
 * Note:
 *         By convention, there can be some states when API could not be invoked.
 *
 *         While, API can be invoked jerry_api_available flag is set,
 *         and while it is incorrect to invoke API - it is not set.
 *
 *         The procedure checks that it is correct to invoke API in current state.
 *         If it is correct, procedure just returns; otherwise - engine is stopped.
 *
 * Note:
 *         API could not be invoked in the following cases:
 *           - between enter to and return from native free callback.
 */
static void
jerry_assert_api_available (void)
{
  if (!jerry_api_available)
  {
    JERRY_UNREACHABLE ();
  }
} /* jerry_assert_api_available */

/**
 * Turn on API availability
 */
static void
jerry_make_api_available (void)
{
  jerry_api_available = true;
} /* jerry_make_api_available */

/**
 * Turn off API availability
 */
static void
jerry_make_api_unavailable (void)
{
  jerry_api_available = false;
} /* jerry_make_api_unavailable */

/**
 * Returns whether the given jerry_value_t is error.
 */
bool
jerry_value_is_error (const jerry_value_t value) /**< api value */
{
  return ECMA_IS_VALUE_ERROR (value);
} /* jerry_value_is_error */

/**
 * Returns whether the given jerry_value_t is null.
 */
bool
jerry_value_is_null (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_null (value);
} /* jerry_value_is_null */

/**
 * Returns whether the given jerry_value_t is undefined.
 */
bool
jerry_value_is_undefined (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_undefined (value);
} /* jerry_value_is_undefined */

/**
 * Returns whether the given jerry_value_t has boolean type.
 */
bool
jerry_value_is_boolean (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_boolean (value);
} /* jerry_value_is_boolean */

/**
 * Returns whether the given jerry_value_t is number.
 *
 * More specifically, returns true if the type is JERRY_DATA_TYPE_FLOAT32,
 * JERRY_DATA_TYPE_FLOAT64 or JERRY_DATA_TYPE_UINT32, false otherwise.
 */
bool
jerry_value_is_number (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_number (value);
} /* jerry_value_is_number */

/**
 * Returns whether the given jerry_value_t is string.
 */
bool
jerry_value_is_string (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_string (value);
} /* jerry_value_is_string */

/**
 * Returns whether the given jerry_value_t is object.
 */
bool
jerry_value_is_object (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_object (value);
} /* jerry_value_is_object */

/**
 * Returns whether the given jerry_value_t is a function object.
 */
bool
jerry_value_is_function (const jerry_value_t value) /**< api value */
{
  return ecma_op_is_callable (value);
} /* jerry_value_is_function */

/**
 * Returns the boolean v_bool member of the given jerry_value_t structure.
 * If the given jerry_value_t structure has type other than
 * JERRY_DATA_TYPE_BOOLEAN, JERRY_ASSERT fails.
 */
bool
jerry_get_boolean_value (const jerry_value_t value) /**< api value */
{
  JERRY_ASSERT (jerry_value_is_boolean (value));

  return ecma_is_value_true (value);
} /* jerry_get_boolean_value */

/**
 * Returns the number value of the given jerry_value_t structure as a double.
 */
double
jerry_get_number_value (const jerry_value_t value) /**< api value */
{
  JERRY_ASSERT (jerry_value_is_number (value));

  return (double) ecma_get_number_from_value (value);
} /* jerry_get_number_value */

/**
 * Returns the v_string member of the given jerry_value_t structure.
 * If the given jerry_value_t structure has type other than
 * JERRY_DATA_TYPE_STRING, JERRY_ASSERT fails.
 */
jerry_string_t *
jerry_get_string_value (const jerry_value_t value) /**< api value */
{
  JERRY_ASSERT (jerry_value_is_string (value));

  return ecma_get_string_from_value (value);
} /* jerry_get_string_value */

/**
 * Returns the v_object member of the given jerry_value_t structure.
 * If the given jerry_value_t structure has type other than
 * JERRY_DATA_TYPE_OBJECT, JERRY_ASSERT fails.
 */
jerry_object_t *
jerry_get_object_value (const jerry_value_t value) /**< api value */
{
  JERRY_ASSERT (jerry_value_is_object (value));

  return ecma_get_object_from_value (value);
} /* jerry_get_object_value */

/**
 * Creates and returns a jerry_value_t with type
 * JERRY_DATA_TYPE_NULL.
 */
jerry_value_t
jerry_create_null_value (void)
{
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
} /* jerry_create_null_value */

/**
 * Creates and returns a jerry_value_t with type
 * JERRY_DATA_TYPE_UNDEFINED.
 */
jerry_value_t
jerry_create_undefined_value (void)
{
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* jerry_create_undefined_value */

/**
 * Creates a JERRY_DATA_TYPE_BOOLEAN jerry_value_t from the given
 * boolean parameter and returns with it.
 */
jerry_value_t
jerry_create_boolean_value (bool value) /**< bool value from which a jerry_value_t will be created */
{
  return (value ? ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE)
                : ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));
} /* jerry_create_boolean_value */

/**
 * Creates a jerry_value_t from the given double parameter and returns
 * with it.
 * The v_float64 member will be set and the will be JERRY_DATA_TYPE_FLOAT64.
 */
jerry_value_t
jerry_create_number_value (double value) /**< double value from which a jerry_value_t will be created */
{
  return ecma_make_number_value ((ecma_number_t) value);
} /* jerry_create_number_value */

/**
 * Creates a JERRY_DATA_TYPE_OBJECT type jerry_value_t from the
 * given jerry_object_t *parameter and returns with it.
 */
jerry_value_t
jerry_create_object_value (jerry_object_t *obj_p) /**< jerry_object_t from which a value will be created */
{
  return ecma_make_object_value (obj_p);
} /* jerry_create_object_value */

/**
 * Creates a JERRY_DATA_TYPE_STRING type jerry_value_t from the
 * given jerry_string_t *parameter and returns with it.
 */
jerry_value_t
jerry_create_string_value (jerry_string_t *str_p) /**< jerry_string_t from which a value will be created */
{
  return ecma_make_string_value (str_p);
} /* jerry_create_string_value */

/**
 * Convert completion of 'eval' to API value and completion code
 *
 * Note:
 *      The value returned in 'retval_p' should be freed with jerry_release_value
 *
 * @return completion code
 */
static inline jerry_completion_code_t __attr_always_inline___
jerry_convert_eval_completion_to_retval (ecma_value_t completion, /**< completion of 'eval'-mode
                                                                   *   code execution */
                                         jerry_value_t *retval_p) /**< [out] api value */
{
  *retval_p = completion;

  return (ECMA_IS_VALUE_ERROR (completion)) ? JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION : JERRY_COMPLETION_CODE_OK;
} /* jerry_convert_eval_completion_to_retval */

/**
 * @}
 */

/**
 * Copy string characters to specified buffer. It is the caller's responsibility
 * to make sure that the string fits in the buffer.
 *
 * Note:
 *   '\0' could occur in characters.
 *
 * @return number of bytes, actually copied to the buffer.
 */
jerry_size_t
jerry_string_to_char_buffer (const jerry_string_t *string_p, /**< string descriptor */
                             jerry_char_t *buffer_p, /**< [out] output characters buffer */
                             jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_available ();

  return ecma_string_copy_to_utf8_buffer (string_p, (lit_utf8_byte_t *) buffer_p, buffer_size);
} /* jerry_string_to_char_buffer */

/**
 * Acquire string pointer for usage outside of the engine
 * from string retrieved in extension routine call from engine.
 *
 * Warning:
 *         acquired pointer should be released with jerry_release_string
 *
 * @return pointer that may be used outside of the engine
 */
jerry_string_t *
jerry_acquire_string (jerry_string_t *string_p) /**< pointer passed to function */
{
  jerry_assert_api_available ();

  ecma_ref_ecma_string (string_p);

  return string_p;
} /* jerry_acquire_string */

/**
 * Release string pointer
 *
 * See also:
 *          jerry_acquire_string
 *          jerry_call_function
 *
 */
void
jerry_release_string (jerry_string_t *string_p) /**< pointer acquired through jerry_acquire_string */
{
  jerry_assert_api_available ();

  ecma_deref_ecma_string (string_p);
} /* jerry_release_string */

/**
 * Acquire object pointer for usage outside of the engine
 * from object retrieved in extension routine call from engine.
 *
 * Warning:
 *         acquired pointer should be released with jerry_release_object
 *
 * @return pointer that may be used outside of the engine
 */
jerry_object_t *
jerry_acquire_object (jerry_object_t *object_p) /**< pointer passed to function */
{
  jerry_assert_api_available ();

  ecma_ref_object (object_p);

  return object_p;
} /* jerry_acquire_object */

/**
 * Release object pointer
 *
 * See also:
 *          jerry_acquire_object
 *          jerry_call_function
 *          jerry_get_object_field_value
 */
void
jerry_release_object (jerry_object_t *object_p) /**< pointer acquired through jerry_acquire_object */
{
  jerry_assert_api_available ();

  ecma_deref_object (object_p);
} /* jerry_release_object */

/**
 * Acquire specified Jerry API value.
 *
 * Note:
 *      For values of string and object types this acquires the underlying data,
 *      for all other types it is a no-op.
 *
 * Warning:
 *         Acquired pointer should be released with jerry_release_value
 */
jerry_value_t
jerry_acquire_value (jerry_value_t value) /**< API value */
{
  jerry_assert_api_available ();

  if (jerry_value_is_string (value))
  {
    jerry_string_t *str_p = jerry_acquire_string (jerry_get_string_value (value));
    return jerry_create_string_value (str_p);
  }
  else if (jerry_value_is_object (value))
  {
    jerry_acquire_object (jerry_get_object_value (value));
  }

  return value;
} /* jerry_acquire_value */

/**
 * Release specified Jerry API value
 *
 * Note:
 *      For values of string and object types this releases the underlying data,
 *      for all other types it is a no-op.
 */
void
jerry_release_value (jerry_value_t value) /**< API value */
{
  jerry_assert_api_available ();

  ecma_free_value (value);
} /* jerry_release_value */

/**
 * Create a string
 *
 * Note:
 *      caller should release the string with jerry_release_string, just when the value becomes unnecessary.
 *
 * @return pointer to created string
 */
jerry_string_t *
jerry_create_string (const jerry_char_t *v) /**< string value */
{
  jerry_assert_api_available ();

  return ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) v, lit_zt_utf8_string_size ((lit_utf8_byte_t *) v));
} /* jerry_create_string */

/**
 * Create a string
 *
 * Note:
 *      caller should release the string with jerry_release_string, just when the value becomes unnecessary.
 *
 * @return pointer to created string
 */
jerry_string_t *
jerry_create_string_sz (const jerry_char_t *v, /**< string value */
                        jerry_size_t v_size) /**< string size */
{
  jerry_assert_api_available ();

  return ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) v,
                                         (lit_utf8_size_t) v_size);
} /* jerry_create_string_sz */

/**
 * Create an object
 *
 * Note:
 *      caller should release the object with jerry_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created object
 */
jerry_object_t *
jerry_create_object (void)
{
  jerry_assert_api_available ();

  return ecma_op_create_object_object_noarg ();
} /* jerry_create_object */

/**
 * Create an array object
 *
 * Note:
 *      caller should release the object with jerry_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created array object
 */
jerry_object_t *
jerry_create_array_object (jerry_size_t size) /**< size of array */
{
  JERRY_ASSERT (size > 0);

  ecma_value_t array_length = ecma_make_uint32_value (size);

  jerry_length_t argument_size = 1;
  ecma_value_t new_array_completion = ecma_op_create_array_object (&array_length, argument_size, true);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array_completion));
  jerry_object_t *obj_p = ecma_get_object_from_value (new_array_completion);

  ecma_free_value (array_length);
  return obj_p;
} /* jerry_create_array_object */

/**
 * Set value of field in the specified array object
 *
 * @return true, if field value was set successfully
 *         throw exception, otherwise
 */
bool
jerry_set_array_index_value (jerry_object_t *array_obj_p, /**< array object */
                             jerry_length_t index, /**< index to be written */
                             jerry_value_t value) /**< value to set */
{
  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_value_t set_completion = ecma_op_object_put (array_obj_p, str_idx_p, value, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (set_completion));

  ecma_free_value (set_completion);
  ecma_deref_ecma_string (str_idx_p);

  return true;
} /* jerry_set_array_index_value */

/**
 * Get value of field in the specified array object
 *
 * Note:
 *      if value was retrieved successfully, it should be freed
 *      with jerry_release_value just when it becomes unnecessary.
 *
 * @return true, if field value was retrieved successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         throw exception - otherwise.
 */
bool
jerry_get_array_index_value (jerry_object_t *array_obj_p, /**< array object */
                             jerry_length_t index, /**< index to be written */
                             jerry_value_t *value_p) /**< [out] value at index */
{
  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_value_t get_completion = ecma_op_object_get (array_obj_p, str_idx_p);
  ecma_deref_ecma_string (str_idx_p);

  if (!ECMA_IS_VALUE_ERROR (get_completion))
  {
    *value_p = get_completion;
    return true;
  }

  *value_p = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  return false;
} /* jerry_get_array_index_value */

/**
 * Create an error object
 *
 * Note:
 *      caller should release the object with jerry_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created error object
 */
jerry_object_t *
jerry_create_error (jerry_error_t error_type, /**< type of error */
                    const jerry_char_t *message_p) /**< value of 'message' property
                                                    *   of constructed error object */
{
  return jerry_create_error_sz (error_type,
                                (lit_utf8_byte_t *) message_p,
                                lit_zt_utf8_string_size (message_p));
} /* jerry_create_error */

/**
 * Create an error object
 *
 * Note:
 *      caller should release the object with jerry_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created error object
 */
jerry_object_t *
jerry_create_error_sz (jerry_error_t error_type, /**< type of error */
                       const jerry_char_t *message_p, /**< value of 'message' property
                                                       *   of constructed error object */
                       jerry_size_t message_size) /**< size of the message in bytes */
{
  jerry_assert_api_available ();

  ecma_standard_error_t standard_error_type = ECMA_ERROR_COMMON;

  switch (error_type)
  {
    case JERRY_ERROR_COMMON:
    {
      standard_error_type = ECMA_ERROR_COMMON;
      break;
    }
    case JERRY_ERROR_EVAL:
    {
      standard_error_type = ECMA_ERROR_EVAL;
      break;
    }
    case JERRY_ERROR_RANGE:
    {
      standard_error_type = ECMA_ERROR_RANGE;
      break;
    }
    case JERRY_ERROR_REFERENCE:
    {
      standard_error_type = ECMA_ERROR_REFERENCE;
      break;
    }
    case JERRY_ERROR_SYNTAX:
    {
      standard_error_type = ECMA_ERROR_SYNTAX;
      break;
    }
    case JERRY_ERROR_TYPE:
    {
      standard_error_type = ECMA_ERROR_TYPE;
      break;
    }
    case JERRY_ERROR_URI:
    {
      standard_error_type = ECMA_ERROR_URI;
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  if (message_p == NULL)
  {
    return ecma_new_standard_error (standard_error_type);
  }
  else
  {
    ecma_string_t *message_string_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) message_p,
                                                                      (lit_utf8_size_t) message_size);

    ecma_object_t *error_object_p = ecma_new_standard_error_with_message (standard_error_type, message_string_p);

    ecma_deref_ecma_string (message_string_p);

    return error_object_p;
  }
} /* jerry_create_error_sz */

/**
 * Create an external function object
 *
 * Note:
 *      caller should release the object with jerry_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created external function object
 */
jerry_object_t *
jerry_create_external_function (jerry_external_handler_t handler_p) /**< pointer to native handler
                                                                     *   for the function */
{
  jerry_assert_api_available ();

  return ecma_op_create_external_function_object ((ecma_external_pointer_t) handler_p);
} /* jerry_create_external_function */

/**
 * Dispatch call to specified external function using the native handler
 *
 * Note:
 *      if called native handler returns true, then dispatcher just returns value received
 *      through 'return value' output argument, otherwise - throws the value as an exception.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
jerry_dispatch_external_function (ecma_object_t *function_object_p, /**< external function object */
                                  ecma_external_pointer_t handler_p, /**< pointer to the function's native handler */
                                  ecma_value_t this_arg_value, /**< 'this' argument */
                                  const ecma_value_t *arguments_list_p, /**< arguments list */
                                  ecma_length_t arguments_list_len) /**< arguments list length */
{
  jerry_assert_api_available ();

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  bool is_successful = ((jerry_external_handler_t) handler_p) (function_object_p,
                                                               this_arg_value,
                                                               arguments_list_p,
                                                               arguments_list_len,
                                                               &ret_value);

  if (!is_successful)
  {
    ret_value = ecma_make_error_value (ret_value);
  }

  return ret_value;
} /* jerry_dispatch_external_function */

/**
 * Dispatch call to object's native free callback function
 *
 * Note:
 *       the callback is called during critical GC phase,
 *       so, should not perform any requests to engine
 */
void
jerry_dispatch_object_free_callback (ecma_external_pointer_t freecb_p, /**< pointer to free callback handler */
                                     ecma_external_pointer_t native_p) /**< native handle, associated
                                                                        *   with freed object */
{
  jerry_make_api_unavailable ();

  ((jerry_object_free_callback_t) freecb_p) ((uintptr_t) native_p);

  jerry_make_api_available ();
} /* jerry_dispatch_object_free_callback */

/**
 * Check if the specified object is a function object.
 *
 * @return true - if the specified object is a function object,
 *         false - otherwise.
 */
bool
jerry_is_function (const jerry_object_t *object_p) /**< an object */
{
  jerry_assert_api_available ();

  JERRY_ASSERT (object_p != NULL);

  ecma_value_t obj_val = ecma_make_object_value (object_p);

  return ecma_op_is_callable (obj_val);
} /* jerry_is_function */

/**
 * Check if the specified object is a constructor function object.
 *
 * @return true - if the specified object is a function object that implements [[Construct]],
 *         false - otherwise.
 */
bool
jerry_is_constructor (const jerry_object_t *object_p) /**< an object */
{
  jerry_assert_api_available ();

  JERRY_ASSERT (object_p != NULL);

  ecma_value_t obj_val = ecma_make_object_value (object_p);

  return ecma_is_constructor (obj_val);
} /* jerry_is_constructor */

/**
 * Create field (named data property) in the specified object
 *
 * @return true, if field was created successfully, i.e. upon the call:
 *                - there is no field with same name in the object;
 *                - the object is extensible;
 *         false - otherwise.
 */
bool
jerry_add_object_field (jerry_object_t *object_p, /**< object to add field at */
                        const jerry_char_t *field_name_p, /**< name of the field */
                        jerry_size_t field_name_size, /**< size of field name in bytes */
                        const jerry_value_t field_value, /**< value of the field */
                        bool is_writable) /**< flag indicating whether the created field should be writable */
{
  jerry_assert_api_available ();

  bool is_successful = false;

  if (ecma_get_object_extensible (object_p))
  {
    ecma_string_t *field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                      (lit_utf8_size_t) field_name_size);

    ecma_property_t *prop_p = ecma_op_object_get_own_property (object_p, field_name_str_p);

    if (prop_p == NULL)
    {
      is_successful = true;

      uint8_t prop_attributes = ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE;

      if (is_writable)
      {
        prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_WRITABLE);
      }

      prop_p = ecma_create_named_data_property (object_p,
                                                field_name_str_p,
                                                prop_attributes);
      ecma_named_data_property_assign_value (object_p, prop_p, field_value);
    }

    ecma_deref_ecma_string (field_name_str_p);
  }

  return is_successful;
} /* jerry_add_object_field */

/**
 * Delete field in the specified object
 *
 * @return true, if field was deleted successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         false - otherwise.
 */
bool
jerry_delete_object_field (jerry_object_t *object_p, /**< object to delete field at */
                           const jerry_char_t *field_name_p, /**< name of the field */
                           jerry_size_t field_name_size) /**< size of the field name in bytes */
{
  jerry_assert_api_available ();

  bool is_successful = true;

  ecma_string_t *field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                    (lit_utf8_size_t) field_name_size);

  ecma_value_t delete_completion = ecma_op_object_delete (object_p,
                                                          field_name_str_p,
                                                          true);

  if (ECMA_IS_VALUE_ERROR (delete_completion))
  {
    is_successful = false;
  }

  ecma_free_value (delete_completion);

  ecma_deref_ecma_string (field_name_str_p);

  return is_successful;
} /* jerry_delete_object_field */

/**
 * Get value of field in the specified object
 *
 * Note:
 *      if value was retrieved successfully, it should be freed
 *      with jerry_release_value just when it becomes unnecessary.
 *
 * @return true, if field value was retrieved successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         false - otherwise.
 */
jerry_value_t
jerry_get_object_field_value (jerry_object_t *object_p, /**< object */
                              const jerry_char_t *field_name_p) /**< field name */
{
  return jerry_get_object_field_value_sz (object_p,
                                          field_name_p,
                                          lit_zt_utf8_string_size (field_name_p));
} /* jerry_get_object_field_value */

/**
 * Applies the given function to the every fields in the objects
 *
 * @return true, if object fields traversal was performed successfully, i.e.:
 *                - no unhandled exceptions were thrown in object fields traversal;
 *                - object fields traversal was stopped on callback that returned false;
 *         false - otherwise,
 *                 if getter of field threw a exception or unhandled exceptions were thrown during traversal;
 */
bool
jerry_foreach_object_field (jerry_object_t *object_p, /**< object */
                            jerry_object_field_foreach_t foreach_p, /**< foreach function */
                            void *user_data_p) /**< user data for foreach function */
{
  jerry_assert_api_available ();

  ecma_collection_iterator_t names_iter;
  ecma_collection_header_t *names_p = ecma_op_object_get_property_names (object_p, false, true, true);
  ecma_collection_iterator_init (&names_iter, names_p);

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  bool continuous = true;

  while (ecma_is_value_empty (ret_value)
         && continuous
         && ecma_collection_iterator_next (&names_iter))
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*names_iter.current_value_p);

    ECMA_TRY_CATCH (property_value, ecma_op_object_get (object_p, property_name_p), ret_value);

    continuous = foreach_p (property_name_p, property_value, user_data_p);

    ECMA_FINALIZE (property_value);
  }

  ecma_free_values_collection (names_p, true);
  if (ecma_is_value_empty (ret_value))
  {
    return true;
  }
  else
  {
    JERRY_ASSERT (ECMA_IS_VALUE_ERROR (ret_value));

    ecma_free_value (ret_value);

    return false;
  }
} /* jerry_foreach_object_field */

/**
 * Get value of field in the specified object
 *
 * Note:
 *      returned value should be freed with jerry_release_value.
 *
 * @return jerry value of the given field
 */
jerry_value_t
jerry_get_object_field_value_sz (jerry_object_t *object_p, /**< object */
                                 const jerry_char_t *field_name_p, /**< name of the field */
                                 jerry_size_t field_name_size) /**< size of field name in bytes */
{
  jerry_assert_api_available ();

  ecma_string_t *field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                    (lit_utf8_size_t) field_name_size);

  ecma_value_t field_value = ecma_op_object_get (object_p, field_name_str_p);

  ecma_deref_ecma_string (field_name_str_p);

  return field_value;
} /* jerry_get_object_field_value_sz */

/**
 * Set value of field in the specified object
 *
 * @return true  - if field value was set successfully
 *         false - otherwise
 */
bool
jerry_set_object_field_value (jerry_object_t *object_p, /**< object */
                              const jerry_char_t *field_name_p, /**< name of the field */
                              const jerry_value_t field_value) /**< field value to set */
{
  return jerry_set_object_field_value_sz (object_p,
                                          field_name_p,
                                          lit_zt_utf8_string_size (field_name_p),
                                          field_value);
} /* jerry_set_object_field_value */

/**
 * Set value of field in the specified object
 *
 * @return true, if field value was set successfully, i.e. upon the call:
 *                - field value is writable;
 *         false - otherwise.
 */
bool
jerry_set_object_field_value_sz (jerry_object_t *object_p, /**< object */
                                 const jerry_char_t *field_name_p, /**< name of the field */
                                 jerry_size_t field_name_size, /**< size of field name in bytes */
                                 const jerry_value_t field_value) /**< field value to set */
{
  jerry_assert_api_available ();

  bool is_successful = true;

  ecma_string_t *field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                    (lit_utf8_size_t) field_name_size);

  ecma_value_t set_completion = ecma_op_object_put (object_p,
                                                    field_name_str_p,
                                                    field_value,
                                                    true);

  if (ECMA_IS_VALUE_ERROR (set_completion))
  {
    is_successful = false;
  }

  ecma_free_value (set_completion);
  ecma_deref_ecma_string (field_name_str_p);

  return is_successful;
} /* jerry_set_object_field_value_sz */

/**
 * Get native handle, associated with specified object
 *
 * @return true - if there is associated handle (handle is returned through out_handle_p),
 *         false - otherwise.
 */
bool
jerry_get_object_native_handle (jerry_object_t *object_p, /**< object to get handle from */
                                uintptr_t *out_handle_p) /**< [out] handle value */
{
  jerry_assert_api_available ();

  uintptr_t handle_value;

  bool does_exist = ecma_get_external_pointer_value (object_p,
                                                     ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE,
                                                     &handle_value);

  if (does_exist)
  {
    *out_handle_p = handle_value;
  }

  return does_exist;
} /* jerry_get_object_native_handle */

/**
 * Set native handle and, optionally, free callback for the specified object
 *
 * Note:
 *      If native handle was already set for the object, its value is updated.
 *
 * Note:
 *      If free callback is specified, it is set to be called upon specified JS-object is freed (by GC).
 *
 *      Otherwise, if NULL is specified for free callback pointer, free callback is not created
 *      and, if a free callback was added earlier for the object, it is removed.
 */
void
jerry_set_object_native_handle (jerry_object_t *object_p, /**< object to set handle in */
                                uintptr_t handle, /**< handle value */
                                jerry_object_free_callback_t freecb_p) /**< object free callback or NULL */
{
  jerry_assert_api_available ();

  ecma_create_external_pointer_property (object_p,
                                         ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE,
                                         handle);
  if (freecb_p != NULL)
  {
    ecma_create_external_pointer_property (object_p,
                                           ECMA_INTERNAL_PROPERTY_FREE_CALLBACK,
                                           (uintptr_t) freecb_p);
  }
  else
  {
    ecma_property_t *prop_p = ecma_find_internal_property (object_p,
                                                           ECMA_INTERNAL_PROPERTY_FREE_CALLBACK);
    if (prop_p != NULL)
    {
      ecma_delete_property (object_p, prop_p);
    }
  }
} /* jerry_set_object_native_handle */

/**
 * Invoke function specified by a function object
 *
 * Note:
 *      returned value must be freed with jerry_release_value
 *
 * Note:
 *      If function is invoked as constructor, it should support [[Construct]] method,
 *      otherwise, if function is simply called - it should support [[Call]] method.
 *
 * @return returned jerry value of the given function object
 */
static jerry_value_t
jerry_invoke_function (bool is_invoke_as_constructor, /**< true - invoke function as constructor
                                                       *          (this_arg_p should be NULL, as it is ignored),
                                                       *   false - perform function call */
                       jerry_object_t *function_object_p, /**< function object to call */
                       jerry_object_t *this_arg_p, /**< object for 'this' binding
                                                    *   or NULL (set 'this' binding to newly constructed object,
                                                    *            if function is invoked as constructor;
                                                    *            in case of simple function call set 'this'
                                                    *            binding to the global object) */
                       const jerry_value_t args_p[], /**< function's call arguments
                                                      *   (NULL if arguments number is zero) */
                       jerry_length_t args_count) /**< number of the arguments */
{
  JERRY_ASSERT (args_count == 0 || args_p != NULL);
  JERRY_STATIC_ASSERT (sizeof (args_count) == sizeof (ecma_length_t),
                       size_of_args_count_must_be_equal_to_size_of_ecma_length_t);

  ecma_value_t call_completion;

  if (is_invoke_as_constructor)
  {
    JERRY_ASSERT (this_arg_p == NULL);
    JERRY_ASSERT (jerry_is_constructor (function_object_p));

    call_completion = ecma_op_function_construct (function_object_p,
                                                  args_p,
                                                  args_count);
  }
  else
  {
    JERRY_ASSERT (jerry_is_function (function_object_p));

    ecma_value_t this_arg_val;

    if (this_arg_p == NULL)
    {
      this_arg_val = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      this_arg_val = ecma_make_object_value (this_arg_p);
    }

    call_completion = ecma_op_function_call (function_object_p,
                                             this_arg_val,
                                             args_p,
                                             args_count);
  }

  return call_completion;
} /* jerry_invoke_function */

/**
 * Construct new TypeError object
 *
 * @return TypeError object value
 */
static jerry_value_t __attr_always_inline___
jerry_construct_type_error (void)
{
  ecma_object_t *type_error_obj_p = ecma_new_standard_error (ECMA_ERROR_TYPE);
  return ecma_make_error_obj_value (type_error_obj_p);
} /* jerry_construct_type_error */

/**
 * Call function specified by a function object
 *
 * Note:
 *      returned value should be freed with jerry_release_value
 *      just when the value becomes unnecessary.
 *
 * @return returned jerry value of the given function object
 */
jerry_value_t
jerry_call_function (jerry_object_t *function_object_p, /**< function object to call */
                     jerry_object_t *this_arg_p, /**< object for 'this' binding
                                                  *   or NULL (set 'this' binding to the global object) */
                     const jerry_value_t args_p[], /**< function's call arguments
                                                    *   (NULL if arguments number is zero) */
                     uint16_t args_count) /**< number of the arguments */
{
  jerry_assert_api_available ();

  if (jerry_is_function (function_object_p))
  {
    return jerry_invoke_function (false, function_object_p, this_arg_p, args_p, args_count);
  }

  return jerry_construct_type_error ();
} /* jerry_call_function */

/**
 * Construct object invoking specified function object as a constructor
 *
 * Note:
 *      returned value should be freed with jerry_release_value
 *      just when the value becomes unnecessary.
 *
 * @return returned jerry value of the given constructor
 */
jerry_value_t
jerry_construct_object (jerry_object_t *function_object_p, /**< function object to call */
                        const jerry_value_t args_p[], /**< function's call arguments
                                                       *   (NULL if arguments number is zero) */
                        uint16_t args_count) /**< number of the arguments */
{
  jerry_assert_api_available ();

  if (jerry_is_constructor (function_object_p))
  {
    return jerry_invoke_function (true, function_object_p, NULL, args_p, args_count);
  }

  return jerry_construct_type_error ();
} /* jerry_construct_object */

/**
 * Get global object
 *
 * Note:
 *       caller should release the object with jerry_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to the global object
 */
jerry_object_t *
jerry_get_global (void)
{
  jerry_assert_api_available ();

  return ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);
} /* jerry_get_global */

/**
 * Perform eval
 *
 * Note:
 *      If current code is executed on top of interpreter, using is_direct argument,
 *      caller can enable direct eval mode that is equivalent to calling eval from
 *      within of current JS execution context.
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_eval (const jerry_char_t *source_p, /**< source code */
            size_t source_size, /**< length of source code */
            bool is_direct, /**< perform eval invocation in direct mode */
            bool is_strict, /**< perform eval as it is called from strict mode code */
            jerry_value_t *retval_p) /**< [out] returned value */
{
  jerry_assert_api_available ();

  jerry_completion_code_t status;

  ecma_value_t completion = ecma_op_eval_chars_buffer ((const lit_utf8_byte_t *) source_p,
                                                       source_size,
                                                       is_direct,
                                                       is_strict);

  status = jerry_convert_eval_completion_to_retval (completion, retval_p);

  return status;
} /* jerry_eval */

/**
 * Perform GC
 */
void
jerry_gc (void)
{
  jerry_assert_api_available ();

  ecma_gc_run ();
} /* jerry_gc */

/**
 * Jerry engine initialization
 */
void
jerry_init (jerry_flag_t flags) /**< combination of Jerry flags */
{
  if (flags & (JERRY_FLAG_ENABLE_LOG))
  {
#ifndef JERRY_ENABLE_LOG
    JERRY_WARNING_MSG ("Ignoring log options because of '!JERRY_ENABLE_LOG' build configuration.\n");
#endif /* !JERRY_ENABLE_LOG */
  }

  if (flags & (JERRY_FLAG_MEM_STATS | JERRY_FLAG_MEM_STATS_SEPARATE))
  {
#ifndef JMEM_STATS
    flags &= (jerry_flag_t) ~(JERRY_FLAG_MEM_STATS | JERRY_FLAG_MEM_STATS_SEPARATE);

    JERRY_WARNING_MSG ("Ignoring memory statistics option because of '!JMEM_STATS' build configuration.\n");
#else /* JMEM_STATS */
    if (flags & JERRY_FLAG_MEM_STATS_SEPARATE)
    {
      flags |= JERRY_FLAG_MEM_STATS;
    }
#endif /* !JMEM_STATS */
  }

  jerry_flags = flags;

  jerry_make_api_available ();

  jmem_init ();
  lit_init ();
  ecma_init ();
} /* jerry_init */

/**
 * Terminate Jerry engine
 */
void
jerry_cleanup (void)
{
  jerry_assert_api_available ();

  bool is_show_mem_stats = ((jerry_flags & JERRY_FLAG_MEM_STATS) != 0);

  vm_finalize ();
  ecma_finalize ();
  lit_finalize ();
  jmem_finalize (is_show_mem_stats);
} /* jerry_cleanup */

/**
 * Get Jerry configured memory limits
 */
void
jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p, /**< [out] Jerry's maximum usage of
                                                            *         data + bss + brk sections */
                         size_t *out_stack_limit_p) /**< [out] Jerry's maximum usage of stack */
{
  *out_data_bss_brk_limit_p = CONFIG_MEM_HEAP_AREA_SIZE + CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE;
  *out_stack_limit_p = CONFIG_MEM_STACK_LIMIT;
} /* jerry_get_memory_limits */

/**
 * Parse script for specified context
 *
 * Note:
 *      returned error object should be freed with jerry_release_object
 *
 * @return true - if script was parsed successfully,
 *         false - otherwise (SyntaxError was raised).
 */
bool
jerry_parse (const jerry_char_t *source_p, /**< script source */
             size_t source_size, /**< script source size */
             jerry_object_t **error_obj_p) /**< [out] error object */
{
  jerry_assert_api_available ();

  vm_finalize ();

  int is_show_instructions = ((jerry_flags & JERRY_FLAG_SHOW_OPCODES) != 0);

  parser_set_show_instrs (is_show_instructions);

  ecma_compiled_code_t *bytecode_data_p;
  jsp_status_t parse_status;

  parse_status = parser_parse_script (source_p,
                                      source_size,
                                      &bytecode_data_p,
                                      error_obj_p);

  if (parse_status != JSP_STATUS_OK)
  {
    JERRY_ASSERT (parse_status == JSP_STATUS_SYNTAX_ERROR);

    return false;
  }

#ifdef JMEM_STATS
  if (jerry_flags & JERRY_FLAG_MEM_STATS_SEPARATE)
  {
    jmem_stats_print ();
    jmem_stats_reset_peak ();
  }
#endif /* JMEM_STATS */

  vm_init (bytecode_data_p);

  return true;
} /* jerry_parse */

/**
 * Run Jerry in specified run context
 *
 * Note:
 *      returned error value should be freed with jerry_release_value
 *      just when the value becomes unnecessary.
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_run (jerry_value_t *error_value_p) /**< [out] error value */
{
  jerry_assert_api_available ();

  jerry_completion_code_t ret_code = vm_run_global (error_value_p);

  return ret_code;
} /* jerry_run */

/**
 * Simple jerry runner
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_run_simple (const jerry_char_t *script_source, /**< script source */
                  size_t script_source_size, /**< script source size */
                  jerry_flag_t flags) /**< combination of Jerry flags */
{
  jerry_init (flags);

  jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;
  jerry_object_t *error_obj_p = NULL;

  if (!jerry_parse (script_source, script_source_size, &error_obj_p))
  {
    /* unhandled SyntaxError */
    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }
  else if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
  {
    jerry_value_t error_value;

    ret_code = jerry_run (&error_value);

    jerry_release_value (error_value);
  }

  jerry_cleanup ();

  return ret_code;
} /* jerry_run_simple */

/**
 * Register external magic string array
 */
void
jerry_register_external_magic_strings (const jerry_char_ptr_t *ex_str_items, /**< character arrays, representing
                                                                              *   external magic strings' contents */
                                       uint32_t count,                       /**< number of the strings */
                                       const jerry_length_t *str_lengths)    /**< lengths of the strings */
{
  lit_magic_strings_ex_set ((const lit_utf8_byte_t **) ex_str_items, count, (const lit_utf8_size_t *) str_lengths);
} /* jerry_register_external_magic_strings */

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE

/*
 * Variables required to take a snapshot.
 */
static bool snapshot_error_occured;
static size_t snapshot_buffer_write_offset;
static uint8_t *snapshot_buffer_p;
static size_t snapshot_buffer_size;

/**
 * Snapshot callback for byte codes.
 *
 * @return start offset
 */
static uint16_t
snapshot_add_compiled_code (ecma_compiled_code_t *compiled_code_p) /**< compiled code */
{
  if (snapshot_error_occured)
  {
    return 0;
  }

  JERRY_ASSERT ((snapshot_buffer_write_offset & (JMEM_ALIGNMENT - 1)) == 0);

  if ((snapshot_buffer_write_offset >> JMEM_ALIGNMENT_LOG) > 0xffffu)
  {
    snapshot_error_occured = true;
    return 0;
  }

  uint16_t start_offset = (uint16_t) (snapshot_buffer_write_offset >> JMEM_ALIGNMENT_LOG);
  ecma_compiled_code_t *copied_compiled_code_p;

  copied_compiled_code_p = (ecma_compiled_code_t *) (snapshot_buffer_p + snapshot_buffer_write_offset);

  if (!(compiled_code_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
    /* Regular expression. */
    if (snapshot_buffer_write_offset + sizeof (ecma_compiled_code_t) > snapshot_buffer_size)
    {
      snapshot_error_occured = true;
      return 0;
    }

    snapshot_buffer_write_offset += sizeof (ecma_compiled_code_t);

    jmem_cpointer_t pattern_cp = ((re_compiled_code_t *) compiled_code_p)->pattern_cp;
    ecma_string_t *pattern_string_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                 pattern_cp);

    ecma_length_t pattern_size = 0;

    ECMA_STRING_TO_UTF8_STRING (pattern_string_p, buffer_p, buffer_size);

    pattern_size = buffer_size;

    if (!jrt_write_to_buffer_by_offset (snapshot_buffer_p,
                                        snapshot_buffer_size,
                                        &snapshot_buffer_write_offset,
                                        buffer_p,
                                        buffer_size))
    {
      snapshot_error_occured = true;
    }

    ECMA_FINALIZE_UTF8_STRING (buffer_p, buffer_size);

    snapshot_buffer_write_offset = JERRY_ALIGNUP (snapshot_buffer_write_offset, JMEM_ALIGNMENT);

    /* Regexp character size is stored in refs. */
    copied_compiled_code_p->refs = (uint16_t) pattern_size;

    pattern_size += (ecma_length_t) sizeof (ecma_compiled_code_t);
    copied_compiled_code_p->size = (uint16_t) ((pattern_size + JMEM_ALIGNMENT - 1) >> JMEM_ALIGNMENT_LOG);

    copied_compiled_code_p->status_flags = compiled_code_p->status_flags;

#else /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
    JERRY_UNIMPLEMENTED ("RegExp is not supported in compact profile.");
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
    return start_offset;
  }

  if (!jrt_write_to_buffer_by_offset (snapshot_buffer_p,
                                      snapshot_buffer_size,
                                      &snapshot_buffer_write_offset,
                                      compiled_code_p,
                                      ((size_t) compiled_code_p->size) << JMEM_ALIGNMENT_LOG))
  {
    snapshot_error_occured = true;
    return 0;
  }

  /* Sub-functions and regular expressions are stored recursively. */
  uint8_t *src_buffer_p = (uint8_t *) compiled_code_p;
  uint8_t *dst_buffer_p = (uint8_t *) copied_compiled_code_p;
  lit_cpointer_t *src_literal_start_p;
  uint16_t *dst_literal_start_p;
  uint32_t const_literal_end;
  uint32_t literal_end;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    src_literal_start_p = (lit_cpointer_t *) (src_buffer_p + sizeof (cbc_uint16_arguments_t));
    dst_literal_start_p = (uint16_t *) (dst_buffer_p + sizeof (cbc_uint16_arguments_t));

    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) src_buffer_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
  }
  else
  {
    src_literal_start_p = (lit_cpointer_t *) (src_buffer_p + sizeof (cbc_uint8_arguments_t));
    dst_literal_start_p = (uint16_t *) (dst_buffer_p + sizeof (cbc_uint8_arguments_t));

    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) src_buffer_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    ecma_compiled_code_t *bytecode_p = ECMA_GET_NON_NULL_POINTER (ecma_compiled_code_t,
                                                                  src_literal_start_p[i]);

    if (bytecode_p == compiled_code_p)
    {
      dst_literal_start_p[i] = start_offset;
    }
    else
    {
      dst_literal_start_p[i] = snapshot_add_compiled_code (bytecode_p);
    }
  }

  return start_offset;
} /* snapshot_add_compiled_code */

/**
 * Set the uint16_t offsets in the code area.
 */
static void
jerry_snapshot_set_offsets (uint8_t *buffer_p, /**< buffer */
                            uint32_t size, /**< buffer size */
                            lit_mem_to_snapshot_id_map_entry_t *lit_map_p) /**< literal map */
{
  JERRY_ASSERT (size > 0);

  do
  {
    ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) buffer_p;
    uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

    if (bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION)
    {
      lit_cpointer_t *literal_start_p;
      uint32_t argument_end;
      uint32_t register_end;
      uint32_t const_literal_end;

      if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
      {
        literal_start_p = (lit_cpointer_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

        cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
        argument_end = args_p->argument_end;
        register_end = args_p->register_end;
        const_literal_end = args_p->const_literal_end;
      }
      else
      {
        literal_start_p = (lit_cpointer_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

        cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
        argument_end = args_p->argument_end;
        register_end = args_p->register_end;
        const_literal_end = args_p->const_literal_end;
      }

      uint32_t register_clear_start = 0;

      if ((bytecode_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED)
          && !(bytecode_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE))
      {
        for (uint32_t i = 0; i < argument_end; i++)
        {
          lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

          if (literal_start_p[i] != JMEM_CP_NULL)
          {
            while (current_p->literal_id != literal_start_p[i])
            {
              current_p++;
            }

            literal_start_p[i] = (uint16_t) current_p->literal_offset;
          }
        }

        register_clear_start = argument_end;
      }

      for (uint32_t i = register_clear_start; i < register_end; i++)
      {
        literal_start_p[i] = JMEM_CP_NULL;
      }

      for (uint32_t i = register_end; i < const_literal_end; i++)
      {
        lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

        if (literal_start_p[i] != JMEM_CP_NULL)
        {
          while (current_p->literal_id != literal_start_p[i])
          {
            current_p++;
          }

          literal_start_p[i] = (uint16_t) current_p->literal_offset;
        }
      }

      /* Set reference counter to 1. */
      bytecode_p->refs = 1;
    }

    buffer_p += code_size;
    size -= code_size;
  }
  while (size > 0);
} /* jerry_snapshot_set_offsets */

#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

/**
 * Generate snapshot from specified source
 *
 * @return size of snapshot, if it was generated succesfully
 *          (i.e. there are no syntax errors in source code, buffer size is sufficient,
 *           and snapshot support is enabled in current configuration through JERRY_ENABLE_SNAPSHOT),
 *         0 - otherwise.
 */
size_t
jerry_parse_and_save_snapshot (const jerry_char_t *source_p, /**< script source */
                               size_t source_size, /**< script source size */
                               bool is_for_global, /**< snapshot would be executed as global (true)
                                                    *   or eval (false) */
                               uint8_t *buffer_p, /**< buffer to save snapshot to */
                               size_t buffer_size) /**< the buffer's size */
{
#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
  jsp_status_t parse_status;
  ecma_compiled_code_t *bytecode_data_p;

  snapshot_buffer_write_offset = JERRY_ALIGNUP (sizeof (jerry_snapshot_header_t),
                                                JMEM_ALIGNMENT);
  snapshot_error_occured = false;
  snapshot_buffer_p = buffer_p;
  snapshot_buffer_size = buffer_size;
  jerry_object_t *error_obj_p = NULL;

  if (is_for_global)
  {
    parse_status = parser_parse_script (source_p,
                                        source_size,
                                        &bytecode_data_p,
                                        &error_obj_p);
  }
  else
  {
    parse_status = parser_parse_eval (source_p,
                                      source_size,
                                      false,
                                      &bytecode_data_p,
                                      &error_obj_p);
  }

  if (parse_status != JSP_STATUS_OK)
  {
    JERRY_ASSERT (error_obj_p != NULL);
    ecma_deref_object (error_obj_p);
  }

  if (parse_status != JSP_STATUS_OK)
  {
    return 0;
  }

  snapshot_add_compiled_code (bytecode_data_p);

  if (snapshot_error_occured)
  {
    return 0;
  }

  jerry_snapshot_header_t header;
  header.version = JERRY_SNAPSHOT_VERSION;
  header.lit_table_offset = (uint32_t) snapshot_buffer_write_offset;
  header.is_run_global = is_for_global;

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num;

  if (!lit_save_literals_for_snapshot (buffer_p,
                                       buffer_size,
                                       &snapshot_buffer_write_offset,
                                       &lit_map_p,
                                       &literals_num,
                                       &header.lit_table_size))
  {
    JERRY_ASSERT (lit_map_p == NULL);
    return 0;
  }

  jerry_snapshot_set_offsets (buffer_p + JERRY_ALIGNUP (sizeof (jerry_snapshot_header_t), JMEM_ALIGNMENT),
                              (uint32_t) (header.lit_table_offset - sizeof (jerry_snapshot_header_t)),
                              lit_map_p);

  size_t header_offset = 0;

  jrt_write_to_buffer_by_offset (buffer_p,
                                 buffer_size,
                                 &header_offset,
                                 &header,
                                 sizeof (header));

  if (lit_map_p != NULL)
  {
    jmem_heap_free_block_size_stored (lit_map_p);
  }

  ecma_bytecode_deref (bytecode_data_p);

  return snapshot_buffer_write_offset;
#else /* !JERRY_ENABLE_SNAPSHOT_SAVE */
  (void) source_p;
  (void) source_size;
  (void) is_for_global;
  (void) buffer_p;
  (void) buffer_size;

  return 0;
#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */
} /* jerry_parse_and_save_snapshot */

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC

/**
 * Byte code blocks shorter than this treshold are always copied into the memory.
 * The memory / performance trade-of of byte code redirection does not worth
 * in such cases.
 */
#define BYTECODE_NO_COPY_TRESHOLD 8

/**
 * Load byte code from snapshot.
 *
 * @return byte code
 */
static ecma_compiled_code_t *
snapshot_load_compiled_code (const uint8_t *snapshot_data_p, /**< snapshot data */
                             size_t offset, /**< byte code offset */
                             lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< literal map */
                             bool copy_bytecode) /**< byte code should be copied to memory */
{
  ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) (snapshot_data_p + offset);
  uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

  if (!(bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
    const re_compiled_code_t *re_bytecode_p = NULL;

    const uint8_t *regex_start_p = ((const uint8_t *) bytecode_p) + sizeof (ecma_compiled_code_t);

    /* Real size is stored in refs. */
    ecma_string_t *pattern_str_p = ecma_new_ecma_string_from_utf8 (regex_start_p,
                                                                   bytecode_p->refs);

    re_compile_bytecode (&re_bytecode_p,
                         pattern_str_p,
                         bytecode_p->status_flags);

    ecma_deref_ecma_string (pattern_str_p);

    return (ecma_compiled_code_t *) re_bytecode_p;
#else /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
    JERRY_UNIMPLEMENTED ("RegExp is not supported in compact profile.");
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
  }

  size_t header_size;
  uint32_t literal_end;
  uint32_t const_literal_end;

  if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    uint8_t *byte_p = (uint8_t *) bytecode_p;
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) byte_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
    header_size = sizeof (cbc_uint16_arguments_t);
  }
  else
  {
    uint8_t *byte_p = (uint8_t *) bytecode_p;
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) byte_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
    header_size = sizeof (cbc_uint8_arguments_t);
  }

  if (copy_bytecode
      || (header_size + (literal_end * sizeof (uint16_t)) + BYTECODE_NO_COPY_TRESHOLD > code_size))
  {
    bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block (code_size);

    memcpy (bytecode_p, snapshot_data_p + offset, code_size);
  }
  else
  {
    code_size = (uint32_t) (header_size + literal_end * sizeof (lit_cpointer_t));

    uint8_t *real_bytecode_p = ((uint8_t *) bytecode_p) + code_size;
    uint32_t total_size = JERRY_ALIGNUP (code_size + 1 + sizeof (uint8_t *), JMEM_ALIGNMENT);

    bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block (total_size);

    memcpy (bytecode_p, snapshot_data_p + offset, code_size);

    bytecode_p->size = (uint16_t) (total_size >> JMEM_ALIGNMENT_LOG);

    uint8_t *instructions_p = ((uint8_t *) bytecode_p);

    instructions_p[code_size] = CBC_SET_BYTECODE_PTR;
    memcpy (instructions_p + code_size + 1, &real_bytecode_p, sizeof (uint8_t *));
  }

  JERRY_ASSERT (bytecode_p->refs == 1);

  lit_cpointer_t *literal_start_p = (lit_cpointer_t *) (((uint8_t *) bytecode_p) + header_size);

  for (uint32_t i = 0; i < const_literal_end; i++)
  {
    lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

    if (literal_start_p[i] != 0)
    {
      while (current_p->literal_offset != literal_start_p[i])
      {
        current_p++;
      }

      literal_start_p[i] = current_p->literal_id;
    }
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    size_t literal_offset = ((size_t) literal_start_p[i]) << JMEM_ALIGNMENT_LOG;

    if (literal_offset == offset)
    {
      /* Self reference */
      ECMA_SET_NON_NULL_POINTER (literal_start_p[i],
                                 bytecode_p);
    }
    else
    {
      ecma_compiled_code_t *literal_bytecode_p;
      literal_bytecode_p = snapshot_load_compiled_code (snapshot_data_p,
                                                        literal_offset,
                                                        lit_map_p,
                                                        copy_bytecode);

      ECMA_SET_NON_NULL_POINTER (literal_start_p[i],
                                 literal_bytecode_p);
    }
  }

  return bytecode_p;
} /* snapshot_load_compiled_code */

#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */

/**
 * Execute snapshot from specified buffer
 *
 * @return completion code
 */
jerry_completion_code_t
jerry_exec_snapshot (const void *snapshot_p, /**< snapshot */
                     size_t snapshot_size, /**< size of snapshot */
                     bool copy_bytecode, /**< flag, indicating whether the passed snapshot
                                          *   buffer should be copied to the engine's memory.
                                          *   If set the engine should not reference the buffer
                                          *   after the function returns (in this case, the passed
                                          *   buffer could be freed after the call).
                                          *   Otherwise (if the flag is not set) - the buffer could only be
                                          *   freed after the engine stops (i.e. after call to jerry_cleanup). */
                     jerry_value_t *retval_p) /**< [out] returned value (ECMA-262 'undefined' if
                                               *         code is executed as global scope code) */
{
  *retval_p = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
  JERRY_ASSERT (snapshot_p != NULL);

  const uint8_t *snapshot_data_p = (uint8_t *) snapshot_p;

  if (snapshot_size <= sizeof (jerry_snapshot_header_t))
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT;
  }

  const jerry_snapshot_header_t *header_p = (const jerry_snapshot_header_t *) snapshot_data_p;

  if (header_p->version != JERRY_SNAPSHOT_VERSION)
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION;
  }

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num;

  if (header_p->lit_table_offset >= snapshot_size)
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION;
  }

  if (!lit_load_literals_from_snapshot (snapshot_data_p + header_p->lit_table_offset,
                                        header_p->lit_table_size,
                                        &lit_map_p,
                                        &literals_num))
  {
    JERRY_ASSERT (lit_map_p == NULL);
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT;
  }

  ecma_compiled_code_t *bytecode_p;
  bytecode_p = snapshot_load_compiled_code (snapshot_data_p,
                                            sizeof (jerry_snapshot_header_t),
                                            lit_map_p,
                                            copy_bytecode);

  if (lit_map_p != NULL)
  {
    jmem_heap_free_block_size_stored (lit_map_p);
  }

  if (bytecode_p == NULL)
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT;
  }

  jerry_completion_code_t ret_code;

  if (header_p->is_run_global)
  {
    vm_init (bytecode_p);

    ecma_value_t error_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

    ret_code = vm_run_global (&error_value);

    if (ret_code == JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION)
    {
      JERRY_ASSERT (ECMA_IS_VALUE_ERROR (error_value));
      ecma_free_value (error_value);
    }
    else
    {
      JERRY_ASSERT (ret_code == JERRY_COMPLETION_CODE_OK);
      JERRY_ASSERT (ecma_is_value_undefined (error_value));
    }

    vm_finalize ();
  }
  else
  {
    /* vm should be already initialized */
    ecma_value_t completion = vm_run_eval (bytecode_p, false);

    ret_code = jerry_convert_eval_completion_to_retval (completion, retval_p);

    ecma_free_value (completion);
  }

  return ret_code;
#else /* !JERRY_ENABLE_SNAPSHOT_EXEC */
  (void) snapshot_p;
  (void) snapshot_size;
  (void) copy_bytecode;
  (void) retval_p;

  return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION;
#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */
} /* jerry_exec_snapshot */

/**
 * Call ToBoolean operation on the api value.
 *
 * @return true  - if the logical value is true
 *         false - otherwise
 */
bool
jerry_value_to_boolean (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  return ecma_op_to_boolean (value);
} /* jerry_value_to_boolean */

/**
 * Call ToNumber operation on the api value.
 *
 * Note:
 *      - Can throw error, so return value should be checked with 'jerry_value_is_error'.
 *      - Return value must be freed with 'jerry_release_value'.
 *
 * @return number value
 */
jerry_value_t
jerry_value_to_number (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  return ecma_op_to_number (value);
} /* jerry_value_to_number */

/**
 * Call ToObject operation on the api value.
 *
 * Note:
 *      - Can throw error, so return value should be checked with 'jerry_value_is_error'.
 *      - Return value must be freed with 'jerry_release_value'.
 *
 * @return object value
 */
jerry_value_t
jerry_value_to_object (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  return ecma_op_to_object (value);
} /* jerry_value_to_object */

/**
 * Call the ToString ecma builtin operation on the api value.
 *
 * Note:
 *      - Can throw error, so return value should be checked with 'jerry_value_is_error'.
 *      - Return value must be freed with 'jerry_release_value'.
 *
 * @return string value
 */
jerry_value_t
jerry_value_to_string (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  return ecma_op_to_string (value);
} /* jerry_value_to_string */

/**
 * Get size of Jerry string
 *
 * @return number of bytes in the buffer needed to represent the string
 */
jerry_size_t
jerry_get_string_size (const jerry_string_t *str_p) /**< input string */
{
  jerry_assert_api_available ();

  return ecma_string_get_size ((ecma_string_t *) str_p);
} /* jerry_get_string_size */

/**
 * Get length of Jerry string
 *
 * @return number of characters in the string
 */
jerry_length_t
jerry_get_string_length (const jerry_string_t *str_p) /**< input string */
{
  jerry_assert_api_available ();

  return ecma_string_get_length ((ecma_string_t *) str_p);
} /* jerry_get_string_length */
