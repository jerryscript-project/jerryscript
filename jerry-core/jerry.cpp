/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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
#include "parser.h"
#include "re-compiler.h"

#define JERRY_INTERNAL
#include "jerry-internal.h"

/**
 * Jerry engine build date
 */
const char *jerry_build_date = JERRY_BUILD_DATE;

/**
 * Jerry engine build commit hash
 */
const char *jerry_commit_hash = JERRY_COMMIT_HASH;

/**
 * Jerry engine build branch name
 */
const char *jerry_branch_name = JERRY_BRANCH_NAME;

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
/**
 * TODO:
 *      Move logging-related functionality to separate module, like jerry-log.cpp
 */

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
 * Returns whether the given jerry_api_value_t is void.
 */
bool
jerry_api_value_is_void (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_VOID;
} /* jerry_api_value_is_void */

/**
 * Returns whether the given jerry_api_value_t is null.
 */
bool
jerry_api_value_is_null (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_NULL;
} /* jerry_api_value_is_null */

/**
 * Returns whether the given jerry_api_value_t is undefined.
 */
bool
jerry_api_value_is_undefined (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_UNDEFINED;
} /* jerry_api_value_is_undefined */

/**
 * Returns whether the given jerry_api_value_t has boolean type.
 */
bool
jerry_api_value_is_boolean (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_BOOLEAN;
} /* jerry_api_value_is_boolean */

/**
 * Returns whether the given jerry_api_value_t is number.
 *
 * More specifically, returns true if the type is JERRY_API_DATA_TYPE_FLOAT32,
 * JERRY_API_DATA_TYPE_FLOAT64 or JERRY_API_DATA_TYPE_UINT32, false otherwise.
 */
bool
jerry_api_value_is_number (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_FLOAT32
  || value_p->type == JERRY_API_DATA_TYPE_FLOAT64
  || value_p->type == JERRY_API_DATA_TYPE_UINT32;
} /* jerry_api_value_is_number */

/**
 * Returns whether the given jerry_api_value_t is string.
 */
bool
jerry_api_value_is_string (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_STRING;
} /* jerry_api_value_is_string */

/**
 * Returns whether the given jerry_api_value_t is object.
 */
bool
jerry_api_value_is_object (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return value_p->type == JERRY_API_DATA_TYPE_OBJECT;
} /* jerry_api_value_is_object */

/**
 * Returns whether the given jerry_api_value_t is a function object.
 *
 * More specifically, returns true if the jerry_api_value_t of the value
 * pointed by value_p has JERRY_API_DATA_TYPE_OBJECT type and
 * jerry_api_is_function() functiron return true for its v_object member,
 * otherwise false.
 */
bool
jerry_api_value_is_function (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  return jerry_api_value_is_object (value_p) && jerry_api_is_function (value_p->v_object);
} /* jerry_api_value_is_function */

/**
 * Returns the boolean v_bool member of the given jerry_api_value_t structure.
 * If the given jerry_api_value_t structure has type other than
 * JERRY_API_DATA_TYPE_BOOLEAN, JERRY_ASSERT fails.
 */
bool
jerry_api_get_boolean_value (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  JERRY_ASSERT (jerry_api_value_is_boolean (value_p));
  return value_p->v_bool;
} /* jerry_api_get_boolean_value */

/**
 * Returns the number value of the given jerry_api_value_t structure
 * as a double.
 *
 * If the given jerry_api_value_t structure has type JERRY_API_DATA_TYPE_UINT32
 * v_uint32 member will be returned as a double value. If the given
 * jerry_api_value_t structure has type JERRY_API_DATA_TYPE_FLOAT32
 * v_float32 member will be returned as a double and if tpye is
 * JERRY_API_DATA_TYPE_FLOAT64 the function returns the v_float64 member.
 * As long as the type is none of the above, JERRY_ASSERT falis.
 */
double
jerry_api_get_number_value (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  JERRY_ASSERT (jerry_api_value_is_number (value_p));
  if (value_p->type == JERRY_API_DATA_TYPE_UINT32)
  {
    return value_p->v_uint32;
  }
  else if (value_p->type == JERRY_API_DATA_TYPE_FLOAT32)
  {
    return value_p->v_float32;
  }
  else
  {
    return value_p->v_float64;
  }
} /* jerry_api_get_number_value */

/**
 * Returns the v_string member of the given jerry_api_value_t structure.
 * If the given jerry_api_value_t structure has type other than
 * JERRY_API_DATA_TYPE_STRING, JERRY_ASSERT fails.
 */
jerry_api_string_t *
jerry_api_get_string_value (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  JERRY_ASSERT (jerry_api_value_is_string (value_p));
  return value_p->v_string;
} /* jerry_api_get_string_value */

/**
 * Returns the v_object member of the given jerry_api_value_t structure.
 * If the given jerry_api_value_t structure has type other than
 * JERRY_API_DATA_TYPE_OBJECT, JERRY_ASSERT fails.
 */
jerry_api_object_t *
jerry_api_get_object_value (const jerry_api_value_t *value_p) /**< pointer to api value */
{
  JERRY_ASSERT (jerry_api_value_is_object (value_p));
  return value_p->v_object;
} /* jerry_api_get_object_value */

/**
 * Creates and returns a jerry_api_value_t with type
 * JERRY_API_DATA_TYPE_VOID.
 */
jerry_api_value_t
jerry_api_create_void_value (void)
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_VOID;
  return jerry_val;
} /* jerry_api_create_void_value */

/**
 * Creates and returns a jerry_api_value_t with type
 * JERRY_API_DATA_TYPE_NULL.
 */
jerry_api_value_t
jerry_api_create_null_value (void)
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_NULL;
  return jerry_val;
} /* jerry_api_create_null_value */

/**
 * Creates and returns a jerry_api_value_t with type
 * JERRY_API_DATA_TYPE_UNDEFINED.
 */
jerry_api_value_t
jerry_api_create_undefined_value (void)
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_UNDEFINED;
  return jerry_val;
} /* jerry_api_create_undefined_value */

/**
 * Creates a JERRY_API_DATA_TYPE_BOOLEAN jerry_api_value_t from the given
 * boolean parameter and returns with it.
 */
jerry_api_value_t
jerry_api_create_boolean_value (bool value) /**< bool value from which a jerry_api_value_t will be created */
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_BOOLEAN;
  jerry_val.v_bool = value;
  return jerry_val;
} /* jerry_api_create_boolean_value */

/**
 * Creates a jerry_api_value_t from the given double parameter and returns
 * with it.
 * The v_float64 member will be set and the will be JERRY_API_DATA_TYPE_FLOAT64.
 */
jerry_api_value_t
jerry_api_create_number_value (double value) /**< double value from which a jerry_api_value_t will be created */
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_FLOAT64;
  jerry_val.v_float64 = value;
  return jerry_val;
} /* jerry_api_create_number_value */

/**
 * Creates a JERRY_API_DATA_TYPE_OBJECT type jerry_api_value_t from the
 * given jerry_api_object_t* parameter and returns with it.
 */
jerry_api_value_t
jerry_api_create_object_value (jerry_api_object_t *value) /**< jerry_api_object_t from which a value will be created */
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_OBJECT;
  jerry_val.v_object = value;
  return jerry_val;
} /* jerry_api_create_object_value */

/**
 * Creates a JERRY_API_DATA_TYPE_STRING type jerry_api_value_t from the
 * given jerry_api_string_t* parameter and returns with it.
 */
jerry_api_value_t
jerry_api_create_string_value (jerry_api_string_t *value) /**< jerry_api_string_t from which a value will be created */
{
  jerry_api_value_t jerry_val;
  jerry_val.type = JERRY_API_DATA_TYPE_STRING;
  jerry_val.v_string = value;
  return jerry_val;
} /* jerry_api_create_string_value */

/**
 * Convert ecma-value to Jerry API value representation
 *
 * Note:
 *      if the output value contains string / object, it should be freed
 *      with jerry_api_release_string / jerry_api_release_object,
 *      just when it becomes unnecessary.
 */
static void
jerry_api_convert_ecma_value_to_api_value (jerry_api_value_t *out_value_p, /**< out: api value */
                                           ecma_value_t value) /**< ecma-value (undefined,
                                                                *   null, boolean, number,
                                                                *   string or object */
{
  jerry_assert_api_available ();

  JERRY_ASSERT (out_value_p != NULL);

  if (ecma_is_value_undefined (value))
  {
    out_value_p->type = JERRY_API_DATA_TYPE_UNDEFINED;
  }
  else if (ecma_is_value_null (value))
  {
    out_value_p->type = JERRY_API_DATA_TYPE_NULL;
  }
  else if (ecma_is_value_boolean (value))
  {
    out_value_p->type = JERRY_API_DATA_TYPE_BOOLEAN;
    out_value_p->v_bool = ecma_is_value_true (value);
  }
  else if (ecma_is_value_number (value))
  {
    ecma_number_t *num = ecma_get_number_from_value (value);

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
    out_value_p->type = JERRY_API_DATA_TYPE_FLOAT32;
    out_value_p->v_float32 = *num;
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    out_value_p->type = JERRY_API_DATA_TYPE_FLOAT64;
    out_value_p->v_float64 = *num;
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
  }
  else if (ecma_is_value_string (value))
  {
    ecma_string_t *str = ecma_get_string_from_value (value);

    out_value_p->type = JERRY_API_DATA_TYPE_STRING;
    out_value_p->v_string = ecma_copy_or_ref_ecma_string (str);
  }
  else if (ecma_is_value_object (value))
  {
    ecma_object_t *obj = ecma_get_object_from_value (value);
    ecma_ref_object (obj);

    out_value_p->type = JERRY_API_DATA_TYPE_OBJECT;
    out_value_p->v_object = obj;
  }
  else
  {
    /* Impossible type of conversion from ecma_value to api_value */
    JERRY_UNREACHABLE ();
  }
} /* jerry_api_convert_ecma_value_to_api_value */

/**
 * Convert value, represented in Jerry API format, to ecma-value.
 *
 * Note:
 *      the output ecma-value should be freed with ecma_free_value when it becomes unnecessary.
 */
static void
jerry_api_convert_api_value_to_ecma_value (ecma_value_t *out_value_p, /**< out: ecma-value */
                                           const jerry_api_value_t* api_value_p) /**< value in Jerry API format */
{
  switch (api_value_p->type)
  {
    case JERRY_API_DATA_TYPE_UNDEFINED:
    {
      *out_value_p = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

      break;
    }
    case JERRY_API_DATA_TYPE_NULL:
    {
      *out_value_p = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);

      break;
    }
    case JERRY_API_DATA_TYPE_BOOLEAN:
    {
      *out_value_p = ecma_make_simple_value (api_value_p->v_bool ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);

      break;
    }
    case JERRY_API_DATA_TYPE_FLOAT32:
    {
      ecma_number_t *num = ecma_alloc_number ();
      *num = static_cast<ecma_number_t> (api_value_p->v_float32);

      *out_value_p = ecma_make_number_value (num);

      break;
    }
    case JERRY_API_DATA_TYPE_FLOAT64:
    {
      ecma_number_t *num = ecma_alloc_number ();
      *num = static_cast<ecma_number_t> (api_value_p->v_float64);

      *out_value_p = ecma_make_number_value (num);

      break;
    }
    case JERRY_API_DATA_TYPE_UINT32:
    {
      ecma_number_t *num = ecma_alloc_number ();
      *num = static_cast<ecma_number_t> (api_value_p->v_uint32);

      *out_value_p = ecma_make_number_value (num);

      break;
    }
    case JERRY_API_DATA_TYPE_STRING:
    {
      ecma_string_t *str_p = ecma_copy_or_ref_ecma_string (api_value_p->v_string);

      *out_value_p = ecma_make_string_value (str_p);

      break;
    }
    case JERRY_API_DATA_TYPE_OBJECT:
    {
      ecma_object_t *obj_p = api_value_p->v_object;

      ecma_ref_object (obj_p);

      *out_value_p = ecma_make_object_value (obj_p);

      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* jerry_api_convert_api_value_to_ecma_value */

/**
 * Convert completion of 'eval' to API value and completion code
 *
 * Note:
 *      if the output value contains string / object, it should be freed
 *      with jerry_api_release_string / jerry_api_release_object,
 *      just when it becomes unnecessary.
 *
 * @return completion code
 */
static jerry_completion_code_t
jerry_api_convert_eval_completion_to_retval (jerry_api_value_t *retval_p, /**< out: api value */
                                             ecma_completion_value_t completion) /**< completion of 'eval'-mode
                                                                                  *   code execution */
{
  jerry_completion_code_t ret_code;

  if (ecma_is_completion_value_normal (completion))
  {
    ret_code = JERRY_COMPLETION_CODE_OK;

    jerry_api_convert_ecma_value_to_api_value (retval_p,
                                               ecma_get_completion_value_value (completion));
  }
  else
  {
    jerry_api_convert_ecma_value_to_api_value (retval_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

    if (ecma_is_completion_value_throw (completion))
    {
      ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (completion));

      ret_code = JERRY_COMPLETION_CODE_OK;
    }
  }

  return ret_code;
} /* jerry_api_convert_eval_completion_to_retval */

/**
 * @}
 */

/**
 * Copy string characters to specified buffer.
 *
 * Note:
 *   '\0' could occur in characters.
 *
 * @return number of bytes, actually copied to the buffer - if string's content was copied successfully;
 *         otherwise (in case size of buffer is insufficient) - negative number, which is calculated
 *         as negation of buffer size, that is required to hold the string's content.
 */
ssize_t
jerry_api_string_to_char_buffer (const jerry_api_string_t *string_p, /**< string descriptor */
                                 jerry_api_char_t *buffer_p, /**< output characters buffer */
                                 ssize_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_available ();

  return ecma_string_to_utf8_string (string_p, (lit_utf8_byte_t *) buffer_p, buffer_size);
} /* jerry_api_string_to_char_buffer */

/**
 * Acquire string pointer for usage outside of the engine
 * from string retrieved in extension routine call from engine.
 *
 * Warning:
 *         acquired pointer should be released with jerry_api_release_string
 *
 * @return pointer that may be used outside of the engine
 */
jerry_api_string_t*
jerry_api_acquire_string (jerry_api_string_t *string_p) /**< pointer passed to function */
{
  jerry_assert_api_available ();

  return ecma_copy_or_ref_ecma_string (string_p);
} /* jerry_api_acquire_string */

/**
 * Release string pointer
 *
 * See also:
 *          jerry_api_acquire_string
 *          jerry_api_call_function
 *
 */
void
jerry_api_release_string (jerry_api_string_t *string_p) /**< pointer acquired through jerry_api_acquire_string */
{
  jerry_assert_api_available ();

  ecma_deref_ecma_string (string_p);
} /* jerry_api_release_string */

/**
 * Acquire object pointer for usage outside of the engine
 * from object retrieved in extension routine call from engine.
 *
 * Warning:
 *         acquired pointer should be released with jerry_api_release_object
 *
 * @return pointer that may be used outside of the engine
 */
jerry_api_object_t*
jerry_api_acquire_object (jerry_api_object_t *object_p) /**< pointer passed to function */
{
  jerry_assert_api_available ();

  ecma_ref_object (object_p);

  return object_p;
} /* jerry_api_acquire_object */

/**
 * Release object pointer
 *
 * See also:
 *          jerry_api_acquire_object
 *          jerry_api_call_function
 *          jerry_api_get_object_field_value
 */
void
jerry_api_release_object (jerry_api_object_t *object_p) /**< pointer acquired through jerry_api_acquire_object */
{
  jerry_assert_api_available ();

  ecma_deref_object (object_p);
} /* jerry_api_release_object */

/**
 * Release specified Jerry API value
 */
void
jerry_api_release_value (jerry_api_value_t *value_p) /**< API value */
{
  jerry_assert_api_available ();

  if (value_p->type == JERRY_API_DATA_TYPE_STRING)
  {
    jerry_api_release_string (value_p->v_string);
  }
  else if (value_p->type == JERRY_API_DATA_TYPE_OBJECT)
  {
    jerry_api_release_object (value_p->v_object);
  }
} /* jerry_api_release_value */

/**
 * Create a string
 *
 * Note:
 *      caller should release the string with jerry_api_release_string, just when the value becomes unnecessary.
 *
 * @return pointer to created string
 */
jerry_api_string_t *
jerry_api_create_string (const jerry_api_char_t *v) /**< string value */
{
  jerry_assert_api_available ();

  return ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) v, lit_zt_utf8_string_size ((lit_utf8_byte_t *) v));
} /* jerry_api_create_string */

/**
 * Create a string
 *
 * Note:
 *      caller should release the string with jerry_api_release_string, just when the value becomes unnecessary.
 *
 * @return pointer to created string
 */
jerry_api_string_t *
jerry_api_create_string_sz (const jerry_api_char_t *v, /**< string value */
                            jerry_api_size_t v_size) /**< string size */
{
  jerry_assert_api_available ();

  return ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) v,
                                         (lit_utf8_size_t) v_size);
} /* jerry_api_create_string_sz */

/**
 * Create an object
 *
 * Note:
 *      caller should release the object with jerry_api_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created object
 */
jerry_api_object_t*
jerry_api_create_object (void)
{
  jerry_assert_api_available ();

  return ecma_op_create_object_object_noarg ();
} /* jerry_api_create_object */

/**
 * Create an array object
 *
 * Note:
 *      caller should release the object with jerry_api_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created array object
 */
jerry_api_object_t *
jerry_api_create_array_object (jerry_api_size_t size) /* size of array */
{
  JERRY_ASSERT (size > 0);

  ecma_number_t *length_num_p = ecma_alloc_number ();
  *length_num_p = ecma_uint32_to_number (size);
  ecma_value_t array_length = ecma_make_number_value (length_num_p);

  jerry_api_length_t argument_size = 1;
  ecma_completion_value_t new_array_completion = ecma_op_create_array_object (&array_length, argument_size, true);
  JERRY_ASSERT (ecma_is_completion_value_normal (new_array_completion));
  ecma_value_t val = ecma_get_completion_value_value (new_array_completion);
  jerry_api_object_t *obj_p = ecma_get_object_from_value (val);

  ecma_free_value (array_length, true);
  return obj_p;
} /* jerry_api_create_array_object */

/**
 * Set value of field in the specified array object
 *
 * @return true, if field value was set successfully
 *         throw exception, otherwise
 */
bool
jerry_api_set_array_index_value (jerry_api_object_t *array_obj_p, /* array object */
                                 jerry_api_length_t index, /* index to be written */
                                 jerry_api_value_t *value_p) /* value to set*/
{
  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_value_t value;
  jerry_api_convert_api_value_to_ecma_value (&value, value_p);
  ecma_completion_value_t set_completion = ecma_op_object_put (array_obj_p, str_idx_p, value, false);
  JERRY_ASSERT (ecma_is_completion_value_normal (set_completion));

  ecma_free_completion_value (set_completion);
  ecma_deref_ecma_string (str_idx_p);
  ecma_free_value (value, true);

  return true;
} /* jerry_api_set_array_index_value */

/**
 * Get value of field in the specified array object
 *
 * Note:
 *      if value was retrieved successfully, it should be freed
 *      with jerry_api_release_value just when it becomes unnecessary.
 *
 * @return true, if field value was retrieved successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         throw exception - otherwise.
 */
bool
jerry_api_get_array_index_value (jerry_api_object_t *array_obj_p, /* array object */
                                 jerry_api_length_t index, /* index to be written */
                                 jerry_api_value_t *value_p) /* output value at index */
{
  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_completion_value_t get_completion = ecma_op_object_get (array_obj_p, str_idx_p);
  JERRY_ASSERT (ecma_is_completion_value_normal (get_completion));
  ecma_value_t val = ecma_get_completion_value_value (get_completion);
  jerry_api_convert_ecma_value_to_api_value (value_p, val);

  ecma_free_completion_value (get_completion);
  ecma_deref_ecma_string (str_idx_p);

  return true;
} /* jerry_api_get_array_index_value */

/**
 * Create an error object
 *
 * Note:
 *      caller should release the object with jerry_api_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created error object
 */
jerry_api_object_t*
jerry_api_create_error (jerry_api_error_t error_type, /**< type of error */
                        const jerry_api_char_t *message_p) /**< value of 'message' property
                                                            *   of constructed error object */
{
  return jerry_api_create_error_sz (error_type,
                                    (lit_utf8_byte_t *) message_p,
                                    lit_zt_utf8_string_size (message_p));
}

/**
 * Create an error object
 *
 * Note:
 *      caller should release the object with jerry_api_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created error object
 */
jerry_api_object_t*
jerry_api_create_error_sz (jerry_api_error_t error_type, /**< type of error */
                           const jerry_api_char_t *message_p, /**< value of 'message' property
                                                               *   of constructed error object */
                           jerry_api_size_t message_size) /**< size of the message in bytes */
{
  jerry_assert_api_available ();

  ecma_standard_error_t standard_error_type = ECMA_ERROR_COMMON;

  switch (error_type)
  {
    case JERRY_API_ERROR_COMMON:
    {
      standard_error_type = ECMA_ERROR_COMMON;
      break;
    }
    case JERRY_API_ERROR_EVAL:
    {
      standard_error_type = ECMA_ERROR_EVAL;
      break;
    }
    case JERRY_API_ERROR_RANGE:
    {
      standard_error_type = ECMA_ERROR_RANGE;
      break;
    }
    case JERRY_API_ERROR_REFERENCE:
    {
      standard_error_type = ECMA_ERROR_REFERENCE;
      break;
    }
    case JERRY_API_ERROR_SYNTAX:
    {
      standard_error_type = ECMA_ERROR_SYNTAX;
      break;
    }
    case JERRY_API_ERROR_TYPE:
    {
      standard_error_type = ECMA_ERROR_TYPE;
      break;
    }
    case JERRY_API_ERROR_URI:
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
    ecma_string_t* message_string_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) message_p,
                                                                      (lit_utf8_size_t) message_size);

    ecma_object_t* error_object_p = ecma_new_standard_error_with_message (standard_error_type, message_string_p);

    ecma_deref_ecma_string (message_string_p);

    return error_object_p;
  }
} /* jerry_api_create_error */

/**
 * Create an external function object
 *
 * Note:
 *      caller should release the object with jerry_api_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to created external function object
 */
jerry_api_object_t*
jerry_api_create_external_function (jerry_external_handler_t handler_p) /**< pointer to native handler
                                                                         *   for the function */
{
  jerry_assert_api_available ();

  return ecma_op_create_external_function_object ((ecma_external_pointer_t) handler_p);
} /* jerry_api_create_external_function */

/**
 * Dispatch call to specified external function using the native handler
 *
 * Note:
 *       if called native handler returns true, then dispatcher just returns value received
 *       through 'return value' output argument, otherwise - throws the value as an exception.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
jerry_dispatch_external_function (ecma_object_t *function_object_p, /**< external function object */
                                  ecma_external_pointer_t handler_p, /**< pointer to the function's native handler */
                                  ecma_value_t this_arg_value, /**< 'this' argument */
                                  ecma_collection_header_t *arg_collection_p) /**< arguments collection */
{
  jerry_assert_api_available ();

  const ecma_length_t args_count = (arg_collection_p != NULL ? arg_collection_p->unit_number : 0);

  ecma_completion_value_t completion_value;

  MEM_DEFINE_LOCAL_ARRAY (api_arg_values, args_count, jerry_api_value_t);

  ecma_collection_iterator_t args_iterator;
  ecma_collection_iterator_init (&args_iterator, arg_collection_p);

  for (uint32_t i = 0; i < args_count; ++i)
  {
    bool is_moved = ecma_collection_iterator_next (&args_iterator);
    JERRY_ASSERT (is_moved);

    jerry_api_convert_ecma_value_to_api_value (&api_arg_values[i], *args_iterator.current_value_p);
  }

  jerry_api_value_t api_this_arg_value, api_ret_value;
  jerry_api_convert_ecma_value_to_api_value (&api_this_arg_value, this_arg_value);

  // default return value
  jerry_api_convert_ecma_value_to_api_value (&api_ret_value,
                                             ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

  bool is_successful = ((jerry_external_handler_t) handler_p) (function_object_p,
                                                               &api_this_arg_value,
                                                               &api_ret_value,
                                                               api_arg_values,
                                                               args_count);

  ecma_value_t ret_value;
  jerry_api_convert_api_value_to_ecma_value (&ret_value, &api_ret_value);

  if (is_successful)
  {
    completion_value = ecma_make_normal_completion_value (ret_value);
  }
  else
  {
    completion_value = ecma_make_throw_completion_value (ret_value);
  }

  jerry_api_release_value (&api_ret_value);
  jerry_api_release_value (&api_this_arg_value);
  for (uint32_t i = 0; i < args_count; i++)
  {
    jerry_api_release_value (&api_arg_values[i]);
  }

  MEM_FINALIZE_LOCAL_ARRAY (api_arg_values);

  return completion_value;
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
jerry_api_is_function (const jerry_api_object_t* object_p) /**< an object */
{
  jerry_assert_api_available ();

  JERRY_ASSERT (object_p != NULL);

  ecma_value_t obj_val = ecma_make_object_value (object_p);

  return ecma_op_is_callable (obj_val);
} /* jerry_api_is_function */

/**
 * Check if the specified object is a constructor function object.
 *
 * @return true - if the specified object is a function object that implements [[Construct]],
 *         false - otherwise.
 */
bool
jerry_api_is_constructor (const jerry_api_object_t* object_p) /**< an object */
{
  jerry_assert_api_available ();

  JERRY_ASSERT (object_p != NULL);

  ecma_value_t obj_val = ecma_make_object_value (object_p);

  return ecma_is_constructor (obj_val);
} /* jerry_api_is_constructor */

/**
 * Create field (named data property) in the specified object
 *
 * @return true, if field was created successfully, i.e. upon the call:
 *                - there is no field with same name in the object;
 *                - the object is extensible;
 *         false - otherwise.
 */
bool
jerry_api_add_object_field (jerry_api_object_t *object_p, /**< object to add field at */
                            const jerry_api_char_t *field_name_p, /**< name of the field */
                            jerry_api_size_t field_name_size, /**< size of field name in bytes */
                            const jerry_api_value_t *field_value_p, /**< value of the field */
                            bool is_writable) /**< flag indicating whether the created field should be writable */
{
  jerry_assert_api_available ();

  bool is_successful = false;

  if (ecma_get_object_extensible (object_p))
  {
    ecma_string_t* field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                      (lit_utf8_size_t) field_name_size);

    ecma_property_t *prop_p = ecma_op_object_get_own_property (object_p, field_name_str_p);

    if (prop_p == NULL)
    {
      is_successful = true;

      ecma_value_t value_to_put;
      jerry_api_convert_api_value_to_ecma_value (&value_to_put, field_value_p);

      prop_p = ecma_create_named_data_property (object_p,
                                                field_name_str_p,
                                                is_writable,
                                                true,
                                                true);
      ecma_named_data_property_assign_value (object_p, prop_p, value_to_put);

      ecma_free_value (value_to_put, true);
    }

    ecma_deref_ecma_string (field_name_str_p);
  }

  return is_successful;
} /* jerry_api_add_object_field */

/**
 * Delete field in the specified object
 *
 * @return true, if field was deleted successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         false - otherwise.
 */
bool
jerry_api_delete_object_field (jerry_api_object_t *object_p, /**< object to delete field at */
                               const jerry_api_char_t *field_name_p, /**< name of the field */
                               jerry_api_size_t field_name_size) /**< size of the field name in bytes */
{
  jerry_assert_api_available ();

  bool is_successful = true;

  ecma_string_t* field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                    (lit_utf8_size_t) field_name_size);

  ecma_completion_value_t delete_completion = ecma_op_object_delete (object_p,
                                                                     field_name_str_p,
                                                                     true);

  if (!ecma_is_completion_value_normal (delete_completion))
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (delete_completion));

    is_successful = false;
  }

  ecma_free_completion_value (delete_completion);

  ecma_deref_ecma_string (field_name_str_p);

  return is_successful;
} /* jerry_api_delete_object_field */

/**
 * Get value of field in the specified object
 *
 * Note:
 *      if value was retrieved successfully, it should be freed
 *      with jerry_api_release_value just when it becomes unnecessary.
 *
 * @return true, if field value was retrieved successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         false - otherwise.
 */
bool jerry_api_get_object_field_value (jerry_api_object_t *object_p, /**< object */
                                       const jerry_api_char_t *field_name_p, /**< field name */
                                       jerry_api_value_t *field_value_p) /**< out: field value */
{
  return jerry_api_get_object_field_value_sz (object_p,
                                              field_name_p,
                                              lit_zt_utf8_string_size (field_name_p),
                                              field_value_p);
}

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
jerry_api_foreach_object_field (jerry_api_object_t *object_p, /**< object */
                               jerry_object_field_foreach_t foreach_p, /**< foreach function */
                               void *user_data_p) /**< user data for foreach function */
{
  jerry_assert_api_available ();

  ecma_collection_iterator_t names_iter;
  ecma_collection_header_t *names_p = ecma_op_object_get_property_names (object_p, false, true, true);
  ecma_collection_iterator_init (&names_iter, names_p);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  bool continuous = true;

  while (ecma_is_completion_value_empty (ret_value)
         && continuous
         && ecma_collection_iterator_next (&names_iter))
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*names_iter.current_value_p);

    ECMA_TRY_CATCH (property_value, ecma_op_object_get (object_p, property_name_p), ret_value);

    jerry_api_value_t field_value;
    jerry_api_convert_ecma_value_to_api_value (&field_value, property_value);

    continuous = foreach_p (property_name_p, &field_value, user_data_p);

    jerry_api_release_value (&field_value);

    ECMA_FINALIZE (property_value);
  }

  ecma_free_values_collection (names_p, true);
  if (ecma_is_completion_value_empty (ret_value))
  {
    return true;
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (ret_value));

    ecma_free_completion_value (ret_value);

    return false;
  }
} /* jerry_api_foreach_object_field */

/**
 * Get value of field in the specified object
 *
 * Note:
 *      if value was retrieved successfully, it should be freed
 *      with jerry_api_release_value just when it becomes unnecessary.
 *
 * @return true, if field value was retrieved successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *         false - otherwise.
 */
bool
jerry_api_get_object_field_value_sz (jerry_api_object_t *object_p, /**< object */
                                     const jerry_api_char_t *field_name_p, /**< name of the field */
                                     jerry_api_size_t field_name_size, /**< size of field name in bytes */
                                     jerry_api_value_t *field_value_p) /**< out: field value, if retrieved
 * successfully */
{
  jerry_assert_api_available ();

  bool is_successful = true;

  ecma_string_t* field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                    (lit_utf8_size_t) field_name_size);

  ecma_completion_value_t get_completion = ecma_op_object_get (object_p,
                                                               field_name_str_p);

  if (ecma_is_completion_value_normal (get_completion))
  {
    ecma_value_t val = ecma_get_completion_value_value (get_completion);

    jerry_api_convert_ecma_value_to_api_value (field_value_p, val);
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (get_completion));

    is_successful = false;
  }

  ecma_free_completion_value (get_completion);

  ecma_deref_ecma_string (field_name_str_p);

  return is_successful;
} /* jerry_api_get_object_field_value */

/**
 * Set value of field in the specified object
 *
 * @return true, if field value was set successfully, i.e. upon the call:
 *                - field value is writable;
 *         false - otherwise.
 */
bool
jerry_api_set_object_field_value (jerry_api_object_t *object_p, /**< object */
                                  const jerry_api_char_t *field_name_p, /**< name of the field */
                                  const jerry_api_value_t *field_value_p) /**< field value to set */
{
  return jerry_api_set_object_field_value_sz (object_p,
                                              field_name_p,
                                              lit_zt_utf8_string_size (field_name_p),
                                              field_value_p);
}

/**
 * Set value of field in the specified object
 *
 * @return true, if field value was set successfully, i.e. upon the call:
 *                - field value is writable;
 *         false - otherwise.
 */
bool
jerry_api_set_object_field_value_sz (jerry_api_object_t *object_p, /**< object */
                                     const jerry_api_char_t *field_name_p, /**< name of the field */
                                     jerry_api_size_t field_name_size, /**< size of field name in bytes */
                                     const jerry_api_value_t *field_value_p) /**< field value to set */
{
  jerry_assert_api_available ();

  bool is_successful = true;

  ecma_string_t* field_name_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) field_name_p,
                                                                    (lit_utf8_size_t) field_name_size);

  ecma_value_t value_to_put;
  jerry_api_convert_api_value_to_ecma_value (&value_to_put, field_value_p);

  ecma_completion_value_t set_completion = ecma_op_object_put (object_p,
                                                               field_name_str_p,
                                                               value_to_put,
                                                               true);

  if (!ecma_is_completion_value_normal (set_completion))
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (set_completion));

    is_successful = false;
  }

  ecma_free_completion_value (set_completion);

  ecma_free_value (value_to_put, true);
  ecma_deref_ecma_string (field_name_str_p);

  return is_successful;
} /* jerry_api_set_object_field_value */

/**
 * Get native handle, associated with specified object
 *
 * @return true - if there is associated handle (handle is returned through out_handle_p),
 *         false - otherwise.
 */
bool
jerry_api_get_object_native_handle (jerry_api_object_t *object_p, /**< object to get handle from */
                                    uintptr_t* out_handle_p) /**< out: handle value */
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
} /* jerry_api_get_object_native_handle */

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
jerry_api_set_object_native_handle (jerry_api_object_t *object_p, /**< object to set handle in */
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
} /* jerry_api_set_object_native_handle */

/**
 * Invoke function specified by a function object
 *
 * Note:
 *      returned value should be freed with jerry_api_release_value
 *      just when the value becomes unnecessary.
 *
 * Note:
 *      If function is invoked as constructor, it should support [[Construct]] method,
 *      otherwise, if function is simply called - it should support [[Call]] method.
 *
 * @return true, if invocation was performed successfully, i.e.:
 *                - no unhandled exceptions were thrown in connection with the call;
 *         false - otherwise.
 */
static bool
jerry_api_invoke_function (bool is_invoke_as_constructor, /**< true - invoke function as constructor
                                                           *          (this_arg_p should be NULL, as it is ignored),
                                                           *   false - perform function call */
                           jerry_api_object_t *function_object_p, /**< function object to call */
                           jerry_api_object_t *this_arg_p, /**< object for 'this' binding
                                                            *   or NULL (set 'this' binding to newly constructed object,
                                                            *            if function is invoked as constructor;
                                                            *            in case of simple function call set 'this'
                                                            *            binding to the global object) */
                           jerry_api_value_t *retval_p, /**< pointer to place for function's
                                                         *   return value / thrown exception value
                                                         *   or NULL (to ignore the values) */
                           const jerry_api_value_t args_p[], /**< function's call arguments
                                                              *   (NULL if arguments number is zero) */
                           jerry_api_length_t args_count) /**< number of the arguments */
{
  JERRY_ASSERT (args_count == 0 || args_p != NULL);
  JERRY_STATIC_ASSERT (sizeof (args_count) == sizeof (ecma_length_t));

  bool is_successful = true;

  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  for (uint32_t i = 0; i < args_count; ++i)
  {
    ecma_value_t arg_value;
    jerry_api_convert_api_value_to_ecma_value (&arg_value, &args_p[i]);

    ecma_append_to_values_collection (arg_collection_p, arg_value, true);

    ecma_free_value (arg_value, true);
  }

  ecma_completion_value_t call_completion;

  if (is_invoke_as_constructor)
  {
    JERRY_ASSERT (this_arg_p == NULL);
    JERRY_ASSERT (jerry_api_is_constructor (function_object_p));

    call_completion = ecma_op_function_construct (function_object_p, arg_collection_p);
  }
  else
  {
    JERRY_ASSERT (jerry_api_is_function (function_object_p));

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
                                             arg_collection_p);
  }

  ecma_free_values_collection (arg_collection_p, true);

  if (!ecma_is_completion_value_normal (call_completion))
  {
    /* unhandled exception during the function call */
    JERRY_ASSERT (ecma_is_completion_value_throw (call_completion));

    is_successful = false;
  }

  if (retval_p != NULL)
  {
    jerry_api_convert_ecma_value_to_api_value (retval_p,
                                               ecma_get_completion_value_value (call_completion));
  }

  ecma_free_completion_value (call_completion);

  return is_successful;
} /* jerry_api_invoke_function */

/**
 * Construct new TypeError object
 */
static void
jerry_api_construct_type_error (jerry_api_value_t *retval_p) /**< out: value with constructed
                                                              *        TypeError object */
{
  ecma_object_t *type_error_obj_p = ecma_new_standard_error (ECMA_ERROR_TYPE);
  ecma_value_t type_error_value = ecma_make_object_value (type_error_obj_p);

  jerry_api_convert_ecma_value_to_api_value (retval_p, type_error_value);

  ecma_deref_object (type_error_obj_p);
} /* jerry_api_construct_type_error */

/**
 * Call function specified by a function object
 *
 * Note:
 *      returned value should be freed with jerry_api_release_value
 *      just when the value becomes unnecessary.
 *
 * @return true, if call was performed successfully, i.e.:
 *                - specified object is a function object (see also jerry_api_is_function);
 *                - no unhandled exceptions were thrown in connection with the call;
 *         false - otherwise, 'retval_p' contains thrown exception:
 *                  if called object is not function object - a TypeError instance;
 *                  else - exception, thrown during the function call.
 */
bool
jerry_api_call_function (jerry_api_object_t *function_object_p, /**< function object to call */
                         jerry_api_object_t *this_arg_p, /**< object for 'this' binding
                                                          *   or NULL (set 'this' binding to the global object) */
                         jerry_api_value_t *retval_p, /**< pointer to place for function's
                                                       *   return value / thrown exception value
                                                       *   or NULL (to ignore the values) */
                         const jerry_api_value_t args_p[], /**< function's call arguments
                                                            *   (NULL if arguments number is zero) */
                         uint16_t args_count) /**< number of the arguments */
{
  jerry_assert_api_available ();

  if (jerry_api_is_function (function_object_p))
  {
    return jerry_api_invoke_function (false, function_object_p, this_arg_p, retval_p, args_p, args_count);
  }
  else
  {
    if (retval_p != NULL)
    {
      jerry_api_construct_type_error (retval_p);
    }

    return false;
  }
} /* jerry_api_call_function */

/**
 * Construct object invoking specified function object as a constructor
 *
 * Note:
 *      returned value should be freed with jerry_api_release_value
 *      just when the value becomes unnecessary.
 *
 * @return true, if construction was performed successfully, i.e.:
 *                - specified object is a constructor function object (see also jerry_api_is_constructor);
 *                - no unhandled exceptions were thrown in connection with the invocation;
 *         false - otherwise, 'retval_p' contains thrown exception:
 *                if  specified object is not a constructor function object - a TypeError instance;
 *                else - exception, thrown during the invocation.
 */
bool
jerry_api_construct_object (jerry_api_object_t *function_object_p, /**< function object to call */
                            jerry_api_value_t *retval_p, /**< pointer to place for function's
                                                          *   return value / thrown exception value
                                                          *   or NULL (to ignore the values) */
                            const jerry_api_value_t args_p[], /**< function's call arguments
                                                               *   (NULL if arguments number is zero) */
                            uint16_t args_count) /**< number of the arguments */
{
  jerry_assert_api_available ();

  if (jerry_api_is_constructor (function_object_p))
  {
    return jerry_api_invoke_function (true, function_object_p, NULL, retval_p, args_p, args_count);
  }
  else
  {
    if (retval_p != NULL)
    {
      jerry_api_construct_type_error (retval_p);
    }

    return false;
  }
} /* jerry_api_construct_object */

/**
 * Get global object
 *
 * Note:
 *       caller should release the object with jerry_api_release_object, just when the value becomes unnecessary.
 *
 * @return pointer to the global object
 */
jerry_api_object_t*
jerry_api_get_global (void)
{
  jerry_assert_api_available ();

  return ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);
} /* jerry_api_get_global */

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
jerry_api_eval (const jerry_api_char_t *source_p, /**< source code */
                size_t source_size, /**< length of source code */
                bool is_direct, /**< perform eval invocation in direct mode */
                bool is_strict, /**< perform eval as it is called from strict mode code */
                jerry_api_value_t *retval_p) /**< out: returned value */
{
  jerry_assert_api_available ();

  jerry_completion_code_t status;

  ecma_completion_value_t completion = ecma_op_eval_chars_buffer ((const lit_utf8_byte_t *) source_p,
                                                                  source_size,
                                                                  is_direct,
                                                                  is_strict);

  status = jerry_api_convert_eval_completion_to_retval (retval_p, completion);

  ecma_free_completion_value (completion);

  return status;
} /* jerry_api_eval */

/**
 * Perform GC
 */
void
jerry_api_gc (void)
{
  jerry_assert_api_available ();

  ecma_gc_run ();
} /* jerry_api_gc */

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

  if (flags & (JERRY_FLAG_MEM_STATS | JERRY_FLAG_MEM_STATS_PER_OPCODE | JERRY_FLAG_MEM_STATS_SEPARATE))
  {
#ifndef MEM_STATS
    flags &= ~(JERRY_FLAG_MEM_STATS
               | JERRY_FLAG_MEM_STATS_PER_OPCODE
               | JERRY_FLAG_MEM_STATS_SEPARATE);

    JERRY_WARNING_MSG ("Ignoring memory statistics option because of '!MEM_STATS' build configuration.\n");
#else /* !MEM_STATS */
    if (flags & (JERRY_FLAG_MEM_STATS_PER_OPCODE | JERRY_FLAG_MEM_STATS_SEPARATE))
    {
      flags |= JERRY_FLAG_MEM_STATS;
    }
#endif /* MEM_STATS */
  }

  jerry_flags = flags;

  jerry_make_api_available ();

  mem_init ();
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

  ecma_finalize ();
  lit_finalize ();
  vm_finalize ();
  mem_finalize (is_show_mem_stats);
} /* jerry_cleanup */

/**
 * Get Jerry configured memory limits
 */
void
jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p, /**< out: Jerry's maximum usage of
                                                            *        data + bss + brk sections */
                         size_t *out_stack_limit_p) /**< out: Jerry's maximum usage of stack */
{
  *out_data_bss_brk_limit_p = CONFIG_MEM_HEAP_AREA_SIZE + CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE;
  *out_stack_limit_p = CONFIG_MEM_STACK_LIMIT;
} /* jerry_get_memory_limits */

/**
 * Check whether 'abort' should be called instead of 'exit' upon exiting with non-zero exit code.
 *
 * @return true - if 'abort on fail' flag is set,
 *         false - otherwise.
 */
bool
jerry_is_abort_on_fail (void)
{
  return ((jerry_flags & JERRY_FLAG_ABORT_ON_FAIL) != 0);
} /* jerry_is_abort_on_fail */

/**
 * Register Jerry's fatal error callback
 */
void
jerry_reg_err_callback (jerry_error_callback_t callback) /**< pointer to callback function */
{
  jerry_assert_api_available ();

  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Error callback is not implemented", callback);
} /* jerry_reg_err_callback */

/**
 * Parse script for specified context
 *
 * @return true - if script was parsed successfully,
 *         false - otherwise (SyntaxError was raised).
 */
bool
jerry_parse (const jerry_api_char_t* source_p, /**< script source */
             size_t source_size) /**< script source size */
{
  jerry_assert_api_available ();

  int is_show_instructions = ((jerry_flags & JERRY_FLAG_SHOW_OPCODES) != 0);

  parser_set_show_instrs (is_show_instructions);

  ecma_compiled_code_t *bytecode_data_p;
  jsp_status_t parse_status;

  parse_status = parser_parse_script (source_p,
                                      source_size,
                                      &bytecode_data_p);

  if (parse_status != JSP_STATUS_OK)
  {
    JERRY_ASSERT (parse_status == JSP_STATUS_SYNTAX_ERROR || parse_status == JSP_STATUS_REFERENCE_ERROR);

    return false;
  }

#ifdef MEM_STATS
  if (jerry_flags & JERRY_FLAG_MEM_STATS_SEPARATE)
  {
    mem_stats_print ();
    mem_stats_reset_peak ();
  }
#endif /* MEM_STATS */

  bool is_show_mem_stats_per_instruction = ((jerry_flags & JERRY_FLAG_MEM_STATS_PER_OPCODE) != 0);

  vm_init (bytecode_data_p, is_show_mem_stats_per_instruction);

  return true;
} /* jerry_parse */

/**
 * Run Jerry in specified run context
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_run (void)
{
  jerry_assert_api_available ();

  return vm_run_global ();
} /* jerry_run */
/**
 * Simple jerry runner
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_run_simple (const jerry_api_char_t *script_source, /**< script source */
                  size_t script_source_size, /**< script source size */
                  jerry_flag_t flags) /**< combination of Jerry flags */
{
  jerry_init (flags);

  jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;

  if (!jerry_parse (script_source, script_source_size))
  {
    /* unhandled SyntaxError */
    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }
  else
  {
    if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
    {
      ret_code = jerry_run ();
    }
  }

  jerry_cleanup ();

  return ret_code;
} /* jerry_run_simple */

#ifdef CONFIG_JERRY_ENABLE_CONTEXTS
/**
 * Allocate new run context
 *
 * @return run context
 */
jerry_ctx_t*
jerry_new_ctx (void)
{
  jerry_assert_api_available ();

  JERRY_UNIMPLEMENTED ("Run contexts are not implemented");
} /* jerry_new_ctx */

/**
 * Cleanup resources associated with specified run context
 */
void
jerry_cleanup_ctx (jerry_ctx_t* ctx_p) /**< run context */
{
  jerry_assert_api_available ();

  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Run contexts are not implemented", ctx_p);
} /* jerry_cleanup_ctx */

/**
 * Activate context and push it to contexts' stack
 */
void
jerry_push_ctx (jerry_ctx_t *ctx_p) /**< run context */
{
  jerry_assert_api_available ();

  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Run contexts are not implemented", ctx_p);
} /* jerry_push_ctx */

/**
 * Pop from contexts' stack and activate new stack's top
 *
 * Note:
 *      default context (most placed on bottom of stack) cannot be popped
 */
void
jerry_pop_ctx (void)
{
  jerry_assert_api_available ();

  JERRY_UNIMPLEMENTED ("Run contexts are not implemented");
} /* jerry_pop_ctx */
#endif /* CONFIG_JERRY_ENABLE_CONTEXTS */


/**
 * Register external magic string array
 */
void
jerry_register_external_magic_strings (const jerry_api_char_ptr_t* ex_str_items, /**< character arrays, representing
                                                                                  * external magic strings' contents */
                                       uint32_t count,                           /**< number of the strings */
                                       const jerry_api_length_t* str_lengths)    /**< lengths of the strings */
{
  lit_magic_strings_ex_set ((const lit_utf8_byte_t **) ex_str_items, count, (const lit_utf8_size_t *) str_lengths);
} /* jerry_register_external_magic_strings */

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE

/*
 * Tells whether snapshot taking is in progress.
 */
bool snapshot_report_byte_code_compilation = false;

/*
 * Mapping snapshot to offset.
 */
typedef struct
{
  mem_cpointer_t next_cp;
  mem_cpointer_t compiled_code_cp;
  uint16_t offset;
} compiled_code_map_entry_t;

/*
 * Variables required to take a snapshot.
 */
static size_t snapshot_buffer_write_offset;
static size_t snapshot_last_compiled_code_offset;
static bool snapshot_error_occured;
static uint8_t *snapshot_buffer_p;
static size_t snapshot_buffer_size;
static compiled_code_map_entry_t *snapshot_map_entries_p;

/**
 * Snapshot callback for byte codes.
 */
void
snapshot_add_compiled_code (ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                            const uint8_t *regexp_pattern, /**< regular expression pattern */
                            uint32_t size) /**< compiled code or regular expression size */
{
  if (snapshot_error_occured)
  {
    return;
  }

  JERRY_ASSERT ((snapshot_buffer_write_offset & (MEM_ALIGNMENT - 1)) == 0);

  if ((snapshot_buffer_write_offset >> MEM_ALIGNMENT_LOG) > 0xffffu)
  {
    snapshot_error_occured = true;
    return;
  }

  snapshot_last_compiled_code_offset = snapshot_buffer_write_offset;

  compiled_code_map_entry_t *new_entry = (compiled_code_map_entry_t *) mem_pools_alloc ();

  if (new_entry == NULL)
  {
    snapshot_error_occured = true;
    return;
  }

  ECMA_SET_POINTER (new_entry->next_cp, snapshot_map_entries_p);
  ECMA_SET_POINTER (new_entry->compiled_code_cp, compiled_code_p);

  new_entry->offset = (uint16_t) (snapshot_buffer_write_offset >> MEM_ALIGNMENT_LOG);
  snapshot_map_entries_p = new_entry;

  const void *data_p = (const void *) compiled_code_p;

  if (!(compiled_code_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
    size += (uint32_t) sizeof (uint16_t);
  }
  else
  {
    JERRY_ASSERT (regexp_pattern == NULL);
  }

  if (!jrt_write_to_buffer_by_offset (snapshot_buffer_p,
                                      snapshot_buffer_size,
                                      &snapshot_buffer_write_offset,
                                      &size,
                                      sizeof (uint32_t)))
  {
    snapshot_error_occured = true;
    return;
  }

  if (!(compiled_code_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
    size -= (uint32_t) sizeof (uint16_t);

    if (!jrt_write_to_buffer_by_offset (snapshot_buffer_p,
                                        snapshot_buffer_size,
                                        &snapshot_buffer_write_offset,
                                        &compiled_code_p->status_flags,
                                        sizeof (uint16_t)))
    {
      snapshot_error_occured = true;
      return;
    }

    data_p = regexp_pattern;
  }

  if (!jrt_write_to_buffer_by_offset (snapshot_buffer_p,
                                      snapshot_buffer_size,
                                      &snapshot_buffer_write_offset,
                                      data_p,
                                      size))
  {
    snapshot_error_occured = true;
    return;
  }

  snapshot_buffer_write_offset = JERRY_ALIGNUP (snapshot_buffer_write_offset, MEM_ALIGNMENT);

  if (snapshot_buffer_write_offset > snapshot_buffer_size)
  {
    snapshot_error_occured = true;
    return;
  }
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
    uint32_t code_size = *(uint32_t *) buffer_p;

    buffer_p += sizeof (uint32_t);

    ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) buffer_p;

    /* Set reference counter to 1. */
    bytecode_p->status_flags &= (1 << ECMA_BYTECODE_REF_SHIFT) - 1;
    bytecode_p->status_flags |= 1 << ECMA_BYTECODE_REF_SHIFT;

    if (bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION)
    {
      lit_cpointer_t *literal_start_p;
      uint32_t literal_end;
      uint32_t const_literal_end;

      if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
      {
        literal_start_p = (lit_cpointer_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

        cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
        literal_end = args_p->literal_end;
        const_literal_end = args_p->const_literal_end;
      }
      else
      {
        literal_start_p = (lit_cpointer_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

        cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
        literal_end = args_p->literal_end;
        const_literal_end = args_p->const_literal_end;
      }

      for (uint32_t i = 0; i < const_literal_end; i++)
      {
        lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

        if (literal_start_p[i].packed_value != MEM_CP_NULL)
        {
          while (current_p->literal_id.packed_value != literal_start_p[i].packed_value)
          {
            current_p++;
          }

          literal_start_p[i].packed_value = (uint16_t) current_p->literal_offset;
        }
      }

      for (uint32_t i = const_literal_end; i < literal_end; i++)
      {
        compiled_code_map_entry_t *current_p = snapshot_map_entries_p;

        while (current_p->compiled_code_cp != literal_start_p[i].value.base_cp)
        {
          current_p = ECMA_GET_NON_NULL_POINTER (compiled_code_map_entry_t,
                                                 current_p->next_cp);
        }

        literal_start_p[i].packed_value = (uint16_t) current_p->offset;
      }
    }

    code_size += (uint32_t) sizeof (uint32_t);
    code_size = JERRY_ALIGNUP (code_size, MEM_ALIGNMENT);

    buffer_p += code_size - sizeof (uint32_t);
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
jerry_parse_and_save_snapshot (const jerry_api_char_t* source_p, /**< script source */
                               size_t source_size, /**< script source size */
                               bool is_for_global, /**< snapshot would be executed as global (true)
                                                    *   or eval (false) */
                               uint8_t *buffer_p, /**< buffer to dump snapshot to */
                               size_t buffer_size) /**< the buffer's size */
{
#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
  jsp_status_t parse_status;
  ecma_compiled_code_t *bytecode_data_p;

  snapshot_buffer_write_offset = 0;
  snapshot_last_compiled_code_offset = 0;
  snapshot_error_occured = false;
  snapshot_buffer_p = buffer_p;
  snapshot_buffer_size = buffer_size;
  snapshot_map_entries_p = NULL;

  uint64_t version = JERRY_SNAPSHOT_VERSION;
  if (!jrt_write_to_buffer_by_offset (buffer_p,
                                      buffer_size,
                                      &snapshot_buffer_write_offset,
                                      &version,
                                      sizeof (version)))
  {
    return 0;
  }

  snapshot_buffer_write_offset = JERRY_ALIGNUP (snapshot_buffer_write_offset, MEM_ALIGNMENT);

  size_t header_offset = snapshot_buffer_write_offset;

  snapshot_buffer_write_offset += JERRY_ALIGNUP (sizeof (jerry_snapshot_header_t), MEM_ALIGNMENT);

  if (snapshot_buffer_write_offset > buffer_size)
  {
    return 0;
  }

  size_t compiled_code_start = snapshot_buffer_write_offset;

  snapshot_report_byte_code_compilation = true;

  if (is_for_global)
  {
    parse_status = parser_parse_script (source_p, source_size, &bytecode_data_p);
  }
  else
  {
    parse_status = parser_parse_eval (source_p,
                                      source_size,
                                      false,
                                      &bytecode_data_p);
  }

  snapshot_report_byte_code_compilation = false;

  if (parse_status == JSP_STATUS_OK
      && !snapshot_error_occured)
  {
    JERRY_ASSERT (snapshot_last_compiled_code_offset != 0);

    jerry_snapshot_header_t header;
    header.last_compiled_code_offset = (uint32_t) snapshot_last_compiled_code_offset;
    header.is_run_global = is_for_global;

    size_t compiled_code_size = snapshot_buffer_write_offset - compiled_code_start;

    lit_mem_to_snapshot_id_map_entry_t* lit_map_p = NULL;
    uint32_t literals_num;

    if (!lit_dump_literals_for_snapshot (buffer_p,
                                         buffer_size,
                                         &snapshot_buffer_write_offset,
                                         &lit_map_p,
                                         &literals_num,
                                         &header.lit_table_size))
    {
      JERRY_ASSERT (lit_map_p == NULL);
      snapshot_buffer_write_offset = 0;
    }
    else
    {
      if (header.lit_table_size > 0xffff)
      {
        /* Aligning literals could increase this range, but
         * it is not a requirement for low-memory environments. */
        snapshot_buffer_write_offset = 0;
      }
      else
      {
        jerry_snapshot_set_offsets (buffer_p + compiled_code_start,
                                    (uint32_t) compiled_code_size,
                                    lit_map_p);

        jrt_write_to_buffer_by_offset (buffer_p,
                                       buffer_size,
                                       &header_offset,
                                       &header,
                                       sizeof (header));
      }

      mem_heap_free_block (lit_map_p);
    }

    ecma_bytecode_deref (bytecode_data_p);
  }
  else
  {
    snapshot_buffer_write_offset = 0;
  }

  compiled_code_map_entry_t *current_p = snapshot_map_entries_p;

  while (current_p != NULL)
  {
    compiled_code_map_entry_t *next_p = ECMA_GET_POINTER (compiled_code_map_entry_t,
                                                          current_p->next_cp);
    mem_pools_free ((uint8_t *) current_p);
    current_p = next_p;
  }

  return snapshot_buffer_write_offset;
#else /* JERRY_ENABLE_SNAPSHOT_SAVE */
  (void) source_p;
  (void) source_size;
  (void) is_for_global;
  (void) buffer_p;
  (void) buffer_size;

  return 0;
#endif /* !JERRY_ENABLE_SNAPSHOT_SAVE */
} /* jerry_parse_and_save_snapshot */

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC

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
  uint32_t code_size = *(uint32_t *) (snapshot_data_p + offset);

  ecma_compiled_code_t *bytecode_p;

  bytecode_p = (ecma_compiled_code_t *) (snapshot_data_p + offset + sizeof (uint32_t));

  if (!(bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
    re_compiled_code_t *re_bytecode_p = NULL;

    const uint8_t *regex_start_p = ((const uint8_t *) bytecode_p) + sizeof (uint16_t);

    code_size -= (uint32_t) sizeof (uint16_t);

    ecma_string_t *pattern_str_p = ecma_new_ecma_string_from_utf8 (regex_start_p,
                                                                   code_size);

    re_compile_bytecode (&re_bytecode_p,
                         pattern_str_p,
                         bytecode_p->status_flags);

    ecma_deref_ecma_string (pattern_str_p);

    return (ecma_compiled_code_t *) re_bytecode_p;
#else
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

  if (copy_bytecode)
  {
    bytecode_p = (ecma_compiled_code_t *) mem_heap_alloc_block (code_size,
                                                                MEM_HEAP_ALLOC_LONG_TERM);

    memcpy (bytecode_p, snapshot_data_p + offset + sizeof (uint32_t), code_size);
  }
  else
  {
    code_size = (uint32_t) (header_size + literal_end * sizeof (lit_cpointer_t));

    uint8_t *real_bytecode_p = ((uint8_t *) bytecode_p) + code_size;

    bytecode_p = (ecma_compiled_code_t *) mem_heap_alloc_block (code_size + 1 + sizeof (uint8_t *),
                                                                MEM_HEAP_ALLOC_LONG_TERM);

    memcpy (bytecode_p, snapshot_data_p + offset + sizeof (uint32_t), code_size);

    uint8_t *instructions_p = ((uint8_t *) bytecode_p);

    instructions_p[code_size] = CBC_SET_BYTECODE_PTR;
    memcpy (instructions_p + code_size + 1, &real_bytecode_p, sizeof (uint8_t *));
  }

  JERRY_ASSERT ((bytecode_p->status_flags >> ECMA_BYTECODE_REF_SHIFT) == 1);

  lit_cpointer_t *literal_start_p = (lit_cpointer_t *) (((uint8_t *) bytecode_p) + header_size);

  for (uint32_t i = 0; i < const_literal_end; i++)
  {
    lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

    if (literal_start_p[i].packed_value != 0)
    {
      while (current_p->literal_offset != literal_start_p[i].packed_value)
      {
        current_p++;
      }

      literal_start_p[i] = current_p->literal_id;
    }
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    size_t literal_offset = ((size_t) literal_start_p[i].packed_value) << MEM_ALIGNMENT_LOG;

    if (literal_offset == offset)
    {
      /* Self reference */
      ECMA_SET_NON_NULL_POINTER (literal_start_p[i].value.base_cp,
                                 bytecode_p);
    }
    else
    {
      ecma_compiled_code_t *literal_bytecode_p;
      literal_bytecode_p = snapshot_load_compiled_code (snapshot_data_p,
                                                        literal_offset,
                                                        lit_map_p,
                                                        copy_bytecode);

      ECMA_SET_NON_NULL_POINTER (literal_start_p[i].value.base_cp,
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
                     jerry_api_value_t *retval_p) /**< out: returned value (ECMA-262 'undefined' if
                                                   * code is executed as global scope code) */
{
  jerry_api_convert_ecma_value_to_api_value (retval_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
  JERRY_ASSERT (snapshot_p != NULL);

  const uint8_t *snapshot_data_p = (uint8_t *) snapshot_p;
  size_t snapshot_read = 0;
  uint64_t version;

  if (!jrt_read_from_buffer_by_offset (snapshot_data_p,
                                       snapshot_size,
                                       &snapshot_read,
                                       &version,
                                       sizeof (version)))
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT;
  }

  if (version != JERRY_SNAPSHOT_VERSION)
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION;
  }

  const jerry_snapshot_header_t *header_p = (const jerry_snapshot_header_t *) (snapshot_data_p + snapshot_read);

  snapshot_read = header_p->last_compiled_code_offset;

  JERRY_ASSERT (snapshot_read == JERRY_ALIGNUP (snapshot_read, MEM_ALIGNMENT));

  uint32_t last_code_size = *(uint32_t *) (snapshot_data_p + snapshot_read);

  snapshot_read += JERRY_ALIGNUP (last_code_size + sizeof (uint32_t), MEM_ALIGNMENT);

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num;

  JERRY_ASSERT (snapshot_read + header_p->lit_table_size <= snapshot_size);

  if (!lit_load_literals_from_snapshot (snapshot_data_p + snapshot_read,
                                        header_p->lit_table_size,
                                        &lit_map_p,
                                        &literals_num))
  {
    JERRY_ASSERT (lit_map_p == NULL);
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT;
  }

  ecma_compiled_code_t *bytecode_p;

  snapshot_read = header_p->last_compiled_code_offset;

  bytecode_p = snapshot_load_compiled_code (snapshot_data_p,
                                            snapshot_read,
                                            lit_map_p,
                                            copy_bytecode);

  if (lit_map_p != NULL)
  {
    mem_heap_free_block (lit_map_p);
  }

  if (bytecode_p == NULL)
  {
    return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT;
  }

  jerry_completion_code_t ret_code;

  if (header_p->is_run_global)
  {
    vm_init (bytecode_p, false);

    ret_code = vm_run_global ();

    vm_finalize ();
  }
  else
  {
    /* vm should be already initialized */
    ecma_completion_value_t completion = vm_run_eval (bytecode_p, false);

    ret_code = jerry_api_convert_eval_completion_to_retval (retval_p, completion);

    ecma_free_completion_value (completion);
  }

  return ret_code;
#else /* JERRY_ENABLE_SNAPSHOT_EXEC */
  (void) snapshot_p;
  (void) snapshot_size;
  (void) copy_bytecode;
  (void) retval_p;

  return JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION;
#endif /* !JERRY_ENABLE_SNAPSHOT_EXEC */
} /* jerry_exec_snapshot */
