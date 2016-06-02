/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#include "common.h"

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "lit-literal-storage.h"
#include "opcodes.h"
#include "vm.h"
#include "vm-stack.h"

#include <alloca.h>

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_executor Executor
 * @{
 */

/**
 * Top (current) interpreter context
 */
static vm_frame_ctx_t *vm_top_context_p = NULL;

/**
 * Direct call from eval;
 */
static bool is_direct_eval_form_call = false;

/**
 * Program bytecode pointer
 */
static ecma_compiled_code_t *__program = NULL;

/**
 * Get the value of object[property].
 *
 * @return ecma value
 */
static ecma_value_t
vm_op_get_value (ecma_value_t object, /**< base object */
                 ecma_value_t property) /**< property name */
{
  if (unlikely (ecma_is_value_undefined (object) || ecma_is_value_null (object)))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (""));
  }

  ecma_value_t prop_to_string_result = ecma_op_to_string (property);

  if (ecma_is_value_error (prop_to_string_result))
  {
    return prop_to_string_result;
  }

  ecma_string_t *property_name_p = ecma_get_string_from_value (prop_to_string_result);

  if (ecma_is_value_object (object))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (object);

    ecma_property_t *property_p = ecma_lcache_lookup (object_p, property_name_p);

    if (property_p != NULL &&
        ECMA_PROPERTY_GET_TYPE (property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
    {
      ecma_deref_ecma_string (property_name_p);
      return ecma_fast_copy_value (ecma_get_named_data_property_value (property_p));
    }
  }

  ecma_value_t get_value_result = ecma_op_get_value_object_base (object, property_name_p);

  ecma_deref_ecma_string (property_name_p);
  return get_value_result;
} /* vm_op_get_value */

/**
 * Set the value of object[property].
 *
 * @return ecma value
 */
static ecma_value_t
vm_op_set_value (ecma_value_t object, /**< base object */
                 ecma_value_t property, /**< property name */
                 ecma_value_t value, /**< ecma value */
                 bool is_strict) /**< strict mode */
{
  ecma_value_t completion_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (obj_val,
                  ecma_op_to_object (object),
                  completion_value);

  ECMA_TRY_CATCH (property_val,
                  ecma_op_to_string (property),
                  completion_value);

  ecma_object_t *object_p = ecma_get_object_from_value (obj_val);
  ecma_string_t *property_p = ecma_get_string_from_value (property_val);

  if (ecma_is_lexical_environment (object_p))
  {
    completion_value = ecma_op_put_value_lex_env_base (object_p,
                                                       property_p,
                                                       is_strict,
                                                       value);
  }
  else
  {
    completion_value = ecma_op_object_put (object_p,
                                           property_p,
                                           value,
                                           is_strict);
  }

  ECMA_FINALIZE (property_val);
  ECMA_FINALIZE (obj_val);

  return completion_value;
} /* vm_op_set_value */

/**
 * Initialize interpreter.
 */
void
vm_init (ecma_compiled_code_t *program_p) /**< pointer to byte-code data */
{
  JERRY_ASSERT (__program == NULL);

  __program = program_p;
} /* vm_init */

#define CBC_OPCODE(arg1, arg2, arg3, arg4) arg4,

/**
 * Decode table for both opcodes and extended opcodes.
 */
static const uint16_t vm_decode_table[] =
{
  CBC_OPCODE_LIST
  CBC_EXT_OPCODE_LIST
};

#undef CBC_OPCODE

/**
 * Run global code
 *
 * Note:
 *      returned error value should be freed with jerry_api_release_value
 *      just when the value becomes unnecessary.
 *
 * @return completion code
 */
jerry_completion_code_t
vm_run_global (ecma_value_t *error_value_p) /**< [out] error value */
{
  jerry_completion_code_t ret_code;

  JERRY_ASSERT (__program != NULL);

  ecma_object_t *glob_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);
  ecma_object_t *lex_env_p = ecma_get_global_environment ();

  ecma_value_t ret_value = vm_run (__program,
                                   ecma_make_object_value (glob_obj_p),
                                   lex_env_p,
                                   false,
                                   NULL,
                                   0);

  if (ecma_is_value_error (ret_value))
  {
    *error_value_p = ret_value;
    ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
  }
  else
  {
    ecma_free_value (ret_value);
    *error_value_p = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
    ret_code = JERRY_COMPLETION_CODE_OK;
  }

  ecma_deref_object (glob_obj_p);
  ecma_deref_object (lex_env_p);

  return ret_code;
} /* vm_run_global */

/**
 * Run specified eval-mode bytecode
 *
 * @return ecma value
 */
ecma_value_t
vm_run_eval (ecma_compiled_code_t *bytecode_data_p, /**< byte-code data */
             bool is_direct) /**< is eval called in direct mode? */
{
  bool is_strict = ((bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0);

  ecma_value_t this_binding;
  ecma_object_t *lex_env_p;

  /* ECMA-262 v5, 10.4.2 */
  if (is_direct)
  {
    this_binding = ecma_copy_value (vm_top_context_p->this_binding);
    lex_env_p = vm_top_context_p->lex_env_p;
    ecma_ref_object (vm_top_context_p->lex_env_p);
  }
  else
  {
    this_binding = ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
    lex_env_p = ecma_get_global_environment ();
  }

  if (is_strict)
  {
    ecma_object_t *strict_lex_env_p = ecma_create_decl_lex_env (lex_env_p);
    ecma_deref_object (lex_env_p);

    lex_env_p = strict_lex_env_p;
  }

  ecma_value_t completion_value = vm_run (bytecode_data_p,
                                          this_binding,
                                          lex_env_p,
                                          true,
                                          NULL,
                                          0);

  ecma_deref_object (lex_env_p);
  ecma_free_value (this_binding);
  ecma_bytecode_deref (bytecode_data_p);

  return completion_value;
} /* vm_run_eval */

/**
 * Construct object
 *
 * @return object value
 */
static ecma_value_t
vm_construct_literal_object (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                             lit_cpointer_t lit_cp) /**< literal */
{
  ecma_compiled_code_t *bytecode_p = ECMA_GET_NON_NULL_POINTER (ecma_compiled_code_t,
                                                                lit_cp);
  bool is_function = ((bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION) != 0);

  if (is_function)
  {
    bool is_strict = ((frame_ctx_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0);

    ecma_object_t *func_obj_p = ecma_op_create_function_object (frame_ctx_p->lex_env_p,
                                                                is_strict,
                                                                bytecode_p);

    return ecma_make_object_value (func_obj_p);
  }
  else
  {
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
    ecma_value_t ret_value;
    ret_value = ecma_op_create_regexp_object_from_bytecode ((re_compiled_code_t *) bytecode_p);

    if (ecma_is_value_error (ret_value))
    {
      /* TODO: throw exception instead of define an 'undefined' value. */
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }

    return ret_value;
#else /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
    JERRY_UNIMPLEMENTED ("Regular Expressions are not supported in compact profile!");
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
  }
} /* vm_construct_literal_object */

/**
 * Get implicit this value
 *
 * @return true, if the implicit 'this' value is updated,
 *         false - otherwise.
 */
static inline bool __attr_always_inline___
vm_get_implicit_this_value (ecma_value_t *this_value_p) /**< [in,out] this value */
{
  if (ecma_is_value_object (*this_value_p))
  {
    ecma_object_t *this_obj_p = ecma_get_object_from_value (*this_value_p);

    if (ecma_is_lexical_environment (this_obj_p))
    {
      ecma_value_t completion_value = ecma_op_implicit_this_value (this_obj_p);

      JERRY_ASSERT (!ecma_is_value_error (completion_value));

      *this_value_p = completion_value;
      return true;
    }
  }
  return false;
} /* vm_get_implicit_this_value */

/**
 * 'Function call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 */
static void
opfunc_call (vm_frame_ctx_t *frame_ctx_p) /**< frame context */
{
  uint8_t opcode = frame_ctx_p->byte_code_p[0];
  uint32_t arguments_list_len;

  if (opcode >= CBC_CALL0)
  {
    arguments_list_len = (unsigned int) ((opcode - CBC_CALL0) / 6);
  }
  else
  {
    arguments_list_len = frame_ctx_p->byte_code_p[1];
  }

  bool is_call_prop = ((opcode - CBC_CALL) % 6) >= 3;

  ecma_value_t this_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  ecma_value_t *stack_top_p = frame_ctx_p->stack_top_p - arguments_list_len;

  if (is_call_prop)
  {
    this_value = stack_top_p[-3];

    if (vm_get_implicit_this_value (&this_value))
    {
      ecma_free_value (stack_top_p[-3]);
      stack_top_p[-3] = this_value;
    }
  }

  ecma_value_t func_value = stack_top_p[-1];
  ecma_value_t completion_value;

  if (!ecma_op_is_callable (func_value))
  {
    completion_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ecma_object_t *func_obj_p = ecma_get_object_from_value (func_value);

    completion_value = ecma_op_function_call (func_obj_p,
                                              this_value,
                                              stack_top_p,
                                              arguments_list_len);
  }

  is_direct_eval_form_call = false;

  /* Free registers. */
  for (uint32_t i = 0; i < arguments_list_len; i++)
  {
    ecma_fast_free_value (stack_top_p[i]);
  }

  if (is_call_prop)
  {
    ecma_free_value (*(--stack_top_p));
    ecma_free_value (*(--stack_top_p));
  }

  ecma_free_value (stack_top_p[-1]);
  stack_top_p[-1] = completion_value;

  frame_ctx_p->stack_top_p = stack_top_p;
} /* opfunc_call */

/**
 * 'Constructor call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.2
 */
static void
opfunc_construct (vm_frame_ctx_t *frame_ctx_p) /**< frame context */
{
  uint8_t opcode = frame_ctx_p->byte_code_p[0];
  unsigned int arguments_list_len;

  if (opcode >= CBC_NEW0)
  {
    arguments_list_len = (unsigned int) (opcode - CBC_NEW0);
  }
  else
  {
    arguments_list_len = frame_ctx_p->byte_code_p[1];
  }

  ecma_value_t *stack_top_p = frame_ctx_p->stack_top_p - arguments_list_len;
  ecma_value_t constructor_value = stack_top_p[-1];
  ecma_value_t completion_value;

  if (!ecma_is_constructor (constructor_value))
  {
    completion_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor_value);

    completion_value = ecma_op_function_construct (constructor_obj_p,
                                                   stack_top_p,
                                                   arguments_list_len);
  }

  /* Free registers. */
  for (uint32_t i = 0; i < arguments_list_len; i++)
  {
    ecma_fast_free_value (stack_top_p[i]);
  }

  ecma_free_value (stack_top_p[-1]);
  stack_top_p[-1] = completion_value;

  frame_ctx_p->stack_top_p = stack_top_p;
} /* opfunc_construct */

#define READ_LITERAL_INDEX(destination) \
  do \
  { \
    (destination) = *byte_code_p++; \
    if ((destination) >= encoding_limit) \
    { \
      (destination) = (uint16_t) ((((destination) << 8) | *byte_code_p++) - encoding_delta); \
    } \
  } \
  while (0)

/* TODO: For performance reasons, we define this as a macro.
 * When we are able to construct a function with similar speed,
 * we can remove this macro. */
#define READ_LITERAL(literal_index, target_value) \
  do \
  { \
    if ((literal_index) < ident_end) \
    { \
      if ((literal_index) < register_end) \
      { \
        /* Note: There should be no specialization for arguments. */ \
        (target_value) = ecma_fast_copy_value (frame_ctx_p->registers_p[literal_index]); \
      } \
      else \
      { \
        ecma_string_t *name_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]); \
        ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p, \
                                                                            name_p); \
        if (ref_base_lex_env_p != NULL) \
        { \
          last_completion_value = ecma_op_get_value_lex_env_base (ref_base_lex_env_p, \
                                                                  name_p, \
                                                                  is_strict); \
        } \
        else \
        { \
          last_completion_value = ecma_raise_reference_error (ECMA_ERR_MSG ("")); \
        } \
        \
        ecma_deref_ecma_string (name_p); \
        \
        if (ecma_is_value_error (last_completion_value)) \
        { \
          goto error; \
        } \
        (target_value) = last_completion_value; \
      } \
    } \
    else if (literal_index < const_literal_end) \
    { \
      lit_cpointer_t lit_cpointer = literal_start_p[literal_index]; \
      lit_literal_t lit = lit_cpointer_decompress (lit_cpointer); \
      if (unlikely (LIT_RECORD_IS_NUMBER (lit))) \
      { \
        ecma_number_t number = lit_number_literal_get_number (lit); \
        (target_value) = ecma_make_number_value (number); \
      } \
      else \
      { \
        ecma_string_t *string_p = ecma_new_ecma_string_from_lit_cp (lit_cpointer); \
        (target_value) = ecma_make_string_value (string_p); \
      } \
    } \
    else \
    { \
      /* Object construction. */ \
      (target_value) = vm_construct_literal_object (frame_ctx_p, literal_start_p[literal_index]); \
    } \
  } \
  while (0)

/**
 * Cleanup interpreter
 */
void
vm_finalize (void)
{
  if (__program)
  {
    ecma_bytecode_deref (__program);
  }

  __program = NULL;
} /* vm_finalize */

/**
 * Run initializer byte codes.
 *
 * @return ecma value
 */
static ecma_value_t
vm_init_loop (vm_frame_ctx_t *frame_ctx_p) /**< frame context */
{
  const ecma_compiled_code_t *bytecode_header_p = frame_ctx_p->bytecode_header_p;
  uint8_t *byte_code_p = frame_ctx_p->byte_code_p;
  ecma_value_t last_completion_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  uint16_t encoding_limit;
  uint16_t encoding_delta;
  uint16_t register_end;
  lit_cpointer_t *literal_start_p = frame_ctx_p->literal_start_p;
  bool is_strict = ((frame_ctx_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0);

  /* Prepare. */
  if (!(bytecode_header_p->status_flags & CBC_CODE_FLAGS_FULL_LITERAL_ENCODING))
  {
    encoding_limit = 255;
    encoding_delta = 0xfe01;
  }
  else
  {
    encoding_limit = 128;
    encoding_delta = 0x8000;
  }

  if (frame_ctx_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) (frame_ctx_p->bytecode_header_p);
    register_end = args_p->register_end;
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) (frame_ctx_p->bytecode_header_p);
    register_end = args_p->register_end;
  }

  while (true)
  {
    switch (*byte_code_p)
    {
      case CBC_DEFINE_VARS:
      {
        uint32_t literal_index_end;
        uint32_t literal_index = register_end;

        byte_code_p++;
        READ_LITERAL_INDEX (literal_index_end);

        while (literal_index <= literal_index_end)
        {
          ecma_string_t *name_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]);
          vm_var_decl (frame_ctx_p, name_p);
          ecma_deref_ecma_string (name_p);
          literal_index++;
        }
        break;
      }

      case CBC_INITIALIZE_VAR:
      case CBC_INITIALIZE_VARS:
      {
        uint8_t type = *byte_code_p;
        uint32_t literal_index;
        uint32_t literal_index_end;

        byte_code_p++;
        READ_LITERAL_INDEX (literal_index);

        if (type == CBC_INITIALIZE_VAR)
        {
          literal_index_end = literal_index;
        }
        else
        {
          READ_LITERAL_INDEX (literal_index_end);
        }

        while (literal_index <= literal_index_end)
        {
          uint32_t value_index;
          ecma_value_t lit_value;
          ecma_string_t *name_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]);

          vm_var_decl (frame_ctx_p, name_p);

          READ_LITERAL_INDEX (value_index);

          ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p, name_p);

          if (value_index < register_end)
          {
            lit_value = frame_ctx_p->registers_p[value_index];
          }
          else
          {
            lit_value = vm_construct_literal_object (frame_ctx_p,
                                                     literal_start_p[value_index]);
          }

          /* TODO: check the return value */
          ecma_op_put_value_lex_env_base (ref_base_lex_env_p,
                                          name_p,
                                          is_strict,
                                          lit_value);

          if (value_index >= register_end)
          {
            ecma_free_value (lit_value);
          }

          ecma_deref_ecma_string (name_p);
          literal_index++;
        }
        break;
      }

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
      case CBC_SET_BYTECODE_PTR:
      {
        memcpy (&byte_code_p, byte_code_p + 1, sizeof (uint8_t *));
        frame_ctx_p->byte_code_start_p = byte_code_p;
        break;
      }
#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */

      default:
      {
        frame_ctx_p->byte_code_p = byte_code_p;
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      }
    }
  }

  return last_completion_value;
} /* vm_init_loop */

/**
 * Run generic byte code.
 *
 * @return ecma value
 */
static ecma_value_t __attr_noinline___
vm_loop (vm_frame_ctx_t *frame_ctx_p) /**< frame context */
{
  const ecma_compiled_code_t *bytecode_header_p = frame_ctx_p->bytecode_header_p;
  uint8_t *byte_code_p = frame_ctx_p->byte_code_p;
  lit_cpointer_t *literal_start_p = frame_ctx_p->literal_start_p;
  ecma_value_t last_completion_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_value_t *stack_top_p;
  uint16_t encoding_limit;
  uint16_t encoding_delta;
  uint16_t register_end;
  uint16_t ident_end;
  uint16_t const_literal_end;
  int32_t branch_offset = 0;
  ecma_value_t left_value;
  ecma_value_t right_value;
  ecma_value_t result = 0;
  ecma_value_t block_result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  bool is_strict = ((frame_ctx_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0);

  /* Prepare for byte code execution. */
  if (!(bytecode_header_p->status_flags & CBC_CODE_FLAGS_FULL_LITERAL_ENCODING))
  {
    encoding_limit = 255;
    encoding_delta = 0xfe01;
  }
  else
  {
    encoding_limit = 128;
    encoding_delta = 0x8000;
  }

  if (bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) (bytecode_header_p);
    register_end = args_p->register_end;
    ident_end = args_p->ident_end;
    const_literal_end = args_p->const_literal_end;
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) (bytecode_header_p);
    register_end = args_p->register_end;
    ident_end = args_p->ident_end;
    const_literal_end = args_p->const_literal_end;
  }

  stack_top_p = frame_ctx_p->stack_top_p;

  /* Outer loop for exception handling. */
  while (true)
  {
    /* Internal loop for byte code execution. */
    while (true)
    {
      uint8_t *byte_code_start_p = byte_code_p;
      uint8_t opcode;
      uint32_t opcode_data;

      opcode = *byte_code_p++;
      opcode_data = opcode;
      if (opcode == CBC_EXT_OPCODE)
      {
        opcode = *byte_code_p++;
        opcode_data = (uint32_t) ((CBC_END + 1) + opcode);
      }

      opcode_data = vm_decode_table[opcode_data];

      if (opcode_data & VM_OC_HAS_BRANCH_ARG)
      {
        branch_offset = 0;
        switch (CBC_BRANCH_OFFSET_LENGTH (opcode))
        {
          case 3:
          {
            branch_offset = *(byte_code_p++);
            /* FALLTHRU */
          }
          case 2:
          {
            branch_offset <<= 8;
            branch_offset |= *(byte_code_p++);
            /* FALLTHRU */
          }
          default:
          {
            JERRY_ASSERT (CBC_BRANCH_OFFSET_LENGTH (opcode) > 0);
            branch_offset <<= 8;
            branch_offset |= *(byte_code_p++);
            break;
          }
        }

        if (opcode_data & VM_OC_BACKWARD_BRANCH)
        {
          branch_offset = -branch_offset;
        }
      }

      left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      right_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

      if (VM_OC_HAS_GET_ARGS (opcode_data))
      {
        uint32_t operands = VM_OC_GET_ARGS_INDEX (opcode_data);

        if (operands >= VM_OC_GET_LITERAL)
        {
          uint16_t literal_index;
          READ_LITERAL_INDEX (literal_index);
          READ_LITERAL (literal_index,
                        left_value);

          if (operands != VM_OC_GET_LITERAL)
          {
            switch (operands)
            {
              case VM_OC_GET_LITERAL_LITERAL:
              {
                uint16_t literal_index;
                READ_LITERAL_INDEX (literal_index);
                READ_LITERAL (literal_index,
                              right_value);
                break;
              }
              case VM_OC_GET_STACK_LITERAL:
              {
                JERRY_ASSERT (stack_top_p > frame_ctx_p->registers_p + register_end);
                right_value = left_value;
                left_value = *(--stack_top_p);
                break;
              }
              default:
              {
                JERRY_ASSERT (operands == VM_OC_GET_THIS_LITERAL);
                right_value = left_value;
                left_value = ecma_copy_value (frame_ctx_p->this_binding);
                break;
              }
            }
          }
        }
        else
        {
          JERRY_ASSERT (operands == VM_OC_GET_STACK
                        || operands == VM_OC_GET_STACK_STACK);

          JERRY_ASSERT (stack_top_p > frame_ctx_p->registers_p + register_end);
          left_value = *(--stack_top_p);

          if (operands == VM_OC_GET_STACK_STACK)
          {
            JERRY_ASSERT (stack_top_p > frame_ctx_p->registers_p + register_end);
            right_value = left_value;
            left_value = *(--stack_top_p);
          }
        }
      }

      switch (VM_OC_GROUP_GET_INDEX (opcode_data))
      {
        case VM_OC_NONE:
        {
          JERRY_ASSERT (opcode == CBC_EXT_DEBUGGER);
          break;
        }
        case VM_OC_POP:
        {
          JERRY_ASSERT (stack_top_p > frame_ctx_p->registers_p + register_end);
          ecma_free_value (*(--stack_top_p));
          continue;
        }
        case VM_OC_POP_BLOCK:
        {
          result = *(--stack_top_p);
          break;
        }
        case VM_OC_PUSH:
        {
          *(stack_top_p++) = left_value;
          continue;
        }
        case VM_OC_PUSH_TWO:
        {
          *(stack_top_p++) = left_value;
          *(stack_top_p++) = right_value;
          continue;
        }
        case VM_OC_PUSH_THREE:
        {
          uint16_t literal_index;

          *(stack_top_p++) = left_value;
          left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

          READ_LITERAL_INDEX (literal_index);
          READ_LITERAL (literal_index,
                        left_value);

          *(stack_top_p++) = right_value;
          *(stack_top_p++) = left_value;
          continue;
        }
        case VM_OC_PUSH_UNDEFINED:
        case VM_OC_VOID:
        {
          result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          break;
        }
        case VM_OC_PUSH_TRUE:
        {
          result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
          break;
        }
        case VM_OC_PUSH_FALSE:
        {
          result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
          break;
        }
        case VM_OC_PUSH_NULL:
        {
          result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
          break;
        }
        case VM_OC_PUSH_THIS:
        {
          result = ecma_copy_value (frame_ctx_p->this_binding);
          break;
        }
        case VM_OC_PUSH_NUMBER:
        {
          ecma_integer_value_t number;

          if (opcode == CBC_PUSH_NUMBER_0)
          {
            number = 0;
          }
          else
          {
            number = *byte_code_p++;

            JERRY_ASSERT (opcode == CBC_PUSH_NUMBER_1);

            if (number >= CBC_PUSH_NUMBER_1_RANGE_END)
            {
              number = -(number - CBC_PUSH_NUMBER_1_RANGE_END);
            }
          }

          result = ecma_make_integer_value (number);
          break;
        }
        case VM_OC_PUSH_OBJECT:
        {
          ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
          ecma_object_t *obj_p = ecma_create_object (prototype_p,
                                                     true,
                                                     ECMA_OBJECT_TYPE_GENERAL);
          result = ecma_make_object_value (obj_p);
          ecma_deref_object (prototype_p);
          break;
        }
        case VM_OC_SET_PROPERTY:
        {
          ecma_object_t *object_p = ecma_get_object_from_value (stack_top_p[-1]);
          ecma_string_t *prop_name_p;
          ecma_property_t *property_p;

          if (ecma_is_value_string (right_value))
          {
            prop_name_p = ecma_get_string_from_value (right_value);
            property_p = ecma_find_named_property (object_p, prop_name_p);
          }
          else
          {
            last_completion_value = ecma_op_to_string (right_value);

            if (ecma_is_value_error (last_completion_value))
            {
              goto error;
            }

            prop_name_p = ecma_get_string_from_value (last_completion_value);
            property_p = ecma_find_named_property (object_p, prop_name_p);
          }

          if (property_p != NULL && ECMA_PROPERTY_GET_TYPE (property_p) != ECMA_PROPERTY_TYPE_NAMEDDATA)
          {
            ecma_delete_property (object_p, property_p);
            property_p = NULL;
          }

          if (property_p == NULL)
          {
            property_p = ecma_create_named_data_property (object_p,
                                                          prop_name_p,
                                                          ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
          }

          ecma_named_data_property_assign_value (object_p, property_p, left_value);

          if (!ecma_is_value_string (right_value))
          {
            ecma_deref_ecma_string (prop_name_p);
          }
          break;
        }
        case VM_OC_SET_GETTER:
        case VM_OC_SET_SETTER:
        {
          opfunc_set_accessor (VM_OC_GROUP_GET_INDEX (opcode_data) == VM_OC_SET_GETTER ? true : false,
                               stack_top_p[-1],
                               left_value,
                               right_value);
          break;
        }
        case VM_OC_PUSH_ARRAY:
        {
          last_completion_value = ecma_op_create_array_object (NULL, 0, false);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }
          result = last_completion_value;
          break;
        }
        case VM_OC_PUSH_ELISON:
        {
          result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_ARRAY_HOLE);
          break;
        }
        case VM_OC_APPEND_ARRAY:
        {
          ecma_object_t *array_obj_p;
          ecma_string_t *length_str_p;
          ecma_property_t *length_prop_p;
          uint32_t length_num;
          uint32_t values_length = *byte_code_p++;

          stack_top_p -= values_length;

          array_obj_p = ecma_get_object_from_value (stack_top_p[-1]);
          length_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
          length_prop_p = ecma_get_named_property (array_obj_p, length_str_p);

          JERRY_ASSERT (length_prop_p != NULL);

          left_value = ecma_get_named_data_property_value (length_prop_p);
          length_num = ecma_get_uint32_from_value (left_value);

          ecma_deref_ecma_string (length_str_p);

          for (uint32_t i = 0; i < values_length; i++)
          {
            if (!ecma_is_value_array_hole (stack_top_p[i]))
            {
              ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (length_num);

              ecma_property_t *prop_p;
              prop_p = ecma_create_named_data_property (array_obj_p,
                                                        index_str_p,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

              JERRY_ASSERT (ecma_is_value_undefined (ecma_get_named_data_property_value (prop_p)));

              ecma_set_named_data_property_value (prop_p, stack_top_p[i]);

              /* The reference is moved so no need to free stack_top_p[i] except for objects. */
              if (ecma_is_value_object (stack_top_p[i]))
              {
                ecma_free_value (stack_top_p[i]);
              }

              ecma_deref_ecma_string (index_str_p);
            }

            length_num++;
          }

          ecma_value_assign_uint32 (&ECMA_PROPERTY_VALUE_PTR (length_prop_p)->value,
                                    length_num);
          break;
        }
        case VM_OC_PUSH_UNDEFINED_BASE:
        {
          result = stack_top_p[-1];
          stack_top_p[-1] = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          break;
        }
        case VM_OC_IDENT_REFERENCE:
        {
          uint16_t literal_index;

          READ_LITERAL_INDEX (literal_index);

          JERRY_ASSERT (literal_index < ident_end);

          if (literal_index < register_end)
          {
            *stack_top_p++ = ecma_make_simple_value (ECMA_SIMPLE_VALUE_REGISTER_REF);
            *stack_top_p++ = literal_index;
            result = ecma_copy_value (frame_ctx_p->registers_p[literal_index]);
          }
          else
          {
            ecma_string_t *name_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]);
            ecma_object_t *ref_base_lex_env_p;

            ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p,
                                                                 name_p);

            if (ref_base_lex_env_p != NULL)
            {
              last_completion_value = ecma_op_get_value_lex_env_base (ref_base_lex_env_p,
                                                                      name_p,
                                                                      is_strict);
            }
            else
            {
              last_completion_value = ecma_raise_reference_error (ECMA_ERR_MSG (""));
            }

            if (ecma_is_value_error (last_completion_value))
            {
              ecma_deref_ecma_string (name_p);
              goto error;
            }

            ecma_ref_object (ref_base_lex_env_p);
            *stack_top_p++ = ecma_make_object_value (ref_base_lex_env_p);
            *stack_top_p++ = ecma_make_string_value (name_p);

            result = last_completion_value;
          }
          break;
        }
        case VM_OC_PROP_REFERENCE:
        {
          /* Forms with reference requires preserving the base and offset. */

          if (opcode == CBC_PUSH_PROP_REFERENCE)
          {
            left_value = stack_top_p[-2];
            right_value = stack_top_p[-1];
          }
          else if (opcode == CBC_PUSH_PROP_LITERAL_REFERENCE)
          {
            *stack_top_p++ = left_value;
            right_value = left_value;
            left_value = stack_top_p[-2];
          }
          else
          {
            JERRY_ASSERT (opcode == CBC_PUSH_PROP_LITERAL_LITERAL_REFERENCE
                          || opcode == CBC_PUSH_PROP_THIS_LITERAL_REFERENCE);
            *stack_top_p++ = left_value;
            *stack_top_p++ = right_value;
          }
          /* FALLTHRU */
        }
        case VM_OC_PROP_GET:
        case VM_OC_PROP_PRE_INCR:
        case VM_OC_PROP_PRE_DECR:
        case VM_OC_PROP_POST_INCR:
        case VM_OC_PROP_POST_DECR:
        {
          last_completion_value = vm_op_get_value (left_value,
                                                   right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            if (opcode >= CBC_PUSH_PROP_REFERENCE && opcode < CBC_PRE_INCR)
            {
              left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
              right_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
            }
            goto error;
          }

          result = last_completion_value;

          if (opcode < CBC_PRE_INCR)
          {
            if (opcode >= CBC_PUSH_PROP_REFERENCE)
            {
              left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
              right_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
            }
            break;
          }

          stack_top_p += 2;
          left_value = result;
          right_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          /* FALLTHRU */
        }
        case VM_OC_PRE_INCR:
        case VM_OC_PRE_DECR:
        case VM_OC_POST_INCR:
        case VM_OC_POST_DECR:
        {
          uint32_t opcode_flags = VM_OC_GROUP_GET_INDEX (opcode_data) - VM_OC_PROP_PRE_INCR;

          last_completion_value = ecma_op_to_number (left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          byte_code_p = byte_code_start_p + 1;
          result = last_completion_value;

          if (ecma_is_value_integer_number (result))
          {
            ecma_integer_value_t int_value = (ecma_integer_value_t) result;
            ecma_integer_value_t int_increase;

            if (opcode_flags & VM_OC_DECREMENT_OPERATOR_FLAG)
            {
              if (int_value <= (ECMA_INTEGER_NUMBER_MIN << ECMA_DIRECT_SHIFT))
              {
                int_increase = 0;
              }
              else
              {
                int_increase = -(1 << ECMA_DIRECT_SHIFT);
              }
            }
            else
            {
              if (int_value >= (ECMA_INTEGER_NUMBER_MAX << ECMA_DIRECT_SHIFT))
              {
                int_increase = 0;
              }
              else
              {
                int_increase = 1 << ECMA_DIRECT_SHIFT;
              }
            }

            if (int_increase != 0)
            {
              /* Postfix operators require the unmodifed number value. */
              if (opcode_flags & VM_OC_POST_INCR_DECR_OPERATOR_FLAG)
              {
                if (opcode_data & VM_OC_PUT_STACK)
                {
                  if (opcode_flags & VM_OC_IDENT_INCR_DECR_OPERATOR_FLAG)
                  {
                    JERRY_ASSERT (opcode == CBC_POST_INCR_IDENT_PUSH_RESULT
                                  || opcode == CBC_POST_DECR_IDENT_PUSH_RESULT);

                    *stack_top_p++ = result;
                  }
                  else
                  {
                    /* The parser ensures there is enough space for the
                     * extra value on the stack. See js-parser-expr.c. */

                    JERRY_ASSERT (opcode == CBC_POST_INCR_PUSH_RESULT
                                  || opcode == CBC_POST_DECR_PUSH_RESULT);

                    stack_top_p++;
                    stack_top_p[-1] = stack_top_p[-2];
                    stack_top_p[-2] = stack_top_p[-3];
                    stack_top_p[-3] = result;
                  }
                  opcode_data &= (uint32_t)~VM_OC_PUT_STACK;
                }
                else if (opcode_data & VM_OC_PUT_BLOCK)
                {
                  ecma_free_value (block_result);
                  block_result = result;
                  opcode_data &= (uint32_t) ~VM_OC_PUT_BLOCK;
                }
              }

              result = (ecma_value_t) (int_value + int_increase);
              break;
            }
          }

          ecma_number_t increase = ECMA_NUMBER_ONE;
          ecma_number_t result_number = ecma_get_number_from_value (result);

          if (opcode_flags & VM_OC_DECREMENT_OPERATOR_FLAG)
          {
            /* For decrement operators */
            increase = ECMA_NUMBER_MINUS_ONE;
          }

          /* Post operators require the unmodifed number value. */
          if (opcode_flags & VM_OC_POST_INCR_DECR_OPERATOR_FLAG)
          {
            if (opcode_data & VM_OC_PUT_STACK)
            {
              if (opcode_flags & VM_OC_IDENT_INCR_DECR_OPERATOR_FLAG)
              {
                JERRY_ASSERT (opcode == CBC_POST_INCR_IDENT_PUSH_RESULT
                              || opcode == CBC_POST_DECR_IDENT_PUSH_RESULT);

                *stack_top_p++ = ecma_copy_value (result);
              }
              else
              {
                /* The parser ensures there is enough space for the
                 * extra value on the stack. See js-parser-expr.c. */

                JERRY_ASSERT (opcode == CBC_POST_INCR_PUSH_RESULT
                              || opcode == CBC_POST_DECR_PUSH_RESULT);

                stack_top_p++;
                stack_top_p[-1] = stack_top_p[-2];
                stack_top_p[-2] = stack_top_p[-3];
                stack_top_p[-3] = ecma_copy_value (result);
              }
              opcode_data &= (uint32_t)~VM_OC_PUT_STACK;
            }
            else if (opcode_data & VM_OC_PUT_BLOCK)
            {
              ecma_free_value (block_result);
              block_result = ecma_copy_value (result);
              opcode_data &= (uint32_t) ~VM_OC_PUT_BLOCK;
            }
          }

          ecma_value_assign_number (&result, result_number + increase);
          break;
        }
        case VM_OC_ASSIGN:
        {
          result = left_value;
          left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          break;
        }
        case VM_OC_ASSIGN_PROP:
        {
          result = stack_top_p[-1];
          stack_top_p[-1] = left_value;
          left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          break;
        }
        case VM_OC_ASSIGN_PROP_THIS:
        {
          result = stack_top_p[-1];
          stack_top_p[-1] = ecma_copy_value (frame_ctx_p->this_binding);
          *stack_top_p++ = left_value;
          left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          break;
        }
        case VM_OC_RET:
        {
          JERRY_ASSERT (opcode == CBC_RETURN
                        || opcode == CBC_RETURN_WITH_BLOCK
                        || opcode == CBC_RETURN_WITH_LITERAL);

          if (opcode == CBC_RETURN_WITH_BLOCK)
          {
            left_value = block_result;
            block_result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          }

          last_completion_value = left_value;
          left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          goto error;
        }
        case VM_OC_THROW:
        {
          last_completion_value = ecma_make_error_value (left_value);
          left_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          goto error;
        }
        case VM_OC_THROW_REFERENCE_ERROR:
        {
          last_completion_value = ecma_raise_reference_error (ECMA_ERR_MSG (""));
          goto error;
        }
        case VM_OC_EVAL:
        {
          is_direct_eval_form_call = true;
          JERRY_ASSERT (*byte_code_p >= CBC_CALL && *byte_code_p <= CBC_CALL2_PROP_BLOCK);
          continue;
        }
        case VM_OC_CALL:
        {
          if (frame_ctx_p->call_operation == VM_NO_EXEC_OP)
          {
            frame_ctx_p->call_operation = VM_EXEC_CALL;
            frame_ctx_p->byte_code_p = byte_code_start_p;
            frame_ctx_p->stack_top_p = stack_top_p;
            frame_ctx_p->call_block_result = block_result;
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          }

          if (opcode < CBC_CALL0)
          {
            byte_code_p++;
          }

          frame_ctx_p->call_operation = VM_NO_EXEC_OP;

          last_completion_value = *(--stack_top_p);
          block_result = frame_ctx_p->call_block_result;

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          if (VM_OC_HAS_PUT_RESULT (opcode_data))
          {
            result = last_completion_value;
          }
          else
          {
            left_value = last_completion_value;
          }
          break;
        }
        case VM_OC_NEW:
        {
          if (frame_ctx_p->call_operation == VM_NO_EXEC_OP)
          {
            frame_ctx_p->call_operation = VM_EXEC_CONSTRUCT;
            frame_ctx_p->byte_code_p = byte_code_start_p;
            frame_ctx_p->stack_top_p = stack_top_p;
            frame_ctx_p->call_block_result = block_result;
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          }

          if (opcode < CBC_NEW0)
          {
            byte_code_p++;
          }

          frame_ctx_p->call_operation = VM_NO_EXEC_OP;

          last_completion_value = *(--stack_top_p);
          block_result = frame_ctx_p->call_block_result;

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          JERRY_ASSERT (VM_OC_HAS_PUT_RESULT (opcode_data));
          result = last_completion_value;
          break;
        }
        case VM_OC_PROP_DELETE:
        {
          last_completion_value = vm_op_delete_prop (left_value, right_value, is_strict);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_DELETE:
        {
          uint16_t literal_index;

          READ_LITERAL_INDEX (literal_index);

          if (literal_index < register_end)
          {
            result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
            break;
          }

          last_completion_value = vm_op_delete_var (literal_start_p[literal_index],
                                                    frame_ctx_p->lex_env_p,
                                                    is_strict);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_JUMP:
        {
          byte_code_p = byte_code_start_p + branch_offset;
          break;
        }
        case VM_OC_BRANCH_IF_STRICT_EQUAL:
        {
          JERRY_ASSERT (stack_top_p > frame_ctx_p->registers_p + register_end);

          last_completion_value = opfunc_equal_value_type (left_value,
                                                           stack_top_p[-1]);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;

          if (result == ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE))
          {
            byte_code_p = byte_code_start_p + branch_offset;
            ecma_free_value (*--stack_top_p);
          }
          break;
        }
        case VM_OC_BRANCH_IF_TRUE:
        case VM_OC_BRANCH_IF_FALSE:
        case VM_OC_BRANCH_IF_LOGICAL_TRUE:
        case VM_OC_BRANCH_IF_LOGICAL_FALSE:
        {
          uint32_t opcode_flags = VM_OC_GROUP_GET_INDEX (opcode_data) - VM_OC_BRANCH_IF_TRUE;

          bool boolean_value = ecma_op_to_boolean (left_value);

          if (opcode_flags & VM_OC_BRANCH_IF_FALSE_FLAG)
          {
            boolean_value = !boolean_value;
          }

          if (boolean_value)
          {
            byte_code_p = byte_code_start_p + branch_offset;
            if (opcode_flags & VM_OC_LOGICAL_BRANCH_FLAG)
            {
              /* "Push" the left_value back to the stack. */
              ++stack_top_p;
              continue;
            }
          }

          ecma_fast_free_value (left_value);
          continue;
        }
        case VM_OC_PLUS:
        {
          last_completion_value = opfunc_unary_plus (left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_MINUS:
        {
          last_completion_value = opfunc_unary_minus (left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_NOT:
        {
          last_completion_value = opfunc_logical_not (left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_BIT_NOT:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_NOT,
                                                           left_value,
                                                           left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_TYPEOF_IDENT:
        {
          uint16_t literal_index;

          READ_LITERAL_INDEX (literal_index);

          JERRY_ASSERT (literal_index < ident_end);

          if (literal_index < register_end)
          {
            left_value = ecma_copy_value (frame_ctx_p->registers_p[literal_index]);
          }
          else
          {
            ecma_string_t *name_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]);
            ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p,
                                                                                name_p);

            if (ref_base_lex_env_p == NULL)
            {
              ecma_deref_ecma_string (name_p);

              ecma_string_t *string_p = ecma_get_magic_string (LIT_MAGIC_STRING_UNDEFINED);
              result = ecma_make_string_value (string_p);
              break;
            }

            last_completion_value = ecma_op_get_value_lex_env_base (ref_base_lex_env_p,
                                                                    name_p,
                                                                    is_strict);

            ecma_deref_ecma_string (name_p);

            if (ecma_is_value_error (last_completion_value))
            {
              goto error;
            }

            left_value = last_completion_value;
          }
          /* FALLTHRU */
        }
        case VM_OC_TYPEOF:
        {
          last_completion_value = opfunc_typeof (left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_ADD:
        {
          last_completion_value = opfunc_addition (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_SUB:
        {
          last_completion_value = do_number_arithmetic (NUMBER_ARITHMETIC_SUBSTRACTION,
                                                        left_value,
                                                        right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_MUL:
        {
          last_completion_value = do_number_arithmetic (NUMBER_ARITHMETIC_MULTIPLICATION,
                                                        left_value,
                                                        right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_DIV:
        {
          last_completion_value = do_number_arithmetic (NUMBER_ARITHMETIC_DIVISION,
                                                        left_value,
                                                        right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_MOD:
        {
          last_completion_value = do_number_arithmetic (NUMBER_ARITHMETIC_REMAINDER,
                                                        left_value,
                                                        right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_EQUAL:
        {
          last_completion_value = opfunc_equal_value (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_NOT_EQUAL:
        {
          last_completion_value = opfunc_not_equal_value (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_STRICT_EQUAL:
        {
          last_completion_value = opfunc_equal_value_type (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_STRICT_NOT_EQUAL:
        {
          last_completion_value = opfunc_not_equal_value_type (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_BIT_OR:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_LOGIC_OR,
                                                           left_value,
                                                           right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_BIT_XOR:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_LOGIC_XOR,
                                                           left_value,
                                                           right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_BIT_AND:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_LOGIC_AND,
                                                           left_value,
                                                           right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_LEFT_SHIFT:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_SHIFT_LEFT,
                                                           left_value,
                                                           right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_RIGHT_SHIFT:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_SHIFT_RIGHT,
                                                           left_value,
                                                           right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_UNS_RIGHT_SHIFT:
        {
          last_completion_value = do_number_bitwise_logic (NUMBER_BITWISE_SHIFT_URIGHT,
                                                           left_value,
                                                           right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_LESS:
        {
          last_completion_value = opfunc_less_than (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_GREATER:
        {
          last_completion_value = opfunc_greater_than (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_LESS_EQUAL:
        {
          last_completion_value = opfunc_less_or_equal_than (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_GREATER_EQUAL:
        {
          last_completion_value = opfunc_greater_or_equal_than (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_IN:
        {
          last_completion_value = opfunc_in (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_INSTANCEOF:
        {
          last_completion_value = opfunc_instanceof (left_value, right_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          result = last_completion_value;
          break;
        }
        case VM_OC_WITH:
        {
          ecma_object_t *object_p;
          ecma_object_t *with_env_p;

          branch_offset += (int32_t) (byte_code_start_p - frame_ctx_p->byte_code_start_p);

          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          last_completion_value = ecma_op_to_object (left_value);

          if (ecma_is_value_error (last_completion_value))
          {
            goto error;
          }

          object_p = ecma_get_object_from_value (last_completion_value);

          with_env_p = ecma_create_object_lex_env (frame_ctx_p->lex_env_p,
                                                   object_p,
                                                   true);

          ecma_deref_object (object_p);

          VM_PLUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_WITH_CONTEXT_STACK_ALLOCATION);
          stack_top_p += PARSER_WITH_CONTEXT_STACK_ALLOCATION;

          stack_top_p[-1] = VM_CREATE_CONTEXT (VM_CONTEXT_WITH, branch_offset);
          stack_top_p[-2] = ecma_make_object_value (frame_ctx_p->lex_env_p);

          frame_ctx_p->lex_env_p = with_env_p;
          break;
        }
        case VM_OC_FOR_IN_CREATE_CONTEXT:
        {
          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          ecma_value_t expr_obj_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          ecma_collection_header_t *header_p = opfunc_for_in (left_value, &expr_obj_value);

          if (header_p == NULL)
          {
            byte_code_p = byte_code_start_p + branch_offset;
            break;
          }

          branch_offset += (int32_t) (byte_code_start_p - frame_ctx_p->byte_code_start_p);

          VM_PLUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION);
          stack_top_p += PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION;
          stack_top_p[-1] = (ecma_value_t) VM_CREATE_CONTEXT (VM_CONTEXT_FOR_IN, branch_offset);
          stack_top_p[-2] = header_p->first_chunk_cp;
          stack_top_p[-3] = expr_obj_value;

          ecma_dealloc_collection_header (header_p);
          break;
        }
        case VM_OC_FOR_IN_GET_NEXT:
        {
          ecma_value_t *context_top_p = frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth;
          ecma_collection_chunk_t *chunk_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_collection_chunk_t, context_top_p[-2]);

          JERRY_ASSERT (VM_GET_CONTEXT_TYPE (context_top_p[-1]) == VM_CONTEXT_FOR_IN);

          lit_utf8_byte_t *data_ptr = chunk_p->data;
          result = *(ecma_value_t *) data_ptr;
          context_top_p[-2] = chunk_p->next_chunk_cp;

          ecma_dealloc_collection_chunk (chunk_p);
          break;
        }
        case VM_OC_FOR_IN_HAS_NEXT:
        {
          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          while (true)
          {
            if (stack_top_p[-2] == JMEM_CP_NULL)
            {
              ecma_free_value (stack_top_p[-3]);

              VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION);
              stack_top_p -= PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION;
              break;
            }

            ecma_collection_chunk_t *chunk_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_collection_chunk_t, stack_top_p[-2]);

            lit_utf8_byte_t *data_ptr = chunk_p->data;
            ecma_string_t *prop_name_p = ecma_get_string_from_value (*(ecma_value_t *) data_ptr);

            if (ecma_op_object_get_property (ecma_get_object_from_value (stack_top_p[-3]),
                                             prop_name_p) == NULL)
            {
              stack_top_p[-2] = chunk_p->next_chunk_cp;
              ecma_deref_ecma_string (prop_name_p);
              ecma_dealloc_collection_chunk (chunk_p);
            }
            else
            {
              byte_code_p = byte_code_start_p + branch_offset;
              break;
            }
          }

          break;
        }
        case VM_OC_TRY:
        {
          /* Try opcode simply creates the try context. */
          branch_offset += (int32_t) (byte_code_start_p - frame_ctx_p->byte_code_start_p);

          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          VM_PLUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
          stack_top_p += PARSER_TRY_CONTEXT_STACK_ALLOCATION;

          stack_top_p[-1] = (ecma_value_t) VM_CREATE_CONTEXT (VM_CONTEXT_TRY, branch_offset);
          break;
        }
        case VM_OC_CATCH:
        {
          /* Catches are ignored and turned to jumps. */
          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);
          JERRY_ASSERT (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_TRY);

          byte_code_p = byte_code_start_p + branch_offset;
          break;
        }
        case VM_OC_FINALLY:
        {
          branch_offset += (int32_t) (byte_code_start_p - frame_ctx_p->byte_code_start_p);

          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          JERRY_ASSERT (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_TRY
                        || VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_CATCH);

          if (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_CATCH)
          {
            ecma_deref_object (frame_ctx_p->lex_env_p);
            frame_ctx_p->lex_env_p = ecma_get_object_from_value (stack_top_p[-2]);
          }

          stack_top_p[-1] = (ecma_value_t) VM_CREATE_CONTEXT (VM_CONTEXT_FINALLY_JUMP, branch_offset);
          stack_top_p[-2] = (ecma_value_t) branch_offset;
          break;
        }
        case VM_OC_CONTEXT_END:
        {
          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          switch (VM_GET_CONTEXT_TYPE (stack_top_p[-1]))
          {
            case VM_CONTEXT_FINALLY_JUMP:
            {
              uint32_t jump_target = stack_top_p[-2];

              VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth,
                                  PARSER_TRY_CONTEXT_STACK_ALLOCATION);
              stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;

              if (vm_stack_find_finally (frame_ctx_p,
                                         &stack_top_p,
                                         VM_CONTEXT_FINALLY_JUMP,
                                         jump_target))
              {
                JERRY_ASSERT (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_FINALLY_JUMP);
                byte_code_p = frame_ctx_p->byte_code_p;
                stack_top_p[-2] = jump_target;
              }
              else
              {
                byte_code_p = frame_ctx_p->byte_code_start_p + jump_target;
              }
              break;
            }
            case VM_CONTEXT_FINALLY_THROW:
            {
              last_completion_value = stack_top_p[-2];

              VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth,
                                  PARSER_TRY_CONTEXT_STACK_ALLOCATION);
              stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
              goto error;
            }
            case VM_CONTEXT_FINALLY_RETURN:
            {
              last_completion_value = stack_top_p[-2];

              VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth,
                                  PARSER_TRY_CONTEXT_STACK_ALLOCATION);
              stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
              goto error;
            }
            default:
            {
              stack_top_p = vm_stack_context_abort (frame_ctx_p, stack_top_p);
            }
          }

          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);
          break;
        }
        case VM_OC_JUMP_AND_EXIT_CONTEXT:
        {
          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

          branch_offset += (int32_t) (byte_code_start_p - frame_ctx_p->byte_code_start_p);

          if (vm_stack_find_finally (frame_ctx_p,
                                     &stack_top_p,
                                     VM_CONTEXT_FINALLY_JUMP,
                                     (uint32_t) branch_offset))
          {
            JERRY_ASSERT (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_FINALLY_JUMP);
            byte_code_p = frame_ctx_p->byte_code_p;
            stack_top_p[-2] = (uint32_t) branch_offset;
          }
          else
          {
            byte_code_p = frame_ctx_p->byte_code_start_p + branch_offset;
          }

          JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);
          break;
        }
        default:
        {
          JERRY_UNREACHABLE ();
          break;
        }
      }

      if (VM_OC_HAS_PUT_RESULT (opcode_data))
      {
        if (opcode_data & VM_OC_PUT_IDENT)
        {
          uint16_t literal_index;

          READ_LITERAL_INDEX (literal_index);

          if (literal_index < register_end)
          {
            ecma_fast_free_value (frame_ctx_p->registers_p[literal_index]);

            frame_ctx_p->registers_p[literal_index] = result;

            if (opcode_data & (VM_OC_PUT_STACK | VM_OC_PUT_BLOCK))
            {
              result = ecma_fast_copy_value (result);
            }
          }
          else
          {
            ecma_string_t *var_name_str_p;
            ecma_object_t *ref_base_lex_env_p;

            var_name_str_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]);
            ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p,
                                                                 var_name_str_p);

            last_completion_value = ecma_op_put_value_lex_env_base (ref_base_lex_env_p,
                                                                    var_name_str_p,
                                                                    is_strict,
                                                                    result);

            ecma_deref_ecma_string (var_name_str_p);

            if (ecma_is_value_error (last_completion_value))
            {
              ecma_free_value (result);
              goto error;
            }

            if (!(opcode_data & (VM_OC_PUT_STACK | VM_OC_PUT_BLOCK)))
            {
              ecma_fast_free_value (result);
            }
          }
        }
        else if (opcode_data & VM_OC_PUT_REFERENCE)
        {
          ecma_value_t property = *(--stack_top_p);
          ecma_value_t object = *(--stack_top_p);

          if (object == ecma_make_simple_value (ECMA_SIMPLE_VALUE_REGISTER_REF))
          {
            ecma_fast_free_value (frame_ctx_p->registers_p[property]);

            frame_ctx_p->registers_p[property] = result;

            if (opcode_data & (VM_OC_PUT_STACK | VM_OC_PUT_BLOCK))
            {
              result = ecma_fast_copy_value (result);
            }
          }
          else
          {
            last_completion_value = vm_op_set_value (object,
                                                     property,
                                                     result,
                                                     is_strict);

            ecma_free_value (object);
            ecma_free_value (property);

            if (ecma_is_value_error (last_completion_value))
            {
              ecma_free_value (result);
              goto error;
            }

            if (!(opcode_data & (VM_OC_PUT_STACK | VM_OC_PUT_BLOCK)))
            {
              ecma_fast_free_value (result);
            }
          }
        }

        if (opcode_data & VM_OC_PUT_STACK)
        {
          *stack_top_p++ = result;
        }
        else if (opcode_data & VM_OC_PUT_BLOCK)
        {
          ecma_fast_free_value (block_result);
          block_result = result;
        }
      }

      ecma_fast_free_value (left_value);
      ecma_fast_free_value (right_value);
    }
error:

    ecma_fast_free_value (left_value);
    ecma_fast_free_value (right_value);

    if (unlikely (ecma_is_value_error (last_completion_value)))
    {
      ecma_value_t *vm_stack_p = stack_top_p;

      for (vm_stack_p = frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth;
           vm_stack_p < stack_top_p;
           vm_stack_p++)
      {
        if (*vm_stack_p == ecma_make_simple_value (ECMA_SIMPLE_VALUE_REGISTER_REF))
        {
          JERRY_ASSERT (vm_stack_p < stack_top_p);
          vm_stack_p++;
        }
        else
        {
          ecma_free_value (*vm_stack_p);
        }
      }

      stack_top_p = frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth;
    }

    JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

    if (frame_ctx_p->context_depth == 0)
    {
      /* In most cases there is no context. */

      ecma_fast_free_value (block_result);
      return last_completion_value;
    }

    if (!ecma_is_value_error (last_completion_value))
    {
      JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

      stack_top_p = frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth;

      if (vm_stack_find_finally (frame_ctx_p,
                                 &stack_top_p,
                                 VM_CONTEXT_FINALLY_RETURN,
                                 0))
      {
        JERRY_ASSERT (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_FINALLY_RETURN);
        JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

        byte_code_p = frame_ctx_p->byte_code_p;
        stack_top_p[-2] = last_completion_value;
        continue;
      }
    }
    else
    {
      if (vm_stack_find_finally (frame_ctx_p,
                                 &stack_top_p,
                                 VM_CONTEXT_FINALLY_THROW,
                                 0))
      {
        JERRY_ASSERT (frame_ctx_p->registers_p + register_end + frame_ctx_p->context_depth == stack_top_p);

        result = last_completion_value;
        byte_code_p = frame_ctx_p->byte_code_p;

        if (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_CATCH)
        {
          uint32_t literal_index;
          ecma_object_t *catch_env_p;
          ecma_string_t *catch_name_p;

          *stack_top_p++ = ecma_get_value_from_error_value (result);

          JERRY_ASSERT (byte_code_p[0] == CBC_ASSIGN_SET_IDENT);

          literal_index = byte_code_p[1];
          if (literal_index >= encoding_limit)
          {
            literal_index = ((literal_index << 8) | byte_code_p[2]) - encoding_delta;
          }

          catch_env_p = ecma_create_decl_lex_env (frame_ctx_p->lex_env_p);

          catch_name_p = ecma_new_ecma_string_from_lit_cp (literal_start_p[literal_index]);

          ecma_op_create_mutable_binding (catch_env_p, catch_name_p, false);

          ecma_deref_ecma_string (catch_name_p);

          stack_top_p[-2 - 1] = ecma_make_object_value (frame_ctx_p->lex_env_p);
          frame_ctx_p->lex_env_p = catch_env_p;
        }
        else
        {
          JERRY_ASSERT (VM_GET_CONTEXT_TYPE (stack_top_p[-1]) == VM_CONTEXT_FINALLY_THROW);
          stack_top_p[-2] = result;
        }
        continue;
      }
    }

    ecma_free_value (block_result);
    return last_completion_value;
  }
} /* vm_loop */

#undef READ_LITERAL
#undef READ_LITERAL_INDEX

/**
 * Execute code block.
 *
 * @return ecma value
 */
static ecma_value_t __attr_noinline___
vm_execute (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
            const ecma_value_t *arg_p, /**< arguments list */
            ecma_length_t arg_list_len) /**< length of arguments list */
{
  const ecma_compiled_code_t *bytecode_header_p = frame_ctx_p->bytecode_header_p;
  ecma_value_t completion_value;
  vm_frame_ctx_t *prev_context_p;
  uint16_t argument_end;
  uint16_t register_end;

  if (bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_header_p;

    argument_end = args_p->argument_end;
    register_end = args_p->register_end;
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_header_p;

    argument_end = args_p->argument_end;
    register_end = args_p->register_end;
  }

  frame_ctx_p->stack_top_p = frame_ctx_p->registers_p + register_end;

  if (arg_list_len > argument_end)
  {
    arg_list_len = argument_end;
  }

  for (uint32_t i = 0; i < arg_list_len; i++)
  {
    frame_ctx_p->registers_p[i] = ecma_fast_copy_value (arg_p[i]);
  }

  /* The arg_list_len contains the end of the copied arguments.
   * Fill everything else with undefined. */
  if (register_end > arg_list_len)
  {
    ecma_value_t *stack_p = frame_ctx_p->registers_p + arg_list_len;

    for (uint32_t i = arg_list_len; i < register_end; i++)
    {
      *stack_p++ = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
  }

  is_direct_eval_form_call = false;

  prev_context_p = vm_top_context_p;
  vm_top_context_p = frame_ctx_p;

  completion_value = vm_init_loop (frame_ctx_p);

  if (likely (!ecma_is_value_error (completion_value)))
  {
    while (true)
    {
      completion_value = vm_loop (frame_ctx_p);

      if (frame_ctx_p->call_operation == VM_NO_EXEC_OP)
      {
        break;
      }

      if (frame_ctx_p->call_operation == VM_EXEC_CALL)
      {
        opfunc_call (frame_ctx_p);
      }
      else
      {
        JERRY_ASSERT (frame_ctx_p->call_operation == VM_EXEC_CONSTRUCT);
        opfunc_construct (frame_ctx_p);
      }
    }
  }

  /* Free arguments and registers */
  for (uint32_t i = 0; i < register_end; i++)
  {
    ecma_fast_free_value (frame_ctx_p->registers_p[i]);
  }

  vm_top_context_p = prev_context_p;
  return completion_value;
} /* vm_execute */

#define INLINE_STACK_SIZE 16

/**
 * Run the code with inline stack.
 *
 * @return ecma value
 */
static ecma_value_t __attr_noinline___
vm_run_with_inline_stack (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                          const ecma_value_t *arg_p, /**< arguments list */
                          ecma_length_t arg_list_len) /**< length of arguments list */
{
  ecma_value_t inline_stack[INLINE_STACK_SIZE];

  frame_ctx_p->registers_p = inline_stack;

  return vm_execute (frame_ctx_p, arg_p, arg_list_len);
} /* vm_run_with_inline_stack */

/**
 * Run the code with inline stack.
 *
 * @return ecma value
 */
static ecma_value_t __attr_noinline___
vm_run_with_alloca (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                    const ecma_value_t *arg_p, /**< arguments list */
                    ecma_length_t arg_list_len, /**< length of arguments list */
                    uint32_t call_stack_size) /**< call stack size */
{
  size_t size = call_stack_size * sizeof (ecma_value_t);

  ecma_value_t *stack = (ecma_value_t *) alloca (size);

  frame_ctx_p->registers_p = stack;

  return vm_execute (frame_ctx_p, arg_p, arg_list_len);
} /* vm_run_with_alloca */

/**
 * Run the code.
 *
 * @return ecma value
 */
ecma_value_t
vm_run (const ecma_compiled_code_t *bytecode_header_p, /**< byte-code data header */
        ecma_value_t this_binding_value, /**< value of 'ThisBinding' */
        ecma_object_t *lex_env_p, /**< lexical environment to use */
        bool is_eval_code, /**< is the code is eval code (ECMA-262 v5, 10.1) */
        const ecma_value_t *arg_list_p, /**< arguments list */
        ecma_length_t arg_list_len) /**< length of arguments list */
{
  lit_cpointer_t *literal_p;
  vm_frame_ctx_t frame_ctx;
  uint32_t call_stack_size;

  if (bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_header_p;
    uint8_t *byte_p = (uint8_t *) bytecode_header_p;

    literal_p = (lit_cpointer_t *) (byte_p + sizeof (cbc_uint16_arguments_t));
    frame_ctx.literal_start_p = literal_p;
    literal_p += args_p->literal_end;
    call_stack_size = (uint32_t) (args_p->register_end + args_p->stack_limit);
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_header_p;
    uint8_t *byte_p = (uint8_t *) bytecode_header_p;

    literal_p = (lit_cpointer_t *) (byte_p + sizeof (cbc_uint8_arguments_t));
    frame_ctx.literal_start_p = literal_p;
    literal_p += args_p->literal_end;
    call_stack_size = (uint32_t) (args_p->register_end + args_p->stack_limit);
  }

  frame_ctx.bytecode_header_p = bytecode_header_p;
  frame_ctx.byte_code_p = (uint8_t *) literal_p;
  frame_ctx.byte_code_start_p = (uint8_t *) literal_p;
  frame_ctx.lex_env_p = lex_env_p;
  frame_ctx.this_binding = this_binding_value;
  frame_ctx.context_depth = 0;
  frame_ctx.is_eval_code = is_eval_code;
  frame_ctx.call_operation = VM_NO_EXEC_OP;

  if (call_stack_size <= INLINE_STACK_SIZE)
  {
    return vm_run_with_inline_stack (&frame_ctx,
                                     arg_list_p,
                                     arg_list_len);
  }
  else
  {
    return vm_run_with_alloca (&frame_ctx,
                               arg_list_p,
                               arg_list_len,
                               call_stack_size);
  }
} /* vm_run */

/**
 * Check whether currently executed code is strict mode code
 *
 * @return true - current code is executed in strict mode,
 *         false - otherwise.
 */
bool
vm_is_strict_mode (void)
{
  JERRY_ASSERT (vm_top_context_p != NULL);

  return vm_top_context_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE;
} /* vm_is_strict_mode */

/**
 * Check whether currently performed call (on top of call-stack) is performed in form,
 * meeting conditions of 'Direct Call to Eval' (see also: ECMA-262 v5, 15.1.2.1.1)
 *
 * Warning:
 *         the function should only be called from implementation
 *         of built-in 'eval' routine of Global object
 *
 * @return true - currently performed call is performed through 'eval' identifier,
 *                without 'this' argument,
 *         false - otherwise.
 */
bool
vm_is_direct_eval_form_call (void)
{
  return is_direct_eval_form_call;
} /* vm_is_direct_eval_form_call */

/**
 * @}
 * @}
 */
