/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "opcodes-ecma-support.h"

#include "globals.h"
#include "interpreter.h"
#include "opcodes.h"

#include "opcodes-native-call.h"

#include "actuators.h"
#include "common-io.h"
#include "sensors.h"

/**
 * 'Native call' opcode handler.
 */
ecma_completion_value_t
opfunc_native_call (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  // const idx_t dst_var_idx = opdata.data.native_call.lhs;
  const idx_t native_call_id_idx = opdata.data.native_call.name;
  const idx_t args_number = opdata.data.native_call.arg_list;

  JERRY_STATIC_ASSERT (OPCODE_NATIVE_CALL__COUNT < (1u << (sizeof (native_call_id_idx) * JERRY_BITSINBYTE)));

  ecma_value_t arg_values[args_number + 1 /* length of array should not be zero */];

  ecma_completion_value_t ret_value;

  ecma_length_t args_read;
  ecma_completion_value_t get_arg_completion = fill_varg_list (int_data,
                                                               args_number,
                                                               arg_values,
                                                               &args_read);

  if (ecma_is_completion_value_empty (get_arg_completion))
  {
    JERRY_ASSERT (args_read == args_number);

    switch ((opcode_native_call_t)native_call_id_idx)
    {
      case OPCODE_NATIVE_CALL_LED_TOGGLE:
      {
        JERRY_ASSERT (args_number == 1);
        JERRY_ASSERT (arg_values[0].value_type == ECMA_TYPE_NUMBER);
        ecma_number_t* num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_values[0].value);
        uint32_t int_num = ecma_number_to_uint32 (*num_p);
        led_toggle (int_num);

        ret_value = ecma_make_empty_completion_value ();
        break;
      }
      case OPCODE_NATIVE_CALL_LED_ON:
      {
        JERRY_ASSERT (args_number == 1);
        JERRY_ASSERT (arg_values[0].value_type == ECMA_TYPE_NUMBER);
        ecma_number_t* num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_values[0].value);
        uint32_t int_num = ecma_number_to_uint32 (*num_p);
        led_on (int_num);

        ret_value = ecma_make_empty_completion_value ();
        break;
      }
      case OPCODE_NATIVE_CALL_LED_OFF:
      {
        JERRY_ASSERT (args_number == 1);
        JERRY_ASSERT (arg_values[0].value_type == ECMA_TYPE_NUMBER);
        ecma_number_t* num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_values[0].value);
        uint32_t int_num = ecma_number_to_uint32 (*num_p);
        led_off (int_num);

        ret_value = ecma_make_empty_completion_value ();
        break;
      }
      case OPCODE_NATIVE_CALL_LED_ONCE:
      {
        JERRY_ASSERT (args_number == 1);
        JERRY_ASSERT (arg_values[0].value_type == ECMA_TYPE_NUMBER);
        ecma_number_t* num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_values[0].value);
        uint32_t int_num = ecma_number_to_uint32 (*num_p);
        led_blink_once (int_num);

        ret_value = ecma_make_empty_completion_value ();
        break;
      }
      case OPCODE_NATIVE_CALL_WAIT:
      {
        JERRY_ASSERT (args_number == 1);
        JERRY_ASSERT (arg_values[0].value_type == ECMA_TYPE_NUMBER);
        ecma_number_t* num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_values[0].value);
        uint32_t int_num = ecma_number_to_uint32 (*num_p);
        wait_ms (int_num);

        ret_value = ecma_make_empty_completion_value ();
        break;
      }

      case OPCODE_NATIVE_CALL__COUNT:
      {
        JERRY_UNREACHABLE ();
      }
    }
  }
  else
  {
    JERRY_ASSERT (!ecma_is_completion_value_normal (get_arg_completion));

    ret_value = get_arg_completion;
  }

  for (ecma_length_t arg_index = 0;
       arg_index < args_read;
       arg_index++)
  {
    ecma_free_value (arg_values[arg_index], true);
  }

  return ret_value;
} /* opfunc_native_call */
