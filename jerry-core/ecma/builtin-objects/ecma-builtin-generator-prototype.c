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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "jcontext.h"
#include "opcodes.h"
#include "vm-defines.h"

#if ENABLED (JERRY_ES2015)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-generator-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID generator_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup generator ECMA Generator object built-in
 * @{
 */

/**
 * Byte code sequence which returns from the generator.
 */
static const uint8_t ecma_builtin_generator_prototype_return[2] =
{
  CBC_EXT_OPCODE, CBC_EXT_RETURN
};

/**
 * Byte code sequence which throws an exception.
 */
static const uint8_t ecma_builtin_generator_prototype_throw[1] =
{
  CBC_THROW
};

/**
 * Helper function for next / return / throw
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_generator_prototype_object_do (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t arg, /**< argument */
                                            ecma_iterator_command_type_t resume_mode) /**< resume mode */
{
  vm_executable_object_t *executable_object_p = NULL;

  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (this_arg);

    if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_CLASS)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_GENERATOR_UL)
      {
        executable_object_p = (vm_executable_object_t *) ext_object_p;
      }
    }
  }

  if (executable_object_p == NULL)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a generator object."));
  }

  if (executable_object_p->extended_object.u.class_prop.extra_info & ECMA_EXECUTABLE_OBJECT_RUNNING)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Generator is currently under execution."));
  }

  if (executable_object_p->extended_object.u.class_prop.extra_info & ECMA_EXECUTABLE_OBJECT_COMPLETED)
  {
    return ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
  }

  arg = ecma_copy_value (arg);

  while (true)
  {
    if (executable_object_p->extended_object.u.class_prop.extra_info & ECMA_GENERATOR_ITERATE_AND_YIELD)
    {
      ecma_value_t iterator = executable_object_p->extended_object.u.class_prop.u.value;

      bool done = false;
      ecma_value_t result = ecma_op_iterator_do (resume_mode, iterator, arg, &done);
      ecma_free_value (arg);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        arg = result;
      }
      else if (done)
      {
        arg = ecma_op_iterator_value (result);
        ecma_free_value (result);
        if (resume_mode == ECMA_ITERATOR_THROW)
        {
          /* This part is changed in the newest ECMAScript standard.
           * It was ECMA_ITERATOR_RETURN in ES2015, but I think it was a typo. */
          resume_mode = ECMA_ITERATOR_NEXT;
        }
      }
      else
      {
        return result;
      }

      executable_object_p->extended_object.u.class_prop.extra_info &= (uint16_t) ~ECMA_GENERATOR_ITERATE_AND_YIELD;

      if (ECMA_IS_VALUE_ERROR (arg))
      {
        arg = JERRY_CONTEXT (error_value);
        JERRY_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_EXCEPTION;
        resume_mode = ECMA_ITERATOR_THROW;
      }
    }

    if (resume_mode == ECMA_ITERATOR_RETURN)
    {
      executable_object_p->frame_ctx.byte_code_p = ecma_builtin_generator_prototype_return;
    }
    else if (resume_mode == ECMA_ITERATOR_THROW)
    {
      executable_object_p->frame_ctx.byte_code_p = ecma_builtin_generator_prototype_throw;
    }

    ecma_value_t value = opfunc_resume_executable_object (executable_object_p, arg);

    if (ECMA_IS_VALUE_ERROR (value))
    {
      return value;
    }

    bool done = (executable_object_p->extended_object.u.class_prop.extra_info & ECMA_EXECUTABLE_OBJECT_COMPLETED);

    if (!done)
    {
      const uint8_t *byte_code_p = executable_object_p->frame_ctx.byte_code_p;

      JERRY_ASSERT (byte_code_p[-2] == CBC_EXT_OPCODE
                    && (byte_code_p[-1] == CBC_EXT_YIELD || byte_code_p[-1] == CBC_EXT_YIELD_ITERATOR));

      if (byte_code_p[-1] == CBC_EXT_YIELD_ITERATOR)
      {
        ecma_value_t iterator = ecma_op_get_iterator (value, ECMA_VALUE_EMPTY);
        ecma_free_value (value);

        if (ECMA_IS_VALUE_ERROR (iterator))
        {
          resume_mode = ECMA_ITERATOR_THROW;
          arg = JERRY_CONTEXT (error_value);
          JERRY_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_EXCEPTION;
          continue;
        }

        ecma_deref_object (ecma_get_object_from_value (iterator));
        executable_object_p->extended_object.u.class_prop.extra_info |= ECMA_GENERATOR_ITERATE_AND_YIELD;
        executable_object_p->extended_object.u.class_prop.u.value = iterator;
        arg = ECMA_VALUE_UNDEFINED;
        continue;
      }
    }

    ecma_value_t result = ecma_create_iter_result_object (value, ecma_make_boolean_value (done));
    ecma_fast_free_value (value);
    return result;
  }
} /* ecma_builtin_generator_prototype_object_do */

/**
 * The Generator.prototype object's 'next' routine
 *
 * See also:
 *          ECMA-262 v6, 25.3.1.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_generator_prototype_object_next (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t next_arg) /**< next argument */
{
  return ecma_builtin_generator_prototype_object_do (this_arg, next_arg, ECMA_ITERATOR_NEXT);
} /* ecma_builtin_generator_prototype_object_next */

/**
 * The Generator.prototype object's 'return' routine
 *
 * See also:
 *          ECMA-262 v6, 25.3.1.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_generator_prototype_object_return (ecma_value_t this_arg, /**< this argument */
                                                ecma_value_t return_arg) /**< return argument */
{
  return ecma_builtin_generator_prototype_object_do (this_arg, return_arg, ECMA_ITERATOR_RETURN);
} /* ecma_builtin_generator_prototype_object_return */

/**
 * The Generator.prototype object's 'throw' routine
 *
 * See also:
 *          ECMA-262 v6, 25.3.1.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_generator_prototype_object_throw (ecma_value_t this_arg, /**< this argument */
                                                ecma_value_t throw_arg) /**< throw argument */
{
  return ecma_builtin_generator_prototype_object_do (this_arg, throw_arg, ECMA_ITERATOR_THROW);
} /* ecma_builtin_generator_prototype_object_throw */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015) */
