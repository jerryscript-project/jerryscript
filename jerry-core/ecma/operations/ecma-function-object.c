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

#include "ecma-alloc.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-objects-arguments.h"
#include "ecma-try-catch-macro.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
/**
 * Get the resource name from the compiled code header
 *
 * @return resource name as ecma-string
 */
ecma_value_t
ecma_op_resource_name (const ecma_compiled_code_t *bytecode_header_p)
{
  JERRY_ASSERT (bytecode_header_p != NULL);

  ecma_length_t formal_params_number = 0;

  if (CBC_NON_STRICT_ARGUMENTS_NEEDED (bytecode_header_p))
  {
    if (bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
    {
      cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_header_p;

      formal_params_number = args_p->argument_end;
    }
    else
    {
      cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_header_p;

      formal_params_number = args_p->argument_end;
    }
  }

  uint8_t *byte_p = (uint8_t *) bytecode_header_p;
  byte_p += ((size_t) bytecode_header_p->size) << JMEM_ALIGNMENT_LOG;

  ecma_value_t *resource_name_p = (ecma_value_t *) byte_p;
  resource_name_p -= formal_params_number;

  return resource_name_p[-1];
} /* ecma_op_resource_name */
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

/**
 * Checks whether the type is a normal or arrow function.
 *
 * @return true - if the type is a normal or arrow function;
 *         false - otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_is_normal_or_arrow_function (ecma_object_type_t type) /**< object type */
{
#if ENABLED (JERRY_ES2015)
  return (type == ECMA_OBJECT_TYPE_FUNCTION || type == ECMA_OBJECT_TYPE_ARROW_FUNCTION);
#else /* !ENABLED (JERRY_ES2015) */
  return (type == ECMA_OBJECT_TYPE_FUNCTION);
#endif /* ENABLED (JERRY_ES2015) */
} /* ecma_is_normal_or_arrow_function */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true - if the given object is callable;
 *         false - otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_is_callable (ecma_object_t *obj_p) /**< ecma object */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  return ecma_get_object_type (obj_p) >= ECMA_OBJECT_TYPE_FUNCTION;
} /* ecma_op_object_is_callable */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true - if value is callable object;
 *         false - otherwise
 */
bool
ecma_op_is_callable (ecma_value_t value) /**< ecma value */
{
  return (ecma_is_value_object (value)
          && ecma_op_object_is_callable (ecma_get_object_from_value (value)));
} /* ecma_op_is_callable */

/**
 * Checks whether the given object implements [[Construct]].
 *
 * @return true - if the given object is constructor;
 *         false - otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_object_is_constructor (ecma_object_t *obj_p) /**< ecma object */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  ecma_object_type_t type = ecma_get_object_type (obj_p);

  if (type == ECMA_OBJECT_TYPE_FUNCTION)
  {
    return (!ecma_get_object_is_builtin (obj_p) || !ecma_builtin_function_is_routine (obj_p));
  }

  return (type >= ECMA_OBJECT_TYPE_BOUND_FUNCTION);
} /* ecma_object_is_constructor */

/**
 * Checks whether the value is Object that implements [[Construct]].
 *
 * @return true - if value is constructor object;
 *         false - otherwise
 */
bool
ecma_is_constructor (ecma_value_t value) /**< ecma value */
{
  return (ecma_is_value_object (value)
          && ecma_object_is_constructor (ecma_get_object_from_value (value)));
} /* ecma_is_constructor */

/**
 * Function object creation operation.
 *
 * See also: ECMA-262 v5, 13.2
 *
 * @return pointer to newly created Function object
 */
ecma_object_t *
ecma_op_create_function_object (ecma_object_t *scope_p, /**< function's scope */
                                const ecma_compiled_code_t *bytecode_data_p) /**< byte-code array */
{
  JERRY_ASSERT (ecma_is_lexical_environment (scope_p));

  /* 1., 4., 13. */
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  size_t function_object_size = sizeof (ecma_extended_object_t);

#if ENABLED (JERRY_SNAPSHOT_EXEC)
  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION)
  {
    function_object_size = sizeof (ecma_static_function_t);
  }
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */

  ecma_object_t *func_p = ecma_create_object (prototype_obj_p,
                                              function_object_size,
                                              ECMA_OBJECT_TYPE_FUNCTION);

  /* 2., 6., 7., 8. */
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  /* 3. */
  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_p;

  /* 9. */
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.scope_cp, scope_p);

  /* 10., 11., 12. */

#if ENABLED (JERRY_SNAPSHOT_EXEC)
  if (!(bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION))
  {
    ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.bytecode_cp, bytecode_data_p);
    ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);
  }
  else
  {
    ext_func_p->u.function.bytecode_cp = ECMA_NULL_POINTER;
    ((ecma_static_function_t *) func_p)->bytecode_p = bytecode_data_p;
  }
#else /* !ENABLED (JERRY_SNAPSHOT_EXEC) */
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_p->u.function.bytecode_cp, bytecode_data_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */

  /* 14., 15., 16., 17., 18. */
  /*
   * 'length' and 'prototype' properties are instantiated lazily
   *
   * See also: ecma_op_function_try_to_lazy_instantiate_property
   */

  return func_p;
} /* ecma_op_create_function_object */

#if ENABLED (JERRY_ES2015)

/**
 * Arrow function object creation operation.
 *
 * See also: ES2015, 9.2.12
 *
 * @return pointer to newly created Function object
 */
ecma_object_t *
ecma_op_create_arrow_function_object (ecma_object_t *scope_p, /**< function's scope */
                                      const ecma_compiled_code_t *bytecode_data_p, /**< byte-code array */
                                      ecma_value_t this_binding) /**< value of 'this' binding */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  size_t arrow_function_object_size = sizeof (ecma_arrow_function_t);

#if ENABLED (JERRY_SNAPSHOT_EXEC)
  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION)
  {
    arrow_function_object_size = sizeof (ecma_static_arrow_function_t);
  }
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */

  ecma_object_t *func_p = ecma_create_object (prototype_obj_p,
                                              arrow_function_object_size,
                                              ECMA_OBJECT_TYPE_ARROW_FUNCTION);

  ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) func_p;

  ECMA_SET_NON_NULL_POINTER (arrow_func_p->scope_cp, scope_p);

#if ENABLED (JERRY_SNAPSHOT_EXEC)
  if (!(bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION))
  {
    ECMA_SET_NON_NULL_POINTER (arrow_func_p->bytecode_cp, bytecode_data_p);
    ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);
  }
  else
  {
    arrow_func_p->bytecode_cp = ECMA_NULL_POINTER;
    ((ecma_static_arrow_function_t *) func_p)->bytecode_p = bytecode_data_p;
  }
#else /* !ENABLED (JERRY_SNAPSHOT_EXEC) */
  ECMA_SET_NON_NULL_POINTER (arrow_func_p->bytecode_cp, bytecode_data_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */

  arrow_func_p->this_binding = ecma_copy_value_if_not_object (this_binding);
  return func_p;
} /* ecma_op_create_arrow_function_object */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * External function object creation operation.
 *
 * Note:
 *      external function object is implementation-defined object type
 *      that represent functions implemented in native code, using Embedding API
 *
 * @return pointer to newly created external function object
 */
ecma_object_t *
ecma_op_create_external_function_object (ecma_external_handler_t handler_cb) /**< pointer to external native handler */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *function_obj_p;
  function_obj_p = ecma_create_object (prototype_obj_p,
                                       sizeof (ecma_extended_object_t),
                                       ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) function_obj_p;
  ext_func_obj_p->u.external_handler_cb = handler_cb;

  return function_obj_p;
} /* ecma_op_create_external_function_object */

/**
 * Get compiled code of a function object.
 *
 * @return compiled code
 */
inline const ecma_compiled_code_t * JERRY_ATTR_ALWAYS_INLINE
ecma_op_function_get_compiled_code (ecma_extended_object_t *function_p) /**< function pointer */
{
#if ENABLED (JERRY_SNAPSHOT_EXEC)
  if (function_p->u.function.bytecode_cp != ECMA_NULL_POINTER)
  {
    return ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                            function_p->u.function.bytecode_cp);
  }
  else
  {
    return ((ecma_static_function_t *) function_p)->bytecode_p;
  }
#else /* !ENABLED (JERRY_SNAPSHOT_EXEC) */
  return ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                          function_p->u.function.bytecode_cp);
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */
} /* ecma_op_function_get_compiled_code */

#if ENABLED (JERRY_ES2015)

/**
 * Get compiled code of an arrow function object.
 *
 * @return compiled code
 */
inline const ecma_compiled_code_t * JERRY_ATTR_ALWAYS_INLINE
ecma_op_arrow_function_get_compiled_code (ecma_arrow_function_t *arrow_function_p) /**< arrow function pointer */
{
#if ENABLED (JERRY_SNAPSHOT_EXEC)
  if (arrow_function_p->bytecode_cp != ECMA_NULL_POINTER)
  {
    return ECMA_GET_NON_NULL_POINTER (const ecma_compiled_code_t,
                                      arrow_function_p->bytecode_cp);
  }
  else
  {
    return ((ecma_static_arrow_function_t *) arrow_function_p)->bytecode_p;
  }
#else /* !ENABLED (JERRY_SNAPSHOT_EXEC) */
  return ECMA_GET_NON_NULL_POINTER (const ecma_compiled_code_t,
                                    arrow_function_p->bytecode_cp);
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */
} /* ecma_op_arrow_function_get_compiled_code */

#endif /* ENABLED (JERRY_ES2015) */

#if ENABLED (JERRY_ES2015)
/**
 * Helper function for implicit class constructors [[HasInstance]] check.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_op_implicit_class_constructor_has_instance (ecma_object_t *func_obj_p, /**< Function object */
                                                 ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

  /* Since bound functions represents individual class constructor functions, we should check
     that the given value is instance of either of the bound function chain elements. */
  do
  {
    ecma_object_t *v_obj_p = ecma_get_object_from_value (value);

    ecma_value_t prototype_obj_value = ecma_op_object_get_by_magic_id (func_obj_p,
                                                                       LIT_MAGIC_STRING_PROTOTYPE);

    if (ECMA_IS_VALUE_ERROR (prototype_obj_value))
    {
      return prototype_obj_value;
    }

    if (!ecma_is_value_object (prototype_obj_value))
    {
      ecma_free_value (prototype_obj_value);
      return ecma_raise_type_error (ECMA_ERR_MSG ("Object expected."));
    }

    ecma_object_t *prototype_obj_p = ecma_get_object_from_value (prototype_obj_value);

    while (true)
    {
      jmem_cpointer_t v_obj_cp = v_obj_p->u2.prototype_cp;

      if (v_obj_cp == JMEM_CP_NULL)
      {
        break;
      }

      v_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, v_obj_cp);

      if (v_obj_p == prototype_obj_p)
      {
        ecma_deref_object (prototype_obj_p);
        return ECMA_VALUE_TRUE;
      }
    }

    ecma_deref_object (prototype_obj_p);

    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

    func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                  ext_function_p->u.bound_function.target_function);
  }
  while (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

  return ECMA_VALUE_FALSE;
} /* ecma_op_implicit_class_constructor_has_instance */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * 15.3.5.3 implementation of [[HasInstance]] for Function objects
 *
 * @return true/false - if arguments are valid
 *         error - otherwise
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p, /**< Function object */
                               ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));

  if (!ecma_is_value_object (value))
  {
    return ECMA_VALUE_FALSE;
  }

  while (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 1. 3. */
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

#if ENABLED (JERRY_ES2015)
    if (JERRY_UNLIKELY (ext_function_p->u.bound_function.args_len_or_this == ECMA_VALUE_IMPLICIT_CONSTRUCTOR))
    {
      return ecma_op_implicit_class_constructor_has_instance (func_obj_p, value);
    }
#endif /* ENABLED (JERRY_ES2015) */

    func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                  ext_function_p->u.bound_function.target_function);
  }

  JERRY_ASSERT (ecma_is_normal_or_arrow_function (ecma_get_object_type (func_obj_p))
                || ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  ecma_object_t *v_obj_p = ecma_get_object_from_value (value);

  ecma_value_t prototype_obj_value = ecma_op_object_get_by_magic_id (func_obj_p,
                                                                     LIT_MAGIC_STRING_PROTOTYPE);

  if (ECMA_IS_VALUE_ERROR (prototype_obj_value))
  {
    return prototype_obj_value;
  }

  if (!ecma_is_value_object (prototype_obj_value))
  {
    ecma_free_value (prototype_obj_value);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Object expected."));
  }

  ecma_object_t *prototype_obj_p = ecma_get_object_from_value (prototype_obj_value);
  JERRY_ASSERT (prototype_obj_p != NULL);

  ecma_value_t result = ECMA_VALUE_FALSE;

  while (true)
  {
    jmem_cpointer_t v_obj_cp = v_obj_p->u2.prototype_cp;

    if (v_obj_cp == JMEM_CP_NULL)
    {
      break;
    }

    v_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, v_obj_cp);

    if (v_obj_p == prototype_obj_p)
    {
      result = ECMA_VALUE_TRUE;
      break;
    }
  }

  ecma_deref_object (prototype_obj_p);
  return result;
} /* ecma_op_function_has_instance */


#if ENABLED (JERRY_ES2015)
/**
 * Indicates whether the class has been invoked with 'new'.
 */
#define ECMA_CLASS_CONSTRUCT_FLAG ((uintptr_t) 0x01u)
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Sets the construct flag in the arguments list pointer.
 *
 * @return arguments list pointer with the construct flag
 */
static inline const ecma_value_t * JERRY_ATTR_ALWAYS_INLINE
ecma_op_function_set_construct_flag (const ecma_value_t *arguments_list_p) /**< original arguments list pointer */
{
  /* Any ecma value list must be aligned to 4 byte. */
  JERRY_ASSERT ((((uintptr_t) arguments_list_p) & 0x3) == 0);

#if ENABLED (JERRY_ES2015)
  arguments_list_p = (const ecma_value_t *)(((uintptr_t) arguments_list_p) | ECMA_CLASS_CONSTRUCT_FLAG);
#endif /* ENABLED (JERRY_ES2015) */

  return arguments_list_p;
} /* ecma_op_function_set_construct_flag */

/**
 * Clears the construct flag in the arguments list pointer.
 *
 * @return arguments list pointer without the construct flag
 */
static inline const ecma_value_t * JERRY_ATTR_ALWAYS_INLINE
ecma_op_function_clear_construct_flag (const ecma_value_t *arguments_list_p) /**< modified arguments list pointer */
{
#if ENABLED (JERRY_ES2015)
  arguments_list_p = (const ecma_value_t *)(((uintptr_t) arguments_list_p) & ~ECMA_CLASS_CONSTRUCT_FLAG);
#endif /* ENABLED (JERRY_ES2015) */

  return arguments_list_p;
} /* ecma_op_function_clear_construct_flag */

/**
 * Returns true if the construct flag is set.
 *
 * @return true, if construct flag is set, false otherwise
 */
static inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_op_function_has_construct_flag (const ecma_value_t *arguments_list_p) /**< modified arguments list pointer */
{
#if ENABLED (JERRY_ES2015)
  return (((uintptr_t) arguments_list_p) & ECMA_CLASS_CONSTRUCT_FLAG);
#else /* !ENABLED (JERRY_ES2015) */
  JERRY_UNUSED (arguments_list_p);
  return false;
#endif /* ENABLED (JERRY_ES2015) */
} /* ecma_op_function_has_construct_flag */

#if ENABLED (JERRY_ES2015)
/**
 * Returns the closest declarative lexical enviroment to the super object bound lexical enviroment.
 *
 * @return the found lexical enviroment
 */
static ecma_object_t *
ecma_op_find_super_declerative_lex_env (ecma_object_t *lex_env_p) /**< starting lexical enviroment */
{
  JERRY_ASSERT (lex_env_p != NULL);
  JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) != ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND);

  while (true)
  {
    jmem_cpointer_t lex_env_outer_cp = lex_env_p->u2.outer_reference_cp;

    if (lex_env_outer_cp != JMEM_CP_NULL)
    {
      ecma_object_t *lex_env_outer_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_outer_cp);

      if (ecma_get_lex_env_type (lex_env_outer_p) == ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND)
      {
        JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);
        return lex_env_p;
      }

      lex_env_p = lex_env_outer_p;
    }
    else
    {
      return NULL;
    }
  }
} /* ecma_op_find_super_declerative_lex_env */

/**
 * Returns with the current class this_binding property
 *
 * @return NULL - if the property was not found
 *         the found property - otherwise
 */
static ecma_property_t *
ecma_op_get_class_this_binding_property (ecma_object_t *lex_env_p) /**< starting lexical enviroment */
{
  JERRY_ASSERT (lex_env_p);
  JERRY_ASSERT (ecma_is_lexical_environment (lex_env_p));

  lex_env_p = ecma_op_find_super_declerative_lex_env (lex_env_p);
  ecma_string_t *name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_CLASS_THIS_BINDING);

  return lex_env_p == NULL ? NULL : ecma_find_named_property (lex_env_p, name_p);
} /* ecma_op_get_class_this_binding_property */

/**
 * Checks whether the 'super(...)' has been called.
 *
 * @return true  - if the 'super (...)' has been called
 *         false - otherwise
 */
inline bool JERRY_ATTR_PURE
ecma_op_is_super_called (ecma_object_t *lex_env_p) /**< starting lexical enviroment */
{
  ecma_property_t *property_p = ecma_op_get_class_this_binding_property (lex_env_p);

  JERRY_ASSERT (property_p);
  return (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);
} /* ecma_op_is_super_called */

/**
 * Sets the value of 'super(...)' has been called.
 */
inline void
ecma_op_set_super_called (ecma_object_t *lex_env_p) /**< starting lexical enviroment */
{
  ecma_property_t *property_p = ecma_op_get_class_this_binding_property (lex_env_p);

  JERRY_ASSERT (property_p);

  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_INTERNAL);
  ECMA_CONVERT_INTERNAL_PROPERTY_TO_DATA_PROPERTY (property_p);
} /* ecma_op_set_super_called */

/**
 * Sets the class context this_binding value.
 */
void
ecma_op_set_class_this_binding (ecma_object_t *lex_env_p, /**< starting lexical enviroment */
                                ecma_value_t this_binding) /**< 'this' argument's value */
{
  JERRY_ASSERT (ecma_is_value_object (this_binding));
  ecma_property_t *property_p = ecma_op_get_class_this_binding_property (lex_env_p);

  ecma_property_value_t *value_p;

  if (property_p)
  {
    JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);
    value_p = ECMA_PROPERTY_VALUE_PTR (property_p);
  }
  else
  {
    ecma_string_t *name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_CLASS_THIS_BINDING);
    value_p = ecma_create_named_data_property (lex_env_p, name_p, ECMA_PROPERTY_FLAG_WRITABLE, &property_p);
    ECMA_CONVERT_DATA_PROPERTY_TO_INTERNAL_PROPERTY (property_p);
  }

  value_p->value = this_binding;
} /* ecma_op_set_class_this_binding */

/**
 * Gets the class context this binding value.
 *
 * @return the class context this binding value
 */
ecma_value_t
ecma_op_get_class_this_binding (ecma_object_t *lex_env_p) /**< starting lexical enviroment */
{
  ecma_property_t *property_p = ecma_op_get_class_this_binding_property (lex_env_p);

  JERRY_ASSERT (property_p);

  ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  JERRY_ASSERT (ecma_is_value_object (value_p->value));
  return value_p->value;
} /* ecma_op_get_class_this_binding */

/**
 * Dummy external function for implicit constructor call.
 *
 * @return ECMA_VALUE_ERROR - TypeError
 */
ecma_value_t
ecma_op_function_implicit_constructor_handler_cb (const ecma_value_t function_obj, /**< the function itself */
                                                  const ecma_value_t this_val, /**< this_arg of the function */
                                                  const ecma_value_t args_p[], /**< argument list */
                                                  const ecma_length_t args_count) /**< argument number */
{
  JERRY_UNUSED_4 (function_obj, this_val, args_p, args_count);
  return ecma_raise_type_error (ECMA_ERR_MSG ("Class constructor cannot be invoked without 'new'."));
} /* ecma_op_function_implicit_constructor_handler_cb */

/**
 * Sets the completion value [[Prototype]] based on the this_arg value
 */
void
ecma_op_set_class_prototype (ecma_value_t completion_value, /**< completion_value */
                             ecma_value_t this_arg) /**< this argument*/
{
  JERRY_ASSERT (ecma_is_value_object (completion_value));
  JERRY_ASSERT (ecma_is_value_object (this_arg));

  ecma_object_t *completion_obj_p = ecma_get_object_from_value (completion_value);
  jmem_cpointer_t prototype_obj_cp = ecma_get_object_from_value (this_arg)->u2.prototype_cp;

  JERRY_ASSERT (prototype_obj_cp != JMEM_CP_NULL);

  completion_obj_p->u2.prototype_cp = prototype_obj_cp;
} /* ecma_op_set_class_prototype */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_FUNCTION).
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, /**< Function object */
                       ecma_value_t this_arg_value, /**< 'this' argument's value */
                       const ecma_value_t *arguments_list_p, /**< arguments list */
                       ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT (ecma_op_is_callable (ecma_make_object_value (func_obj_p)));

  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION
                || ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION
                || ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION
                || !ecma_op_function_has_construct_flag (arguments_list_p));

  ECMA_CHECK_STACK_USAGE ();

  switch (ecma_get_object_type (func_obj_p))
  {
    case ECMA_OBJECT_TYPE_FUNCTION:
    {
      if (JERRY_UNLIKELY (ecma_get_object_is_builtin (func_obj_p)))
      {
        JERRY_ASSERT (!ecma_op_function_has_construct_flag (arguments_list_p));

        ecma_value_t ret_value = ecma_builtin_dispatch_call (func_obj_p,
                                                             this_arg_value,
                                                             arguments_list_p,
                                                             arguments_list_len);

        return ret_value;
      }

      /* Entering Function Code (ECMA-262 v5, 10.4.3) */
      ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;

      ecma_object_t *scope_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                ext_func_p->u.function.scope_cp);

      /* 8. */
      ecma_value_t this_binding = this_arg_value;
      bool free_this_binding = false;

      const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);

#if ENABLED (JERRY_ES2015)
      bool is_class_constructor = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_CONSTRUCTOR) != 0;

      if (is_class_constructor && !ecma_op_function_has_construct_flag (arguments_list_p))
      {
        return ecma_raise_type_error (ECMA_ERR_MSG ("Class constructor cannot be invoked without 'new'."));
      }
#endif /* ENABLED (JERRY_ES2015) */

      bool is_strict = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0;
      bool is_no_lex_env = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED) != 0;

      /* 1. */
      if (!is_strict)
      {
        if (ecma_is_value_undefined (this_binding)
            || ecma_is_value_null (this_binding))
        {
          /* 2. */
          this_binding = ecma_make_object_value (ecma_builtin_get_global ());
        }
        else if (!ecma_is_value_object (this_binding))
        {
          /* 3., 4. */
          this_binding = ecma_op_to_object (this_binding);
          free_this_binding = true;

          JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (this_binding));
        }
      }

      arguments_list_p = ecma_op_function_clear_construct_flag (arguments_list_p);

      /* 5. */
      ecma_object_t *local_env_p;
      if (is_no_lex_env)
      {
        local_env_p = scope_p;
      }
      else
      {
        local_env_p = ecma_create_decl_lex_env (scope_p);
        if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED)
        {
          ecma_op_create_arguments_object (func_obj_p,
                                           local_env_p,
                                           arguments_list_p,
                                           arguments_list_len,
                                           bytecode_data_p);
        }
#if ENABLED (JERRY_ES2015)
        if (JERRY_UNLIKELY (is_class_constructor))
        {
          ecma_op_set_class_this_binding (local_env_p, this_binding);
        }
#endif /* ENABLED (JERRY_ES2015) */
      }

      ecma_value_t ret_value = vm_run (bytecode_data_p,
                                       this_binding,
                                       local_env_p,
                                       ECMA_PARSE_NO_OPTS,
                                       arguments_list_p,
                                       arguments_list_len);

      if (!is_no_lex_env)
      {
        ecma_deref_object (local_env_p);
      }

      if (JERRY_UNLIKELY (free_this_binding))
      {
        ecma_free_value (this_binding);
      }

      return ret_value;
    }
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    {
      ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;

      ecma_value_t ret_value = ext_func_obj_p->u.external_handler_cb (ecma_make_object_value (func_obj_p),
                                                                      this_arg_value,
                                                                      arguments_list_p,
                                                                      arguments_list_len);

      if (JERRY_UNLIKELY (ecma_is_value_error_reference (ret_value)))
      {
        JERRY_CONTEXT (error_value) = ecma_clear_error_reference (ret_value, true);
        return ECMA_VALUE_ERROR;
      }

#if ENABLED (JERRY_DEBUGGER)
      JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_VM_EXCEPTION_THROWN);
#endif /* ENABLED (JERRY_DEBUGGER) */
      return ret_value;
    }
#if ENABLED (JERRY_ES2015)
    case ECMA_OBJECT_TYPE_ARROW_FUNCTION:
    {
      /* Entering Function Code (ES2015, 9.2.1) */
      ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) func_obj_p;

      ecma_object_t *scope_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                          arrow_func_p->scope_cp);

      bool is_no_lex_env;

      const ecma_compiled_code_t *bytecode_data_p = ecma_op_arrow_function_get_compiled_code (arrow_func_p);

      is_no_lex_env = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED) != 0;

      ecma_object_t *local_env_p = scope_p;

      if (!is_no_lex_env)
      {
        local_env_p = ecma_create_decl_lex_env (scope_p);

        JERRY_ASSERT (!(bytecode_data_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED));
      }

      ecma_value_t ret_value = vm_run (bytecode_data_p,
                                       arrow_func_p->this_binding,
                                       local_env_p,
                                       ECMA_PARSE_NO_OPTS,
                                       arguments_list_p,
                                       arguments_list_len);

      if (!is_no_lex_env)
      {
        ecma_deref_object (local_env_p);
      }

      return ret_value;
    }
#endif /* ENABLED (JERRY_ES2015) */
    default:
    {
      JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
      break;
    }
  }

  JERRY_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_DIRECT_EVAL;

  ecma_extended_object_t *ext_function_p;
  ecma_object_t *target_func_obj_p;
  ecma_length_t args_length;

  do
  {
    /* 2-3. */
    ext_function_p = (ecma_extended_object_t *) func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    /* 4. */
    ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;

    if (!ecma_is_value_integer_number (args_len_or_this))
    {
#if ENABLED (JERRY_ES2015)
      if (JERRY_UNLIKELY (args_len_or_this == ECMA_VALUE_IMPLICIT_CONSTRUCTOR))
      {
        if (!ecma_op_function_has_construct_flag (arguments_list_p))
        {
          return ecma_raise_type_error (ECMA_ERR_MSG ("Class constructor cannot be invoked without 'new'."));
        }
        if (ecma_get_object_is_builtin (target_func_obj_p))
        {
          arguments_list_p = ecma_op_function_clear_construct_flag (arguments_list_p);
        }
      }
      else
      {
#endif /* ENABLED (JERRY_ES2015) */
        this_arg_value = args_len_or_this;
#if ENABLED (JERRY_ES2015)
      }
#endif /* ENABLED (JERRY_ES2015) */

      args_length = 1;
    }
    else
    {
      this_arg_value = *(ecma_value_t *) (ext_function_p + 1);
      args_length = (ecma_length_t) ecma_get_integer_from_value (args_len_or_this);
    }

    JERRY_ASSERT (args_length > 0);

    if (args_length == 1)
    {
      func_obj_p = target_func_obj_p;
    }
    else
    {
#if ENABLED (JERRY_ES2015)
      arguments_list_p = ecma_op_function_clear_construct_flag (arguments_list_p);
#endif /* ENABLED (JERRY_ES2015) */

      JERRY_ASSERT (!ecma_op_function_has_construct_flag (arguments_list_p));
      args_length--;

      ecma_length_t merged_args_list_len = args_length + arguments_list_len;
      ecma_value_t ret_value;

      JMEM_DEFINE_LOCAL_ARRAY (merged_args_list_p, merged_args_list_len, ecma_value_t);

      ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);

      memcpy (merged_args_list_p, args_p + 1, args_length * sizeof (ecma_value_t));
      memcpy (merged_args_list_p + args_length, arguments_list_p, arguments_list_len * sizeof (ecma_value_t));

      /* 5. */
      ret_value = ecma_op_function_call (target_func_obj_p,
                                         this_arg_value,
                                         merged_args_list_p,
                                         merged_args_list_len);

      JMEM_FINALIZE_LOCAL_ARRAY (merged_args_list_p);

      return ret_value;
    }
  }
  while (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

  return ecma_op_function_call (func_obj_p,
                                this_arg_value,
                                arguments_list_p,
                                arguments_list_len);
} /* ecma_op_function_call */

/**
 * [[Construct]] implementation:
 *   13.2.2 - for Function objects, created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION),
 *            and externally defined host functions (ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
 *   15.3.4.5.1 - for Function objects, created through 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION).
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                            ecma_value_t this_arg_value, /**< optional 'this' object value
                                                          *   or ECMA_VALUE_UNDEFINED */
                            const ecma_value_t *arguments_list_p, /**< arguments list */
                            ecma_length_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));

  JERRY_ASSERT (ecma_is_value_object (this_arg_value)
                || this_arg_value == ECMA_VALUE_UNDEFINED);

  ecma_object_t *target_func_obj_p = NULL;

  while (JERRY_UNLIKELY (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION))
  {
    /* 1-3. */
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) func_obj_p;

    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    /* 4. */
    ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;

    ecma_length_t args_length = 1;

    if (ecma_is_value_integer_number (args_len_or_this))
    {
      args_length = (ecma_length_t) ecma_get_integer_from_value (args_len_or_this);
    }

    JERRY_ASSERT (args_length > 0);

    /* 5. */
    if (args_length == 1)
    {
#if ENABLED (JERRY_ES2015)
      if (args_len_or_this == ECMA_VALUE_IMPLICIT_CONSTRUCTOR && ecma_is_value_undefined (this_arg_value))
      {
        break;
      }
#endif /* ENABLED (JERRY_ES2015) */
      func_obj_p = target_func_obj_p;
      continue;
    }

    ecma_value_t *args_p = (ecma_value_t *) (ext_function_p + 1);
    ecma_value_t ret_value;

    args_length--;
    ecma_length_t merged_args_list_len = args_length + arguments_list_len;

    JMEM_DEFINE_LOCAL_ARRAY (merged_args_list_p, merged_args_list_len, ecma_value_t);

    memcpy (merged_args_list_p, args_p + 1, args_length * sizeof (ecma_value_t));
    memcpy (merged_args_list_p + args_length, arguments_list_p, arguments_list_len * sizeof (ecma_value_t));

    /* 5. */
    ret_value = ecma_op_function_construct (target_func_obj_p,
                                            this_arg_value,
                                            merged_args_list_p,
                                            merged_args_list_len);

    JMEM_FINALIZE_LOCAL_ARRAY (merged_args_list_p);

    return ret_value;
  }

  ecma_object_type_t type = ecma_get_object_type (func_obj_p);

#if ENABLED (JERRY_ES2015)
  if (JERRY_UNLIKELY (type == ECMA_OBJECT_TYPE_ARROW_FUNCTION))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Arrow functions have no constructor."));
  }
#endif /* ENABLED (JERRY_ES2015) */

  if (JERRY_UNLIKELY (type == ECMA_OBJECT_TYPE_FUNCTION && ecma_get_object_is_builtin (func_obj_p)))
  {
    if (ecma_builtin_function_is_routine (func_obj_p))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Built-in routines have no constructor."));
    }

    ecma_value_t ret_value = ecma_builtin_dispatch_construct (func_obj_p,
                                                              arguments_list_p,
                                                              arguments_list_len);

#if ENABLED (JERRY_ES2015)
    if (!ecma_is_value_undefined (this_arg_value) && !ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_op_set_class_prototype (ret_value, this_arg_value);
    }
#endif /* ENABLED (JERRY_ES2015) */

    return ret_value;
  }

  ecma_object_t *new_this_obj_p = NULL;

  if (JERRY_LIKELY (this_arg_value == ECMA_VALUE_UNDEFINED))
  {
    /* 5. */
    ecma_value_t prototype_prop_value = ecma_op_object_get_by_magic_id (func_obj_p,
                                                                        LIT_MAGIC_STRING_PROTOTYPE);

    if (ECMA_IS_VALUE_ERROR (prototype_prop_value))
    {
      return prototype_prop_value;
    }

    /* 1., 2., 4. */
    if (ecma_is_value_object (prototype_prop_value))
    {
      /* 6. */
      new_this_obj_p = ecma_create_object (ecma_get_object_from_value (prototype_prop_value),
                                           0,
                                           ECMA_OBJECT_TYPE_GENERAL);
    }
    else
    {
      /* 7. */
      ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

      new_this_obj_p = ecma_create_object (prototype_p, 0, ECMA_OBJECT_TYPE_GENERAL);
    }

    ecma_free_value (prototype_prop_value);

    this_arg_value = ecma_make_object_value (new_this_obj_p);
  }

  /* 8. */
  ecma_value_t ret_value;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_FUNCTION:
    {
      arguments_list_p = ecma_op_function_set_construct_flag (arguments_list_p);

      ret_value = ecma_op_function_call (func_obj_p,
                                         this_arg_value,
                                         arguments_list_p,
                                         arguments_list_len);
      break;
    }
#if ENABLED (JERRY_ES2015)
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    {
      JERRY_ASSERT (!ecma_op_function_has_construct_flag (arguments_list_p));
      JERRY_ASSERT (target_func_obj_p != NULL);

      ret_value = ecma_op_function_construct (target_func_obj_p,
                                              this_arg_value,
                                              arguments_list_p,
                                              arguments_list_len);
      break;
    }
    case ECMA_OBJECT_TYPE_GENERAL:
    {
      /* Catch the special case when a the class extends value in null
         and the class has no explicit constructor to raise TypeError.*/
      JERRY_ASSERT (!ecma_op_function_has_construct_flag (arguments_list_p));
      JERRY_ASSERT (func_obj_p->u2.prototype_cp != JMEM_CP_NULL);
      JERRY_ASSERT ((ECMA_GET_NON_NULL_POINTER (ecma_object_t, func_obj_p->u2.prototype_cp) \
                    == ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE)));

      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Super constructor null is not a constructor."));
      break;
    }
#endif /* ENABLED (JERRY_ES2015) */
    default:
    {
      JERRY_ASSERT (type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

#if ENABLED (JERRY_ES2015)
      ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;

      if (ext_func_obj_p->u.external_handler_cb == ecma_op_function_implicit_constructor_handler_cb)
      {
        ret_value = ECMA_VALUE_UNDEFINED;
        break;
      }
#endif /* ENABLED (JERRY_ES2015) */

      ret_value = ecma_op_function_call (func_obj_p,
                                         this_arg_value,
                                         arguments_list_p,
                                         arguments_list_len);
      break;
    }
  }

  /* 9. */
  if (ECMA_IS_VALUE_ERROR (ret_value) || ecma_is_value_object (ret_value))
  {
    if (new_this_obj_p != NULL)
    {
      ecma_deref_object (new_this_obj_p);
    }
    return ret_value;
  }

  ecma_fast_free_value (ret_value);

  if (JERRY_UNLIKELY (new_this_obj_p == NULL))
  {
    ecma_ref_object (ecma_get_object_from_value (this_arg_value));
  }

  return this_arg_value;
} /* ecma_op_function_construct */

/**
 * Lazy instantiation of 'prototype' property for non-builtin and external functions
 *
 * @return pointer to newly instantiated property
 */
static ecma_property_t *
ecma_op_lazy_instantiate_prototype_object (ecma_object_t *object_p) /**< the function object */
{
  JERRY_ASSERT (ecma_get_object_type (object_p)  == ECMA_OBJECT_TYPE_FUNCTION
                || ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  /* ECMA-262 v5, 13.2, 16-18 */
  /* 16. */
  ecma_object_t *proto_object_p = ecma_op_create_object_object_noarg ();

  /* 17. */
  ecma_property_value_t *constructor_prop_value_p;
  constructor_prop_value_p = ecma_create_named_data_property (proto_object_p,
                                                              ecma_get_magic_string (LIT_MAGIC_STRING_CONSTRUCTOR),
                                                              ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                              NULL);

  constructor_prop_value_p->value = ecma_make_object_value (object_p);

  /* 18. */
  ecma_property_t *prototype_prop_p;
  ecma_property_value_t *prototype_prop_value_p;
  prototype_prop_value_p = ecma_create_named_data_property (object_p,
                                                            ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE),
                                                            ECMA_PROPERTY_FLAG_WRITABLE,
                                                            &prototype_prop_p);

  prototype_prop_value_p->value = ecma_make_object_value (proto_object_p);

  ecma_deref_object (proto_object_p);

  return prototype_prop_p;
} /* ecma_op_lazy_instantiate_prototype_object */

/**
 * Lazy instantiation of non-builtin ecma function object's properties
 *
 * Warning:
 *         Only non-configurable properties could be instantiated lazily in this function,
 *         as configurable properties could be deleted and it would be incorrect
 *         to reinstantiate them in the function in second time.
 *
 * @return pointer to newly instantiated property, if a property was instantiated,
 *         NULL - otherwise
 */
ecma_property_t *
ecma_op_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, /**< the function object */
                                                   ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (!ecma_get_object_is_builtin (object_p));

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_PROTOTYPE)
      && ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    return ecma_op_lazy_instantiate_prototype_object (object_p);
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLER)
      || ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_ARGUMENTS))
  {
    const ecma_compiled_code_t *bytecode_data_p;
#if ENABLED (JERRY_ES2015)
    if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARROW_FUNCTION)
    {
      ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) object_p;
      bytecode_data_p = ecma_op_arrow_function_get_compiled_code (arrow_func_p);
    }
    else
    {
#endif /* ENABLED (JERRY_ES2015) */
      ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
      bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);
#if ENABLED (JERRY_ES2015)
    }
#endif /* ENABLED (JERRY_ES2015) */

    if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE)
    {
      ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

      ecma_property_t *caller_prop_p;
      /* The property_name_p argument contans the name. */
      ecma_create_named_accessor_property (object_p,
                                           property_name_p,
                                           thrower_p,
                                           thrower_p,
                                           ECMA_PROPERTY_FIXED,
                                           &caller_prop_p);
      return caller_prop_p;
    }
  }

  return NULL;
} /* ecma_op_function_try_to_lazy_instantiate_property */

/**
 * Create specification defined non-configurable properties for external functions.
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t *
ecma_op_external_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, /**< object */
                                                            ecma_string_t *property_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_PROTOTYPE))
  {
    return ecma_op_lazy_instantiate_prototype_object (object_p);
  }

  return NULL;
} /* ecma_op_external_function_try_to_lazy_instantiate_property */

/**
 * Create specification defined non-configurable properties for bound functions.
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t *
ecma_op_bound_function_try_to_lazy_instantiate_property (ecma_object_t *object_p, /**< object */
                                                         ecma_string_t *property_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

  if (ecma_string_is_length (property_name_p))
  {
    ecma_extended_object_t *ext_function_p = (ecma_extended_object_t *) object_p;
    ecma_object_t *target_func_obj_p;
    target_func_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                         ext_function_p->u.bound_function.target_function);

    ecma_integer_value_t length = 0;

    if (ecma_object_get_class_name (target_func_obj_p) == LIT_MAGIC_STRING_FUNCTION_UL)
    {
      /* The property_name_p argument contans the 'length' string. */
      ecma_value_t get_len_value = ecma_op_object_get (target_func_obj_p, property_name_p);

      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (get_len_value));
      JERRY_ASSERT (ecma_is_value_integer_number (get_len_value));

      ecma_value_t args_len_or_this = ext_function_p->u.bound_function.args_len_or_this;
      ecma_integer_value_t args_length = 1;

      if (ecma_is_value_integer_number (args_len_or_this))
      {
        args_length = ecma_get_integer_from_value (args_len_or_this);
      }

      length = ecma_get_integer_from_value (get_len_value) - (args_length - 1);

      if (length < 0)
      {
        length = 0;
      }
    }

    ecma_property_t *len_prop_p;
    ecma_property_value_t *len_prop_value_p = ecma_create_named_data_property (object_p,
                                                                               property_name_p,
                                                                               ECMA_PROPERTY_FIXED,
                                                                               &len_prop_p);

    len_prop_value_p->value = ecma_make_integer_value (length);
    return len_prop_p;
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLER)
      || ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_ARGUMENTS))
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    ecma_property_t *caller_prop_p;
    /* The string_p argument contans the name. */
    ecma_create_named_accessor_property (object_p,
                                         property_name_p,
                                         thrower_p,
                                         thrower_p,
                                         ECMA_PROPERTY_FIXED,
                                         &caller_prop_p);
    return caller_prop_p;
  }

  return NULL;
} /* ecma_op_bound_function_try_to_lazy_instantiate_property */

/**
 * List names of a Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_function_try_to_lazy_instantiate_property
 */
void
ecma_op_function_list_lazy_property_names (ecma_object_t *object_p, /**< functionobject */
                                           bool separate_enumerable, /**< true - list enumerable properties into
                                                                      *          main collection and non-enumerable
                                                                      *          to collection of 'skipped
                                                                      *          non-enumerable' properties,
                                                                      *   false - list all properties into main
                                                                      *           collection.
                                                                      */
                                           ecma_collection_t *main_collection_p, /**< 'main' collection */
                                           ecma_collection_t *non_enum_collection_p) /**< skipped
                                                                                      *   'non-enumerable'
                                                                                      *   collection */
{
  JERRY_UNUSED (main_collection_p);

  ecma_collection_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));

  const ecma_compiled_code_t *bytecode_data_p;

#if ENABLED (JERRY_ES2015)
  if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARROW_FUNCTION)
  {
    bytecode_data_p = ecma_op_arrow_function_get_compiled_code ((ecma_arrow_function_t *) object_p);
  }
  else
  {
#endif /* ENABLED (JERRY_ES2015) */
    bytecode_data_p = ecma_op_function_get_compiled_code ((ecma_extended_object_t *) object_p);

    /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
    ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_PROTOTYPE));
#if ENABLED (JERRY_ES2015)
  }
#endif /* ENABLED (JERRY_ES2015) */

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE)
  {
    /* 'caller' property is non-enumerable (ECMA-262 v5, 13.2.5) */
    ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_CALLER));

    /* 'arguments' property is non-enumerable (ECMA-262 v5, 13.2.5) */
    ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_ARGUMENTS));
  }
} /* ecma_op_function_list_lazy_property_names */

/**
 * List names of an External Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_external_function_try_to_lazy_instantiate_property
 */
void
ecma_op_external_function_list_lazy_property_names (bool separate_enumerable, /**< true - list enumerable properties
                                                                               *          into main collection and
                                                                               *          non-enumerable to collection
                                                                               *          of 'skipped non-enumerable'
                                                                               *          properties,
                                                                               *   false - list all properties into
                                                                               *           main collection.
                                                                               */
                                                   ecma_collection_t *main_collection_p, /**< 'main' collection */
                                                   ecma_collection_t *non_enum_collection_p) /**< skipped
                                                                                              *   collection */
{
  JERRY_UNUSED (main_collection_p);

  ecma_collection_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
  ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_PROTOTYPE));
} /* ecma_op_external_function_list_lazy_property_names */

/**
 * List names of a Bound Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_bound_function_try_to_lazy_instantiate_property
 */
void
ecma_op_bound_function_list_lazy_property_names (bool separate_enumerable, /**< true - list enumerable properties
                                                                            *          into main collection and
                                                                            *          non-enumerable to collection
                                                                            *          of 'skipped non-enumerable'
                                                                            *          properties,
                                                                            *   false - list all properties into
                                                                            *           main collection.
                                                                            */
                                                 ecma_collection_t *main_collection_p, /**< 'main' collection */
                                                 ecma_collection_t *non_enum_collection_p) /**< skipped
                                                                                            *   'non-enumerable'
                                                                                            *   collection */
{
  JERRY_UNUSED (main_collection_p);

  ecma_collection_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));

  /* 'caller' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_CALLER));

  /* 'arguments' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  ecma_collection_push_back (for_non_enumerable_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_ARGUMENTS));
} /* ecma_op_bound_function_list_lazy_property_names */

/**
 * @}
 * @}
 */
