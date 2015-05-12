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

#include "jerry.h"

/* Counting fields */
enum
{
#define EXTENSION_FIELD(_field_name, _type, _value) \
  JERRY_EXTENSION_ ## EXTENSION_NAME ## _ ## _field_name,
# include EXTENSION_DESCRIPTION_HEADER
#undef EXTENSION_FIELD
  JERRY_EXTENSION_FIELDS_NUMBER
};

/* Counting functions */
enum
{
#define EXTENSION_FUNCTION(_function_name, _function_wrapper, _ret_value_type, _args_number, ... /* args */) \
  JERRY_EXTENSION_ ## EXTENSION_NAME ## _ ## _function_name,
# include EXTENSION_DESCRIPTION_HEADER
#undef EXTENSION_FUNCTION
  JERRY_EXTENSION_FUNCTIONS_NUMBER
};

/* Fields description */
static const jerry_extension_field_t jerry_extension_fields[JERRY_EXTENSION_FIELDS_NUMBER + 1] =
{
#define EXTENSION_FIELD(_field_name, _type, _value) \
  { # _field_name, JERRY_API_DATA_TYPE_ ## _type, { _value } },
# include EXTENSION_DESCRIPTION_HEADER
#undef EXTENSION_FIELD
#define EMPTY_FIELD_ENTRY { NULL, JERRY_API_DATA_TYPE_UNDEFINED, { NULL } }
  EMPTY_FIELD_ENTRY
#undef EMPTY_FIELD_ENTRY
};

/* Functions wrapper definitions */
#define EXTENSION_ARG_PASS_BOOL(_arg_index) \
  args_p[_arg_index].v_bool
#define EXTENSION_ARG_PASS_FLOAT32(_arg_index) \
  args_p[_arg_index].v_float32
#define EXTENSION_ARG_PASS_FLOAT64(_arg_index) \
  args_p[_arg_index].v_float64
#define EXTENSION_ARG_PASS_UINT32(_arg_index) \
  args_p[_arg_index].v_uint32
#define EXTENSION_ARG_PASS_STRING(_arg_index) \
  args_p[_arg_index].v_string
#define EXTENSION_ARG_PASS_OBJECT(_arg_index) \
  args_p[_arg_index].v_object
#define EXTENSION_ARG(_arg_index, _type) EXTENSION_ARG_PASS_ ## _type(_arg_index)
#define EXTENSION_RET_VALUE_SET_VOID
#define EXTENSION_RET_VALUE_SET_BOOLEAN function_block_p->ret_value.v_bool =
#define EXTENSION_RET_VALUE_SET_UINT32 function_block_p->ret_value.v_uint32 =
#define EXTENSION_RET_VALUE_SET_FLOAT32 function_block_p->ret_value.v_float32 =
#define EXTENSION_RET_VALUE_SET_FLOAT64 function_block_p->ret_value.v_float64 =
#define EXTENSION_RET_VALUE_SET_STRING function_block_p->ret_value.v_string =
#define EXTENSION_RET_VALUE_SET_OBJECT function_block_p->ret_value.v_object =
#define EXTENSION_FUNCTION(_function_name, _function_to_call, _ret_value_type, _args_number, ...) \
  static void jerry_extension_ ## _function_name ## _wrapper (jerry_extension_function_t *function_block_p) \
  { \
     const jerry_api_value_t *args_p = function_block_p->args_p; \
     EXTENSION_RET_VALUE_SET_ ## _ret_value_type _function_to_call (__VA_ARGS__); \
  }
# include EXTENSION_DESCRIPTION_HEADER
#undef EXTENSION_FUNCTION
#undef EXTENSION_ARG
#undef EXTENSION_ARG_PASS_OBJECT
#undef EXTENSION_ARG_PASS_STRING
#undef EXTENSION_ARG_PASS_UINT32
#undef EXTENSION_ARG_PASS_FLOAT64
#undef EXTENSION_ARG_PASS_FLOAT32
#undef EXTENSION_ARG_PASS_BOOL

/* Functions' arguments description */
#define EXTENSION_ARG(_arg_index, _type) [_arg_index] = { \
  (JERRY_API_DATA_TYPE_ ## _type), \
  { false } /* just for initialization, should be overwritten upon call */ \
}
#define EXTENSION_FUNCTION(_function_name, _function_to_call, _ret_value_type, _args_number, ...) \
  static jerry_api_value_t jerry_extension_function_ ## _function_name ## _args[_args_number] = { \
    __VA_ARGS__ \
  };
# include EXTENSION_DESCRIPTION_HEADER
#undef EXTENSION_FUNCTION
#undef EXTENSION_ARG

/* Functions description */
static jerry_extension_function_t jerry_extension_functions[JERRY_EXTENSION_FUNCTIONS_NUMBER + 1] =
{
#define EXTENSION_FUNCTION(_function_name, _function_to_call, _ret_value_type, _args_number, ...) \
  {  \
    # _function_name, jerry_extension_ ## _function_name ## _wrapper, \
    { JERRY_API_DATA_TYPE_ ## _ret_value_type, { false } }, \
    jerry_extension_function_ ## _function_name ## _args, \
     _args_number \
  },
# include EXTENSION_DESCRIPTION_HEADER
#undef EXTENSION_FUNCTION
#define EMPTY_FUNCTION_ENTRY { NULL, NULL, { JERRY_API_DATA_TYPE_VOID, { false } }, NULL, 0 }
  EMPTY_FUNCTION_ENTRY
#undef EMPTY_FUNCTION_ENTRY
};

static jerry_extension_descriptor_t jerry_extension =
{
  JERRY_EXTENSION_FIELDS_NUMBER,
  JERRY_EXTENSION_FUNCTIONS_NUMBER,
  jerry_extension_fields,
  jerry_extension_functions,
  EXTENSION_NAME,
  NULL,
  0 /* just for initialization, should be overwritten upon registration */
};
