/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "deserializer.h"
#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-extension.h"
#include "ecma-gc.h"
#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-init-finalize.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "mem-heap.h"
#include "jerry.h"
#include "jrt.h"
#include "parser.h"
#include "serializer.h"
#include "vm.h"

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

/** \addtogroup jerry Jerry engine extension interface
 * @{
 */

/**
 * Buffer of character data (used for exchange between core and extensions' routines)
 */
char jerry_extension_characters_buffer [CONFIG_EXTENSION_CHAR_BUFFER_SIZE];


static jerry_api_value_t
convert_ecma_value_to_api_value (const ecma_value_t& value)
{
  jerry_api_value_t ret;
  if (ecma_is_value_empty (value))
  {
    ret.type = JERRY_API_DATA_TYPE_EMPTY;
  }
  else if (ecma_is_value_undefined (value))
  {
    ret.type = JERRY_API_DATA_TYPE_UNDEFINED;
  }
  else if (ecma_is_value_boolean (value))
  {
    ret.type = JERRY_API_DATA_TYPE_BOOLEAN;
    ret.v_bool = ecma_is_value_true (value);
  }
  else if (ecma_is_value_number (value))
  {
    ecma_number_t *num = ecma_get_number_from_value(value);

    #if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32

    ret.type = JERRY_API_DATA_TYPE_FLOAT32;
    ret.v_float32 = *num;

    #elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64

    ret.type = JERRY_API_DATA_TYPE_FLOAT64;
    ret.v_float64 = *num;

    #endif
  }
  else if (ecma_is_value_string (value))
  {
    ecma_string_t *str = ecma_get_string_from_value(value);

    ret.type = JERRY_API_DATA_TYPE_STRING;
    ret.v_string = ecma_copy_or_ref_ecma_string(str);
  }
  else if (ecma_is_value_object (value))
  {
    ecma_object_t *obj = ecma_get_object_from_value(value);
    ecma_ref_object(obj);

    ret.type = JERRY_API_DATA_TYPE_OBJECT;
    ret.v_object = obj;
  } else {
    JERRY_UNIMPLEMENTED ("Unsupported type of conversion from ecma_value to api_value");
  }
  return ret;
}


static ecma_value_t
convert_api_value_to_ecma_value (const jerry_api_value_t& value)
{
  switch (value.type)
  {
    case JERRY_API_DATA_TYPE_EMPTY:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
    }
    case JERRY_API_DATA_TYPE_UNDEFINED:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    case JERRY_API_DATA_TYPE_NULL:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
    case JERRY_API_DATA_TYPE_BOOLEAN:
    {
      return ecma_make_simple_value (value.v_bool ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }
    case JERRY_API_DATA_TYPE_FLOAT32:
    {
      ecma_number_t *num = ecma_alloc_number ();
      *num = static_cast<ecma_number_t> (value.v_float32);
      return ecma_make_number_value (num);
    }
    case JERRY_API_DATA_TYPE_FLOAT64:
    {
      ecma_number_t *num = ecma_alloc_number ();
      *num = static_cast<ecma_number_t> (value.v_float64);
      return ecma_make_number_value (num);
    }
    case JERRY_API_DATA_TYPE_UINT32:
    {
      ecma_number_t *num = ecma_alloc_number ();
      *num = static_cast<ecma_number_t> (value.v_uint32);
      return ecma_make_number_value (num);
    }
    case JERRY_API_DATA_TYPE_STRING:
    {
      return ecma_make_string_value (value.v_string);
    }
    case JERRY_API_DATA_TYPE_OBJECT:
    {
      return ecma_make_object_value (value.v_object);
    }
  }
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
}


/**
 * Extend Global scope with specified extension object
 *
 * After extension the object is accessible through non-configurable property
 * with name equal to builtin_object_name converted to ecma chars.
 */
bool
jerry_extend_with (jerry_extension_descriptor_t *desc_p) /**< description of the extension object */
{
  return ecma_extension_register (desc_p);
} /* jerry_extend_with */

/**
 * @}
 */

/**
 * Copy string characters to specified buffer, append zero character at end of the buffer.
 *
 * @return number of bytes, actually copied to the buffer - if string's content was copied successfully;
 *         otherwise (in case size of buffer is insuficcient) - negative number, which is calculated
 *         as negation of buffer size, that is required to hold the string's content.
 */
ssize_t
jerry_api_string_to_char_buffer (const jerry_api_string_t *string_p, /**< string descriptor */
                             char *buffer_p, /**< output characters buffer */
                             ssize_t buffer_size) /**< size of output buffer */
{
  return ecma_string_to_zt_string (string_p, (ecma_char_t*) buffer_p, buffer_size);
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
  ecma_deref_object (object_p);
} /* jerry_api_release_object */

/**
 * Call function specified by a function object
 *
 * Note:
 *      if call was performed successfully and returned value of type string or object, then caller
 *      should release the string / object with corresponding jerry_api_release_string / jerry_api_release_object,
 *      just when the value becomes unnecessary.
 *
 * @return true, if call was performed successfully, i.e.:
 *                - arguments number equals to the function's length property;
 *                - no unhandled exceptions were thrown;
 *                - returned value type is corresponding to one of jerry_api_data_type_t (if retval_p is not NULL)
 *                  or is 'undefined' (if retval_p is NULL);
 *         false - otherwise.
 */
#include <stdio.h>
#include "ecma-helpers.h"
bool
jerry_api_call_function (jerry_api_object_t *function_object_p, /**< function object to call */
                         jerry_api_value_t *retval_p, /**< place for function's return value (if it is required)
                                                       *   or NULL (if it should be 'undefined') */
                         const jerry_api_value_t args_p [], /**< function's call arguments
                                                             *   (NULL if arguments number is zero) */
                         uint32_t args_count) /**< number of the arguments */
{
  JERRY_ASSERT (args_count == 0 || args_p != NULL);

  MEM_DEFINE_LOCAL_ARRAY(arg_values, args_count, ecma_value_t);

  for (uint32_t i = 0; i < args_count; ++i) {
    arg_values[i] = convert_api_value_to_ecma_value (args_p[i]);
    }

  ecma_completion_value_t call_result = ecma_op_function_call (
      function_object_p,
      ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
      arg_values,
      static_cast<ecma_length_t> (args_count));

  ecma_value_t val = ecma_get_completion_value_value (call_result);
  *retval_p = convert_ecma_value_to_api_value (val);

  MEM_FINALIZE_LOCAL_ARRAY (arg_values);

  return true;
} /* jerry_api_call_function */

/**
* Get global object
*
* Note:
*       caller should release the object with jerry_api_release_objc, just when the value becomes unnecessary.
*
* @return pointer to the global object
*/
jerry_api_object_t*
jerry_api_get_global (void)
{
  return jerry_api_acquire_object (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
}

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
  return ecma_op_create_object_object_noarg ();
} /* jerry_api_create_object */

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
                            const char *field_name_p, /**< name of the field */
                            const jerry_api_value_t *field_value_p, /**< value of the field */
                            bool is_writable) /**< flag indicating whether the created field should be writable */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("API routine is not implemented",
                                       object_p, field_name_p, field_value_p, is_writable);
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
                               const char *field_name_p) /**< name of the field */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("API routine is not implemented",
                                       object_p, field_name_p);
} /* jerry_api_delete_object_field */

/**
 * Get value of field in the specified object
 *
 * Note:
 *      if value was retrieved successfully and it is of type string or object, then caller
 *      should release the string / object with corresponding jerry_api_release_string / jerry_api_release_object,
 *      just when the value becomes unnecessary.
 *
 * @return true, if field value was retrieved successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *                - field value is not undefined nor null;
 *         false - otherwise.
 */
bool
jerry_api_get_object_field_value (jerry_api_object_t *object_p, /**< object */
                                  const char *field_name_p, /**< name of the field */
                                  jerry_api_value_t *field_value_p) /**< out: field value, if retrieved successfully */
{
  JERRY_ASSERT(object_p != NULL);

  ecma_string_t* field_name_str_p = ecma_new_ecma_string ((const ecma_char_t*)field_name_p);
  ecma_value_t val = ecma_get_completion_value_value (ecma_op_object_get (object_p, field_name_str_p));
  *field_value_p = convert_ecma_value_to_api_value (val);
  return true;
} /* jerry_api_get_object_field_value */

/**
 * Set value of field in the specified object
 *
 * @return true, if field value was set successfully, i.e. upon the call:
 *                - there is field with specified name in the object;
 *                - field value is writable;
 *         false - otherwise.
 */
bool
jerry_api_set_object_field_value (jerry_api_object_t *object_p, /**< object */
                                  const char *field_name_p, /**< name of the field */
                                  const jerry_api_value_t *field_value_p) /**< field value to set */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("API routine is not implemented",
                                       object_p, field_name_p, field_value_p);
} /* jerry_api_set_object_field_value */


/**
 * Jerry engine initialization
 */
void
jerry_init (jerry_flag_t flags) /**< combination of Jerry flags */
{
  jerry_flags = flags;

  mem_init ();
  deserializer_init ();
  ecma_init ();
} /* jerry_init */

/**
 * Terminate Jerry engine
 *
 * Warning:
 *  All contexts should be freed with jerry_cleanup_ctx
 *  before calling the cleanup routine.
 */
void
jerry_cleanup (void)
{
  bool is_show_mem_stats = ((jerry_flags & JERRY_FLAG_MEM_STATS) != 0);

  ecma_finalize ();
  deserializer_free ();
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
 * Register Jerry's fatal error callback
 */
void
jerry_reg_err_callback (jerry_error_callback_t callback) /**< pointer to callback function */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Error callback is not implemented", callback);
} /* jerry_reg_err_callback */

/**
 * Allocate new run context
 */
jerry_ctx_t*
jerry_new_ctx (void)
{
  JERRY_UNIMPLEMENTED ("Run contexts are not implemented");
} /* jerry_new_ctx */

/**
 * Cleanup resources associated with specified run context
 */
void
jerry_cleanup_ctx (jerry_ctx_t* ctx_p) /**< run context */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS ("Run contexts are not implemented", ctx_p);
} /* jerry_cleanup_ctx */

/**
 * Parse script for specified context
 */
bool
jerry_parse (jerry_ctx_t* ctx_p, /**< run context */
             const char* source_p, /**< script source */
             size_t source_size) /**< script source size */
{
  /* FIXME: Remove after implementation of run contexts */
  (void) ctx_p;

  bool is_show_opcodes = ((jerry_flags & JERRY_FLAG_SHOW_OPCODES) != 0);

  parser_init (source_p, source_size, is_show_opcodes);
  parser_parse_program ();

  const opcode_t* opcodes = (const opcode_t*) deserialize_bytecode ();

  serializer_print_opcodes ();
  parser_free ();

  bool is_show_mem_stats = ((jerry_flags & JERRY_FLAG_MEM_STATS) != 0);
  init_int (opcodes, is_show_mem_stats);

  return true;
} /* jerry_parse */

/**
 * Run Jerry in specified run context
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_run (jerry_ctx_t* ctx_p) /**< run context */
{
  /* FIXME: Remove after implementation of run contexts */
  (void) ctx_p;

  return run_int ();
} /* jerry_run */

/**
 * Simple jerry runner
 *
 * @return completion status
 */
jerry_completion_code_t
jerry_run_simple (const char *script_source, /**< script source */
                  size_t script_source_size, /**< script source size */
                  jerry_flag_t flags) /**< combination of Jerry flags */
{
  jerry_init (flags);

  jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;

  if (!jerry_parse (NULL, script_source, script_source_size))
  {
    /* unhandled SyntaxError */
    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }
  else
  {
    if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
    {
      ret_code = jerry_run (NULL);
    }
  }

  jerry_cleanup ();

  return ret_code;
} /* jerry_run_simple */
