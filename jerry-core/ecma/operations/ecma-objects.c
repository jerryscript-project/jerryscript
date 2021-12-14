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

#include "ecma-objects.h"

#include "ecma-arguments-object.h"
#include "ecma-array-object.h"
#include "ecma-bigint.h"
#include "ecma-bound-function.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-class-object.h"
#include "ecma-constructor-function.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-native-function.h"
#include "ecma-objects-general.h"
#include "ecma-ordinary-object.h"
#include "ecma-proxy-object.h"
#include "ecma-string-object.h"

#include "jcontext.h"

#if JERRY_BUILTIN_TYPEDARRAY
#include "ecma-arraybuffer-object.h"
#include "ecma-typedarray-object.h"
#endif /* JERRY_BUILTIN_TYPEDARRAY */

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

/**
 * Hash bitmap size for ecma objects
 */
#define ECMA_OBJECT_HASH_BITMAP_SIZE 256

/**
 * Assert that specified object type value is valid
 *
 * @param type object's implementation-defined type
 */
#ifndef JERRY_NDEBUG
#define JERRY_ASSERT_OBJECT_TYPE_IS_VALID(type) JERRY_ASSERT (type < ECMA_OBJECT_TYPE__MAX);
#else /* JERRY_NDEBUG */
#define JERRY_ASSERT_OBJECT_TYPE_IS_VALID(type)
#endif /* !JERRY_NDEBUG */

/**
 * Get the virtual function table for the given object
 */
#define ECMA_OBJECT_INTERNAL_METHOD_TABLE(obj_p) (VTABLE_CONTAINER[ecma_get_object_type (obj_p)])

/**
 * Container for every virtual table indexed by the proper object type
 */
static const ecma_internal_method_table_t VTABLE_CONTAINER[] = {
  ECMA_ORDINARY_OBJ_VTABLE,
  ECMA_BUILT_IN_OBJ_VTABLE,
  ECMA_CLASS_OBJ_VTABLE,
  ECMA_BUILT_IN_CLASS_OBJ_VTABLE,
  ECMA_ARRAY_OBJ_VTABLE,
  ECMA_BUILT_IN_ARRAY_OBJ_VTABLE,
#if JERRY_BUILTIN_PROXY
  ECMA_PROXY_OBJ_VTABLE,
#endif /* JERRY_BUILTIN_PROXY */
  ECMA_FUNCTION_OBJ_VTABLE,
  ECMA_BUILT_IN_FUNCTION_OBJ_VTABLE,
  ECMA_BOUND_FUNCTION_OBJ_VTABLE,
#if JERRY_ESNEXT
  ECMA_CONSTRUCTOR_FUNCTION_OBJ_VTABLE,
#endif /* JERRY_ESNEXT */
  ECMA_NATIVE_FUNCTION_OBJ_VTABLE,
};

/**
 * Helper function for calling the given object's [[GetPrototypeOf]] internal method
 *
 * @return ecma_object_t *
 */
extern inline ecma_object_t *JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_get_prototype_of (ecma_object_t *obj_p) /**< object */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).get_prototype_of (obj_p);
} /* ecma_internal_method_get_prototype_of */

/**
 * Helper function for calling the given object's [[SetPrototypeOf]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_set_prototype_of (ecma_object_t *obj_p, /**< base object */
                                       ecma_value_t proto) /**< prototype object */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).set_prototype_of (obj_p, proto);
} /* ecma_internal_method_set_prototype_of */

/**
 * Helper function for calling the given object's [[IsExtensible]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_is_extensible (ecma_object_t *obj_p) /**< object */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).is_extensible (obj_p);
} /* ecma_internal_method_is_extensible */

/**
 * Helper function for calling the given object's [[PreventExtensions]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_prevent_extensions (ecma_object_t *obj_p) /**< object */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).prevent_extensions (obj_p);
} /* ecma_internal_method_prevent_extensions */

/**
 * Helper function for calling the given object's [[GetOwnProperty]] internal method
 *
 * @return ecma property reference
 */
extern inline ecma_property_descriptor_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_get_own_property (ecma_object_t *obj_p, /**< the object */
                                       ecma_string_t *property_name_p) /**< property name */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).get_own_property (obj_p, property_name_p);
} /* ecma_internal_method_get_own_property */

/**
 * Helper function for calling the given object's [[DefineOwnProperty]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_define_own_property (ecma_object_t *obj_p, /**< the object */
                                          ecma_string_t *property_name_p, /**< property name */
                                          const ecma_property_descriptor_t *property_desc_p) /**< property
                                                                                              *   descriptor */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).define_own_property (obj_p, property_name_p, property_desc_p);
} /* ecma_internal_method_define_own_property */

/**
 * Helper function for calling the given object's [[HasProperty]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_has_property (ecma_object_t *obj_p, /**< the object */
                                   ecma_string_t *property_name_p) /**< property name */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).has_property (obj_p, property_name_p);
} /* ecma_internal_method_has_property */

/**
 * Helper function for calling the given object's [[Get]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_get (ecma_object_t *obj_p, /**< target object */
                          ecma_string_t *property_name_p, /**< property name */
                          ecma_value_t base_value) /**< base value */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).get (obj_p, property_name_p, base_value);
} /* ecma_internal_method_get */

/**
 * Helper function for calling the given object's [[Set]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_set (ecma_object_t *obj_p, /**< the object */
                          ecma_string_t *property_name_p, /**< property name */
                          ecma_value_t value, /**< ecma value */
                          ecma_value_t receiver, /**< receiver */
                          bool is_throw) /**< flag that controls failure handling */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).set (obj_p, property_name_p, value, receiver, is_throw);
} /* ecma_internal_method_set */

/**
 * Helper function for calling the given object's [[Delete]] internal method
 *
 * @return ecma value t
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_delete (ecma_object_t *obj_p, /**< the object */
                             ecma_string_t *property_name_p, /**< property name */
                             bool is_strict) /**< flag that controls failure handling */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).delete(obj_p, property_name_p, is_strict);
} /* ecma_internal_method_delete */

/**
 * Helper function for calling the given object's [[OwnPropertyKeys]] internal method
 *
 * @return ecma collection
 */
extern inline ecma_collection_t *JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_own_property_keys (ecma_object_t *obj_p, /**< object */
                                        jerry_property_filter_t filter) /**< name filters */
{
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).own_property_keys (obj_p, filter);
} /* ecma_internal_method_own_property_keys */

/**
 * @return the result of the function call.
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_call (ecma_object_t *obj_p, /**< function object */
                           ecma_value_t this_value, /**< 'this' argument's value */
                           const ecma_value_t *arguments_list_p, /**< arguments list */
                           uint32_t arguments_list_len) /**< length of arguments list */
{
  ECMA_CHECK_STACK_USAGE ();
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).call (obj_p, this_value, arguments_list_p, arguments_list_len);
} /* ecma_internal_method_call */

/**
 * @return the result of the function call.
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_validated_call (ecma_value_t callee, /**< callee */
                                     ecma_value_t this_value, /**< 'this' argument's value */
                                     const ecma_value_t *arguments_list_p, /**< arguments list */
                                     uint32_t arguments_list_len) /**< length of arguments list */
{
  if (!ecma_is_value_object (callee))
  {
    return ecma_raise_type_error (ECMA_ERR_EXPECTED_A_FUNCTION);
  }

  ecma_object_t *callee_p = ecma_get_object_from_value (callee);

  return ecma_internal_method_call (callee_p, this_value, arguments_list_p, arguments_list_len);
} /* ecma_internal_method_validated_call */

/**
 * @return the result of the function call.
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_construct (ecma_object_t *obj_p, /**< function object */
                                ecma_object_t *new_target_p, /**< 'this' argument's value */
                                const ecma_value_t *arguments_list_p, /**< arguments list */
                                uint32_t arguments_list_len) /**< length of arguments list */
{
  ECMA_CHECK_STACK_USAGE ();
  return ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).construct (obj_p,
                                                              new_target_p,
                                                              arguments_list_p,
                                                              arguments_list_len);
} /* ecma_internal_method_construct */

/**
 * Helper function for calling the given object's [[Delete]] internal method
 *
 * @return ecma value t
 */
extern inline void JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_list_lazy_property_keys (ecma_object_t *obj_p, /**< a built-in object */
                                              ecma_collection_t *prop_names_p, /**< prop name collection */
                                              ecma_property_counter_t *prop_counter_p, /**< property counters */
                                              jerry_property_filter_t filter) /**< name filters */
{
  ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).list_lazy_property_keys (obj_p, prop_names_p, prop_counter_p, filter);
} /* ecma_internal_method_list_lazy_property_keys */

/**
 * Helper function for calling the given object's [[Delete]] internal method
 *
 * @return ecma value t
 */
extern inline void JERRY_ATTR_ALWAYS_INLINE
ecma_internal_method_delete_lazy_property (ecma_object_t *obj_p, /**< the object */
                                           ecma_string_t *property_name_p) /**< property name */
{
  ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).delete_lazy_property (obj_p, property_name_p);
} /* ecma_internal_method_delete_lazy_property */

/**
 * Helper function for calling the given object's find method
 *
 * @return ecma value t
 */
ecma_value_t
ecma_op_object_find (ecma_object_t *obj_p, /**< target object */
                     ecma_string_t *property_name_p, /**< property name */
                     ecma_value_t receiver) /**< base value */
{
  ecma_property_descriptor_t prop_desc;
  ecma_internal_has_property_cb_t has_cb = ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).has_property;

  while (has_cb == ecma_ordinary_object_has_property)
  {
    /* 2. */
    prop_desc = ecma_internal_method_get_own_property (obj_p, property_name_p);

    if (ecma_property_descriptor_error (&prop_desc))
    {
      return ECMA_VALUE_ERROR;
    }

    /* 3. */
    if (ecma_property_descriptor_found (&prop_desc))
    {
      return ecma_property_descriptor_get (&prop_desc, receiver);
    }

    /* 3.a */
    jmem_cpointer_t proto_cp = ecma_object_get_prototype_of (obj_p);

    /* 3.b */
    if (proto_cp == JMEM_CP_NULL)
    {
      return ECMA_VALUE_NOT_FOUND;
    }

    /* 3.c */
    obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);
    has_cb = ECMA_OBJECT_INTERNAL_METHOD_TABLE (obj_p).has_property;
  }

  JERRY_ASSERT (has_cb);
  ecma_value_t has_result = has_cb (obj_p, property_name_p);

  if (ecma_is_value_true (has_result))
  {
    return ecma_internal_method_get (obj_p, property_name_p, ecma_make_object_value (obj_p));
  }

  if (ecma_is_value_false (has_result))
  {
    return ECMA_VALUE_NOT_FOUND;
  }

  return has_result;
} /* ecma_op_object_find */

/**
 * Search the value corresponding to a property index
 *
 * Note: this method falls back to the general find method
 *
 * @return ecma value if property is found
 *         ECMA_VALUE_NOT_FOUND if property is not found
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_find_by_index (ecma_object_t *object_p, /**< the object */
                              ecma_length_t index) /**< property index */
{
  ecma_value_t obj_value = ecma_make_object_value (object_p);

  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_op_object_find (object_p, ECMA_CREATE_DIRECT_UINT32_STRING (index), obj_value);
  }

  ecma_string_t *index_str_p = ecma_new_ecma_string_from_length (index);
  ecma_value_t ret_value = ecma_op_object_find (object_p, index_str_p, obj_value);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_find_by_index */

/**
 * [[Get]] operation of ecma object specified for property index
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_get_by_index (ecma_object_t *object_p, /**< the object */
                             ecma_length_t index) /**< property index */
{
  ecma_value_t obj_value = ecma_make_object_value (object_p);

  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_internal_method_get (object_p, ECMA_CREATE_DIRECT_UINT32_STRING (index), obj_value);
  }

  ecma_string_t *index_str_p = ecma_new_ecma_string_from_length (index);
  ecma_value_t ret_value = ecma_internal_method_get (object_p, index_str_p, obj_value);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_get_by_index */

/**
 * Perform ToLength(O.[[Get]]("length")) operation
 *
 * The property is converted to uint32 during the operation
 *
 * @return ECMA_VALUE_ERROR - if there was any error during the operation
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
ecma_op_object_get_length (ecma_object_t *object_p, /**< the object */
                           ecma_length_t *length_p) /**< [out] length value converted to uint32 */
{
  if (JERRY_LIKELY (ecma_get_object_base_type (object_p) == ECMA_OBJECT_BASE_TYPE_ARRAY))
  {
    *length_p = (ecma_length_t) ecma_array_get_length (object_p);
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t len_value = ecma_op_object_get_by_magic_id (object_p, LIT_MAGIC_STRING_LENGTH);
  ecma_value_t len_number = ecma_op_to_length (len_value, length_p);
  ecma_free_value (len_value);

  JERRY_ASSERT (ECMA_IS_VALUE_ERROR (len_number) || ecma_is_value_empty (len_number));

  return len_number;
} /* ecma_op_object_get_length */

/**
 * [[Get]] operation of ecma object where the property name is a magic string
 *
 * This function returns the value of a named property, or undefined
 * if the property is not found in the prototype chain. If the property
 * is an accessor, it calls the "get" callback function and returns
 * with its result (including error throws).
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_get_by_magic_id (ecma_object_t *object_p, /**< the object */
                                lit_magic_string_id_t property_id) /**< property magic string id */
{
  return ecma_internal_method_get (object_p, ecma_get_magic_string (property_id), ecma_make_object_value (object_p));
} /* ecma_op_object_get_by_magic_id */

#if JERRY_ESNEXT

/**
 * Descriptor string for each global symbol
 */
static const uint16_t ecma_global_symbol_descriptions[] = {
  LIT_MAGIC_STRING_ASYNC_ITERATOR, LIT_MAGIC_STRING_HAS_INSTANCE,  LIT_MAGIC_STRING_IS_CONCAT_SPREADABLE,
  LIT_MAGIC_STRING_ITERATOR,       LIT_MAGIC_STRING_MATCH,         LIT_MAGIC_STRING_REPLACE,
  LIT_MAGIC_STRING_SEARCH,         LIT_MAGIC_STRING_SPECIES,       LIT_MAGIC_STRING_SPLIT,
  LIT_MAGIC_STRING_TO_PRIMITIVE,   LIT_MAGIC_STRING_TO_STRING_TAG, LIT_MAGIC_STRING_UNSCOPABLES,
  LIT_MAGIC_STRING_MATCH_ALL,
};

JERRY_STATIC_ASSERT (sizeof (ecma_global_symbol_descriptions) / sizeof (uint16_t) == ECMA_BUILTIN_GLOBAL_SYMBOL_COUNT,
                     ecma_global_symbol_descriptions_must_have_global_symbol_count_elements);

/**
 * [[Get]] a well-known symbol by the given property id
 *
 * @return pointer to the requested well-known symbol
 */
ecma_string_t *
ecma_op_get_global_symbol (lit_magic_string_id_t property_id) /**< property symbol id */
{
  JERRY_ASSERT (LIT_IS_GLOBAL_SYMBOL (property_id));

  uint32_t symbol_index = (uint32_t) property_id - (uint32_t) LIT_GLOBAL_SYMBOL__FIRST;
  jmem_cpointer_t symbol_cp = JERRY_CONTEXT (global_symbols_cp)[symbol_index];

  if (symbol_cp != JMEM_CP_NULL)
  {
    ecma_string_t *symbol_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, symbol_cp);
    ecma_ref_ecma_string (symbol_p);
    return symbol_p;
  }

  ecma_string_t *symbol_dot_p = ecma_get_magic_string (LIT_MAGIC_STRING_SYMBOL_DOT_UL);
  uint16_t description = ecma_global_symbol_descriptions[symbol_index];
  ecma_string_t *name_p = ecma_get_magic_string ((lit_magic_string_id_t) description);
  ecma_string_t *descriptor_p = ecma_concat_ecma_strings (symbol_dot_p, name_p);

  ecma_string_t *symbol_p = ecma_new_symbol_from_descriptor_string (ecma_make_string_value (descriptor_p));
  symbol_p->u.hash = (uint16_t) ((property_id << ECMA_SYMBOL_FLAGS_SHIFT) | ECMA_SYMBOL_FLAG_GLOBAL);

  ECMA_SET_NON_NULL_POINTER (JERRY_CONTEXT (global_symbols_cp)[symbol_index], symbol_p);

  ecma_ref_ecma_string (symbol_p);
  return symbol_p;
} /* ecma_op_get_global_symbol */

/**
 * Checks whether the string equals to the global symbol.
 *
 * @return true - if the string equals to the global symbol
 *         false - otherwise
 */
bool
ecma_op_compare_string_to_global_symbol (ecma_string_t *string_p, /**< string to compare */
                                         lit_magic_string_id_t property_id) /**< property symbol id */
{
  JERRY_ASSERT (LIT_IS_GLOBAL_SYMBOL (property_id));

  uint32_t symbol_index = (uint32_t) property_id - (uint32_t) LIT_GLOBAL_SYMBOL__FIRST;
  jmem_cpointer_t symbol_cp = JERRY_CONTEXT (global_symbols_cp)[symbol_index];

  return (symbol_cp != JMEM_CP_NULL && string_p == ECMA_GET_NON_NULL_POINTER (ecma_string_t, symbol_cp));
} /* ecma_op_compare_string_to_global_symbol */

/**
 * [[Get]] operation of ecma object where the property is a well-known symbol
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_get_by_symbol_id (ecma_object_t *object_p, /**< the object */
                                 lit_magic_string_id_t property_id) /**< property symbol id */
{
  ecma_string_t *symbol_p = ecma_op_get_global_symbol (property_id);
  ecma_value_t ret_value = ecma_internal_method_get (object_p, symbol_p, ecma_make_object_value (object_p));
  ecma_deref_ecma_string (symbol_p);

  return ret_value;
} /* ecma_op_object_get_by_symbol_id */

/**
 * GetMethod operation
 *
 * See also: ECMA-262 v6, 7.3.9
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator function object - if success
 *         raised error - otherwise
 */
static ecma_value_t
ecma_op_get_method (ecma_value_t value, /**< ecma value */
                    ecma_string_t *prop_name_p) /** property name */
{
  /* 2. */
  ecma_value_t obj_value = ecma_op_to_object (value);

  if (ECMA_IS_VALUE_ERROR (obj_value))
  {
    return obj_value;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
  ecma_value_t func = ecma_internal_method_get (obj_p, prop_name_p, obj_value);
  ecma_deref_object (obj_p);

  /* 3. */
  if (ECMA_IS_VALUE_ERROR (func))
  {
    return func;
  }

  /* 4. */
  if (ecma_is_value_undefined (func) || ecma_is_value_null (func))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  /* 5. */
  if (!ecma_op_is_callable (func))
  {
    ecma_free_value (func);
    return ecma_raise_type_error (ECMA_ERR_ITERATOR_IS_NOT_CALLABLE);
  }

  /* 6. */
  return func;
} /* ecma_op_get_method */

/**
 * GetMethod operation when the property is a well-known symbol
 *
 * See also: ECMA-262 v6, 7.3.9
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator function object - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_get_method_by_symbol_id (ecma_value_t value, /**< ecma value */
                                 lit_magic_string_id_t symbol_id) /**< property symbol id */
{
  ecma_string_t *prop_name_p = ecma_op_get_global_symbol (symbol_id);
  ecma_value_t ret_value = ecma_op_get_method (value, prop_name_p);
  ecma_deref_ecma_string (prop_name_p);

  return ret_value;
} /* ecma_op_get_method_by_symbol_id */

/**
 * GetMethod operation when the property is a magic string
 *
 * See also: ECMA-262 v6, 7.3.9
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator function object - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_get_method_by_magic_id (ecma_value_t value, /**< ecma value */
                                lit_magic_string_id_t magic_id) /**< property magic id */
{
  return ecma_op_get_method (value, ecma_get_magic_string (magic_id));
} /* ecma_op_get_method_by_magic_id */
#endif /* JERRY_ESNEXT */

/**
 * [[Put]] ecma general object's operation specialized for property index
 *
 * Note: This function falls back to the general ecma_op_object_put
 *
 * @return ecma value
 *         The returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_object_put_by_index (ecma_object_t *object_p, /**< the object */
                             ecma_length_t index, /**< property index */
                             ecma_value_t value, /**< ecma value */
                             bool is_throw) /**< flag that controls failure handling */
{
  ecma_value_t obj_value = ecma_make_object_value (object_p);

  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_internal_method_set (object_p, ECMA_CREATE_DIRECT_UINT32_STRING (index), value, obj_value, is_throw);
  }

  ecma_string_t *index_str_p = ecma_new_ecma_string_from_length (index);
  ecma_value_t ret_value = ecma_internal_method_set (object_p, index_str_p, value, obj_value, is_throw);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_put_by_index */

/**
 * [[Delete]] ecma object's operation specialized for property index
 *
 * Note:
 *      This method falls back to the general [[Delete]] internal method
 *
 * @return true - if deleted successfully
 *         false - or type error otherwise (based in 'is_throw')
 */
ecma_value_t
ecma_op_object_delete_by_index (ecma_object_t *obj_p, /**< the object */
                                ecma_length_t index, /**< property index */
                                bool is_throw) /**< flag that controls failure handling */
{
  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_internal_method_delete (obj_p, ECMA_CREATE_DIRECT_UINT32_STRING (index), is_throw);
  }

  ecma_string_t *index_str_p = ecma_new_ecma_string_from_length (index);
  ecma_value_t ret_value = ecma_internal_method_delete (obj_p, index_str_p, is_throw);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_delete_by_index */

/**
 * [[DefaultValue]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_default_value (ecma_object_t *obj_p, /**< the object */
                              ecma_preferred_type_hint_t hint) /**< hint on preferred result type */
{
  JERRY_ASSERT (obj_p != NULL && !ecma_is_lexical_environment (obj_p));

  JERRY_ASSERT_OBJECT_TYPE_IS_VALID (ecma_get_object_type (obj_p));

  /*
   * typedef ecma_property_t * (*default_value_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const default_value_ptr_t default_value [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_CLASS]             = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_NATIVE_FUNCTION]   = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_PSEUDO_ARRAY]      = &ecma_op_general_object_default_value
   * };
   *
   * return default_value[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_default_value (obj_p, hint);
} /* ecma_op_object_default_value */

/**
 * [[HasInstance]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 9
 *
 * @return ecma value containing a boolean value or an error
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_has_instance (ecma_object_t *obj_p, /**< the object */
                             ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (obj_p != NULL && !ecma_is_lexical_environment (obj_p));

  JERRY_ASSERT_OBJECT_TYPE_IS_VALID (ecma_get_object_type (obj_p));

  if (ecma_op_object_is_callable (obj_p))
  {
    return ecma_op_function_has_instance (obj_p, value);
  }

  return ecma_raise_type_error (ECMA_ERR_EXPECTED_A_FUNCTION_OBJECT);
} /* ecma_op_object_has_instance */

/**
 * Object's isPrototypeOf operation
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6; 3
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_TRUE - if the target object is prototype of the base object
 *         ECMA_VALUE_FALSE - if the target object is not prototype of the base object
 */
ecma_value_t
ecma_op_object_is_prototype_of (ecma_object_t *base_p, /**< base object */
                                ecma_object_t *target_p) /**< target object */
{
  ecma_ref_object (target_p);

  do
  {
    ecma_object_t *proto_p = ecma_internal_method_get_prototype_of (target_p);
    ecma_deref_object (target_p);

    if (proto_p == NULL)
    {
      return ECMA_VALUE_FALSE;
    }
    else if (proto_p == ECMA_OBJECT_POINTER_ERROR)
    {
      return ECMA_VALUE_ERROR;
    }
    else if (proto_p == base_p)
    {
      ecma_deref_object (proto_p);
      return ECMA_VALUE_TRUE;
    }

    /* Advance up on prototype chain. */
    target_p = proto_p;
  } while (true);
} /* ecma_op_object_is_prototype_of */

/**
 * Object's EnumerableOwnPropertyNames operation
 *
 * See also:
 *          ECMA-262 v11, 7.3.23
 *
 * @return NULL - if operation fails
 *         collection of property names / values / name-value pairs - otherwise
 */
ecma_collection_t *
ecma_op_object_get_enumerable_property_names (ecma_object_t *obj_p, /**< routine's first argument */
                                              ecma_enumerable_property_names_options_t option) /**< listing option */
{
  /* 2. */
  ecma_collection_t *prop_names_p =
    ecma_internal_method_own_property_keys (obj_p, JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS);

#if JERRY_BUILTIN_PROXY
  if (JERRY_UNLIKELY (prop_names_p == NULL))
  {
    return prop_names_p;
  }
#endif /* JERRY_BUILTIN_PROXY */

  ecma_value_t *names_buffer_p = prop_names_p->buffer_p;
  /* 3. */
  ecma_collection_t *properties_p = ecma_new_collection ();

  /* 4. */
  for (uint32_t i = 0; i < prop_names_p->item_count; i++)
  {
    /* 4.a */
    if (ecma_is_value_string (names_buffer_p[i]))
    {
      ecma_string_t *key_p = ecma_get_string_from_value (names_buffer_p[i]);

      /* 4.a.i */
      ecma_property_descriptor_t prop_desc = ecma_internal_method_get_own_property (obj_p, key_p);

      if (ecma_property_descriptor_error (&prop_desc))
      {
        ecma_collection_free (prop_names_p);
        ecma_collection_free (properties_p);
        ecma_free_property_descriptor (&prop_desc);

        return NULL;
      }

      if (!ecma_property_descriptor_found (&prop_desc))
      {
        ecma_free_property_descriptor (&prop_desc);
        continue;
      }

      const bool is_enumerable = ecma_property_descriptor_is_enumerable (&prop_desc);
      ecma_free_property_descriptor (&prop_desc);
      /* 4.a.ii */
      if (is_enumerable)
      {
        /* 4.a.ii.1 */
        if (option == ECMA_ENUMERABLE_PROPERTY_KEYS)
        {
          ecma_collection_push_back (properties_p, ecma_copy_value (names_buffer_p[i]));
        }
        else
        {
          /* 4.a.ii.2.a */
          ecma_value_t value = ecma_internal_method_get (obj_p, key_p, ecma_make_object_value (obj_p));

          if (ECMA_IS_VALUE_ERROR (value))
          {
            ecma_collection_free (prop_names_p);
            ecma_collection_free (properties_p);

            return NULL;
          }

          /* 4.a.ii.2.b */
          if (option == ECMA_ENUMERABLE_PROPERTY_VALUES)
          {
            ecma_collection_push_back (properties_p, value);
          }
          else
          {
            /* 4.a.ii.2.c.i */
            JERRY_ASSERT (option == ECMA_ENUMERABLE_PROPERTY_ENTRIES);

            /* 4.a.ii.2.c.ii */
            ecma_object_t *entry_p = ecma_op_new_array_object (2);

            ecma_builtin_helper_def_prop_by_index (entry_p,
                                                   0,
                                                   names_buffer_p[i],
                                                   ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
            ecma_builtin_helper_def_prop_by_index (entry_p, 1, value, ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
            ecma_free_value (value);

            /* 4.a.ii.2.c.iii */
            ecma_collection_push_back (properties_p, ecma_make_object_value (entry_p));
          }
        }
      }
    }
  }

  ecma_collection_free (prop_names_p);

  return properties_p;
} /* ecma_op_object_get_enumerable_property_names */

/**
 * EnumerateObjectProperties abstract method
 *
 * See also:
 *          ECMA-262 v11, 13.7.5.15
 *
 * @return NULL - if the Proxy.[[OwnPropertyKeys]] operation raises error
 *         collection of enumerable property names - otherwise
 */
ecma_collection_t *
ecma_op_object_enumerate (ecma_object_t *obj_p) /**< object */
{
  ecma_collection_t *visited_names_p = ecma_new_collection ();
  ecma_collection_t *return_names_p = ecma_new_collection ();

  ecma_ref_object (obj_p);

  while (true)
  {
    ecma_collection_t *keys = ecma_internal_method_own_property_keys (obj_p, JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS);

#if JERRY_ESNEXT
    if (JERRY_UNLIKELY (keys == NULL))
    {
      ecma_collection_free (return_names_p);
      ecma_collection_free (visited_names_p);
      ecma_deref_object (obj_p);
      return keys;
    }
#endif /* JERRY_ESNEXT */

    for (uint32_t i = 0; i < keys->item_count; i++)
    {
      ecma_value_t prop_name = keys->buffer_p[i];
      ecma_string_t *name_p = ecma_get_prop_name_from_value (prop_name);

#if JERRY_ESNEXT
      if (ecma_prop_name_is_symbol (name_p))
      {
        continue;
      }
#endif /* JERRY_ESNEXT */

      ecma_property_descriptor_t prop_desc = ecma_internal_method_get_own_property (obj_p, name_p);

      if (ecma_property_descriptor_error (&prop_desc))
      {
        ecma_collection_free (keys);
        ecma_collection_free (return_names_p);
        ecma_collection_free (visited_names_p);
        ecma_deref_object (obj_p);
        ecma_free_property_descriptor (&prop_desc);

        return NULL;
      }

      if (ecma_property_descriptor_found (&prop_desc))
      {
        bool is_enumerable = ecma_property_descriptor_is_enumerable (&prop_desc);

        if (ecma_collection_has_string_value (visited_names_p, name_p)
            || ecma_collection_has_string_value (return_names_p, name_p))
        {
          ecma_free_property_descriptor (&prop_desc);
          continue;
        }

        ecma_ref_ecma_string (name_p);

        if (is_enumerable)
        {
          ecma_collection_push_back (return_names_p, prop_name);
        }
        else
        {
          ecma_collection_push_back (visited_names_p, prop_name);
        }
      }

      ecma_free_property_descriptor (&prop_desc);
    }

    ecma_collection_free (keys);

    /* Query the prototype. */
    ecma_object_t *proto_p = ecma_internal_method_get_prototype_of (obj_p);
    ecma_deref_object (obj_p);

    if (proto_p == NULL)
    {
      break;
    }
    else if (JERRY_UNLIKELY (proto_p == ECMA_OBJECT_POINTER_ERROR))
    {
      ecma_collection_free (return_names_p);
      ecma_collection_free (visited_names_p);
      return NULL;
    }

    /* Advance up on prototype chain. */
    obj_p = proto_p;
  }

  ecma_collection_free (visited_names_p);

  return return_names_p;
} /* ecma_op_object_enumerate */

#ifndef JERRY_NDEBUG

/**
 * Check if passed object is the instance of specified built-in.
 *
 * @return true  - if the object is instance of the specified built-in
 *         false - otherwise
 */
static bool
ecma_builtin_is (ecma_object_t *object_p, /**< pointer to an object */
                 ecma_builtin_id_t builtin_id) /**< id of built-in to check on */
{
  JERRY_ASSERT (object_p != NULL && !ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (builtin_id < ECMA_BUILTIN_ID__COUNT);

  ecma_object_type_t type = ecma_get_object_type (object_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_BUILT_IN_GENERAL:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      ecma_extended_object_t *built_in_object_p = (ecma_extended_object_t *) object_p;

      return (built_in_object_p->u.built_in.id == builtin_id && built_in_object_p->u.built_in.routine_id == 0);
    }
    case ECMA_OBJECT_TYPE_BUILT_IN_CLASS:
    case ECMA_OBJECT_TYPE_BUILT_IN_ARRAY:
    {
      ecma_extended_built_in_object_t *extended_built_in_object_p = (ecma_extended_built_in_object_t *) object_p;

      return (extended_built_in_object_p->built_in.id == builtin_id
              && extended_built_in_object_p->built_in.routine_id == 0);
    }
    default:
    {
      return false;
    }
  }
} /* ecma_builtin_is */

#endif /* !JERRY_NDEBUG */

/**
 * The function is used in the assert of ecma_object_get_class_name
 *
 * @return true  - if class name is an object
 *         false - otherwise
 */
static inline bool
ecma_object_check_class_name_is_object (ecma_object_t *obj_p) /**< object */
{
#ifndef JERRY_NDEBUG
  return (ecma_builtin_is_global (obj_p)
#if JERRY_BUILTIN_TYPEDARRAY
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE)
#if JERRY_BUILTIN_SHAREDARRAYBUFFER
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SHARED_ARRAYBUFFER_PROTOTYPE)
#endif /* JERRY_BUILTIN_SHAREDARRAYBUFFER */
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT8ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_INT16ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT16ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_INT32ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT32ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_FLOAT32ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY_PROTOTYPE)
#if JERRY_NUMBER_TYPE_FLOAT64
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_FLOAT64ARRAY_PROTOTYPE)
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */
#if JERRY_BUILTIN_BIGINT
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_BIGINT_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_BIGINT64ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_BIGUINT64ARRAY_PROTOTYPE)
#endif /* JERRY_BUILTIN_BIGINT */
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_ESNEXT
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ARRAY_PROTOTYPE_UNSCOPABLES)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ARRAY_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_STRING_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_REGEXP_STRING_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_GENERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_AGGREGATE_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ERROR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_DATE_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SYMBOL_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ASYNC_FUNCTION_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_PROMISE_PROTOTYPE)
#endif /* JERRY_ESNEXT */
#if JERRY_BUILTIN_CONTAINER
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_MAP_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SET_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_WEAKMAP_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_WEAKSET_PROTOTYPE)
#if JERRY_ESNEXT
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SET_ITERATOR_PROTOTYPE)
#endif /* JERRY_ESNEXT */
#endif /* JERRY_BUILTIN_CONTAINER */
#if JERRY_BUILTIN_WEAKREF
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_WEAKREF_PROTOTYPE)
#endif /* JERRY_BUILTIN_WEAKREF */
#if JERRY_BUILTIN_DATAVIEW
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_DATAVIEW_PROTOTYPE)
#endif /* JERRY_BUILTIN_DATAVIEW */
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE));
#else /* JERRY_NDEBUG */
  JERRY_UNUSED (obj_p);
  return true;
#endif /* !JERRY_NDEBUG */
} /* ecma_object_check_class_name_is_object */

/**
 * Used by ecma_object_get_class_name to get the magic string id of class objects
 */
static const uint16_t ecma_class_object_magic_string_id[] = {
/* These objects require custom property resolving. */
#if JERRY_BUILTIN_TYPEDARRAY
  LIT_MAGIC_STRING__EMPTY, /**< ECMA_OBJECT_CLASS_TYPEDARRAY needs special resolver */
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_MODULE_SYSTEM
  LIT_MAGIC_STRING_MODULE_UL, /**< magic string id of ECMA_OBJECT_CLASS_MODULE_NAMESPACE */
#endif /* JERRY_MODULE_SYSTEM */
  LIT_MAGIC_STRING_STRING_UL, /**< magic string id of ECMA_OBJECT_CLASS_STRING */
  LIT_MAGIC_STRING_ARGUMENTS_UL, /**< magic string id of ECMA_OBJECT_CLASS_ARGUMENTS */

/* These objects are marked by Garbage Collector. */
#if JERRY_ESNEXT
  LIT_MAGIC_STRING_GENERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_GENERATOR */
  LIT_MAGIC_STRING_ASYNC_GENERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_ASYNC_GENERATOR */
  LIT_MAGIC_STRING_ARRAY_ITERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_ARRAY_ITERATOR */
  LIT_MAGIC_STRING_SET_ITERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_SET_ITERATOR */
  LIT_MAGIC_STRING_MAP_ITERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_MAP_ITERATOR */
#if JERRY_BUILTIN_REGEXP
  LIT_MAGIC_STRING_REGEXP_STRING_ITERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_REGEXP_STRING_ITERATOR */
#endif /* JERRY_BUILTIN_REGEXP */
#endif /* JERRY_ESNEXT */
#if JERRY_MODULE_SYSTEM
  LIT_MAGIC_STRING_MODULE_UL, /**< magic string id of ECMA_OBJECT_CLASS_MODULE */
#endif /* JERRY_MODULE_SYSTEM */
#if JERRY_ESNEXT
  LIT_MAGIC_STRING_PROMISE_UL, /**< magic string id of ECMA_OBJECT_CLASS_PROMISE */
  LIT_MAGIC_STRING_OBJECT_UL, /**< magic string id of ECMA_OBJECT_CLASS_PROMISE_CAPABILITY */
  LIT_MAGIC_STRING_OBJECT_UL, /**< magic string id of ECMA_OBJECT_CLASS_ASYNC_FROM_SYNC_ITERATOR */
#endif /* JERRY_ESNEXT */
#if JERRY_BUILTIN_DATAVIEW
  LIT_MAGIC_STRING_DATAVIEW_UL, /**< magic string id of ECMA_OBJECT_CLASS_DATAVIEW */
#endif /* JERRY_BUILTIN_DATAVIEW */
#if JERRY_BUILTIN_CONTAINER
  LIT_MAGIC_STRING__EMPTY, /**< magic string id of ECMA_OBJECT_CLASS_CONTAINER needs special resolver */
#endif /* JERRY_BUILTIN_CONTAINER */

  /* Normal objects. */
  LIT_MAGIC_STRING_BOOLEAN_UL, /**< magic string id of ECMA_OBJECT_CLASS_BOOLEAN */
  LIT_MAGIC_STRING_NUMBER_UL, /**< magic string id of ECMA_OBJECT_CLASS_NUMBER */
  LIT_MAGIC_STRING_ERROR_UL, /**< magic string id of ECMA_OBJECT_CLASS_ERROR */
  LIT_MAGIC_STRING_OBJECT_UL, /**< magic string id of ECMA_OBJECT_CLASS_INTERNAL_OBJECT */
#if JERRY_PARSER
  LIT_MAGIC_STRING_SCRIPT_UL, /**< magic string id of ECMA_OBJECT_CLASS_SCRIPT */
#endif /* JERRY_PARSER */
#if JERRY_BUILTIN_DATE
  LIT_MAGIC_STRING_DATE_UL, /**< magic string id of ECMA_OBJECT_CLASS_DATE */
#endif /* JERRY_BUILTIN_DATE */
#if JERRY_BUILTIN_REGEXP
  LIT_MAGIC_STRING_REGEXP_UL, /**< magic string id of ECMA_OBJECT_CLASS_REGEXP */
#endif /* JERRY_BUILTIN_REGEXP */
#if JERRY_ESNEXT
  LIT_MAGIC_STRING_SYMBOL_UL, /**< magic string id of ECMA_OBJECT_CLASS_SYMBOL */
  LIT_MAGIC_STRING_STRING_ITERATOR_UL, /**< magic string id of ECMA_OBJECT_CLASS_STRING_ITERATOR */
#endif /* JERRY_ESNEXT */
#if JERRY_BUILTIN_TYPEDARRAY
  LIT_MAGIC_STRING_ARRAY_BUFFER_UL, /**< magic string id of ECMA_OBJECT_CLASS_ARRAY_BUFFER */
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_BUILTIN_SHAREDARRAYBUFFER
  LIT_MAGIC_STRING_SHARED_ARRAY_BUFFER_UL, /**< magic string id of ECMA_OBJECT_CLASS_SHAREDARRAY_BUFFER */
#endif /* JERRY_BUILTIN_SHAREDARRAYBUFFER */
#if JERRY_BUILTIN_BIGINT
  LIT_MAGIC_STRING_BIGINT_UL, /**< magic string id of ECMA_OBJECT_CLASS_BIGINT */
#endif /* JERRY_BUILTIN_BIGINT */
#if JERRY_BUILTIN_WEAKREF
  LIT_MAGIC_STRING_WEAKREF_UL, /**< magic string id of ECMA_OBJECT_CLASS_WEAKREF */
#endif /* JERRY_BUILTIN_WEAKREF */
};

JERRY_STATIC_ASSERT (sizeof (ecma_class_object_magic_string_id) == ECMA_OBJECT_CLASS__MAX * sizeof (uint16_t),
                     ecma_class_object_magic_string_id_must_have_object_class_max_elements);

/**
 * Get [[Class]] string of specified object
 *
 * @return class name magic string
 */
lit_magic_string_id_t
ecma_object_get_class_name (ecma_object_t *obj_p) /**< object */
{
  ecma_object_type_t type = ecma_get_object_type (obj_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_BUILT_IN_ARRAY:
    {
      return LIT_MAGIC_STRING_ARRAY_UL;
    }
    case ECMA_OBJECT_TYPE_CLASS:
    case ECMA_OBJECT_TYPE_BUILT_IN_CLASS:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      switch (ext_object_p->u.cls.type)
      {
#if JERRY_BUILTIN_TYPEDARRAY
        case ECMA_OBJECT_CLASS_TYPEDARRAY:
        {
          return ecma_get_typedarray_magic_string_id (ext_object_p->u.cls.u1.typedarray_type);
        }
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_BUILTIN_CONTAINER
        case ECMA_OBJECT_CLASS_CONTAINER:
        {
          return (lit_magic_string_id_t) ext_object_p->u.cls.u2.container_id;
        }
#endif /* JERRY_BUILTIN_CONTAINER */
        default:
        {
          break;
        }
      }

      JERRY_ASSERT (ext_object_p->u.cls.type < ECMA_OBJECT_CLASS__MAX);
      JERRY_ASSERT (ecma_class_object_magic_string_id[ext_object_p->u.cls.type] != LIT_MAGIC_STRING__EMPTY);

      return (lit_magic_string_id_t) ecma_class_object_magic_string_id[ext_object_p->u.cls.type];
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_NATIVE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    case ECMA_OBJECT_TYPE_CONSTRUCTOR_FUNCTION:
    {
      return LIT_MAGIC_STRING_FUNCTION_UL;
    }
#if JERRY_BUILTIN_PROXY
    case ECMA_OBJECT_TYPE_PROXY:
    {
      ecma_proxy_object_t *proxy_obj_p = (ecma_proxy_object_t *) obj_p;

      if (!ecma_is_value_null (proxy_obj_p->target) && ecma_is_value_object (proxy_obj_p->target))
      {
        ecma_object_t *target_obj_p = ecma_get_object_from_value (proxy_obj_p->target);
        return ecma_object_get_class_name (target_obj_p);
      }
      return LIT_MAGIC_STRING_OBJECT_UL;
    }
#endif /* JERRY_BUILTIN_PROXY */
    case ECMA_OBJECT_TYPE_BUILT_IN_GENERAL:
    {
      ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

      switch (ext_obj_p->u.built_in.id)
      {
#if JERRY_BUILTIN_MATH
        case ECMA_BUILTIN_ID_MATH:
        {
          return LIT_MAGIC_STRING_MATH_UL;
        }
#endif /* JERRY_BUILTIN_MATH */
#if JERRY_BUILTIN_REFLECT
        case ECMA_BUILTIN_ID_REFLECT:
        {
          return LIT_MAGIC_STRING_REFLECT_UL;
        }
#endif /* JERRY_BUILTIN_REFLECT */
#if JERRY_ESNEXT
        case ECMA_BUILTIN_ID_GENERATOR:
        {
          return LIT_MAGIC_STRING_GENERATOR_UL;
        }
        case ECMA_BUILTIN_ID_ASYNC_GENERATOR:
        {
          return LIT_MAGIC_STRING_ASYNC_GENERATOR_UL;
        }
#endif /* JERRY_ESNEXT */
#if JERRY_BUILTIN_JSON
        case ECMA_BUILTIN_ID_JSON:
        {
          return LIT_MAGIC_STRING_JSON_U;
        }
#endif /* JERRY_BUILTIN_JSON */
#if JERRY_BUILTIN_ATOMICS
        case ECMA_BUILTIN_ID_ATOMICS:
        {
          return LIT_MAGIC_STRING_ATOMICS_U;
        }
#endif /* JERRY_BUILTIN_ATOMICS */
#if !JERRY_ESNEXT
#if JERRY_BUILTIN_ERRORS
        case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
        case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
        case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
        case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
        case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
        case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
#endif /* JERRY_BUILTIN_ERRORS */
        case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
        {
          return LIT_MAGIC_STRING_ERROR_UL;
        }
#endif /* !JERRY_ESNEXT */
        default:
        {
          break;
        }
      }

      JERRY_ASSERT (ecma_object_check_class_name_is_object (obj_p));
      return LIT_MAGIC_STRING_OBJECT_UL;
    }
    default:
    {
      JERRY_ASSERT (type == ECMA_OBJECT_TYPE_GENERAL || type == ECMA_OBJECT_TYPE_PROXY);

      return LIT_MAGIC_STRING_OBJECT_UL;
    }
  }
} /* ecma_object_get_class_name */

#if JERRY_BUILTIN_REGEXP
/**
 * Checks if the given argument has [[RegExpMatcher]] internal slot
 *
 * @return true - if the given argument is a regexp
 *         false - otherwise
 */
extern inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_object_is_regexp_object (ecma_value_t arg) /**< argument */
{
  return (ecma_is_value_object (arg)
          && ecma_object_class_is (ecma_get_object_from_value (arg), ECMA_OBJECT_CLASS_REGEXP));
} /* ecma_object_is_regexp_object */
#endif /* JERRY_BUILTIN_REGEXP */

#if JERRY_ESNEXT
/**
 * Object's IsConcatSpreadable operation, used for Array.prototype.concat
 * It checks the argument's [Symbol.isConcatSpreadable] property value
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.1.1;
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_TRUE - if the argument is concatSpreadable
 *         ECMA_VALUE_FALSE - otherwise
 */
ecma_value_t
ecma_op_is_concat_spreadable (ecma_value_t arg) /**< argument */
{
  if (!ecma_is_value_object (arg))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_value_t spreadable =
    ecma_op_object_get_by_symbol_id (ecma_get_object_from_value (arg), LIT_GLOBAL_SYMBOL_IS_CONCAT_SPREADABLE);

  if (ECMA_IS_VALUE_ERROR (spreadable))
  {
    return spreadable;
  }

  if (!ecma_is_value_undefined (spreadable))
  {
    const bool to_bool = ecma_op_to_boolean (spreadable);
    ecma_free_value (spreadable);
    return ecma_make_boolean_value (to_bool);
  }

  return ecma_is_value_array (arg);
} /* ecma_op_is_concat_spreadable */

/**
 * IsRegExp operation
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.1.1;
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_TRUE - if the argument is regexp
 *         ECMA_VALUE_FALSE - otherwise
 */
ecma_value_t
ecma_op_is_regexp (ecma_value_t arg) /**< argument */
{
  if (!ecma_is_value_object (arg))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_value_t is_regexp = ecma_op_object_get_by_symbol_id (ecma_get_object_from_value (arg), LIT_GLOBAL_SYMBOL_MATCH);

  if (ECMA_IS_VALUE_ERROR (is_regexp))
  {
    return is_regexp;
  }

  if (!ecma_is_value_undefined (is_regexp))
  {
    const bool to_bool = ecma_op_to_boolean (is_regexp);
    ecma_free_value (is_regexp);
    return ecma_make_boolean_value (to_bool);
  }

  return ecma_make_boolean_value (ecma_object_is_regexp_object (arg));
} /* ecma_op_is_regexp */

/**
 * SpeciesConstructor operation
 * See also:
 *          ECMA-262 v6, 7.3.20;
 *
 * @return ecma_value
 *         returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_species_constructor (ecma_object_t *this_value, /**< This Value */
                             ecma_builtin_id_t default_constructor_id) /**< Builtin ID of default constructor */
{
  ecma_object_t *default_constructor_p = ecma_builtin_get (default_constructor_id);
  ecma_value_t constructor = ecma_op_object_get_by_magic_id (this_value, LIT_MAGIC_STRING_CONSTRUCTOR);
  if (ECMA_IS_VALUE_ERROR (constructor))
  {
    return constructor;
  }

  if (ecma_is_value_undefined (constructor))
  {
    ecma_ref_object (default_constructor_p);
    return ecma_make_object_value (default_constructor_p);
  }

  if (!ecma_is_value_object (constructor))
  {
    ecma_free_value (constructor);
    return ecma_raise_type_error (ECMA_ERR_CONSTRUCTOR_NOT_AN_OBJECT);
  }

  ecma_object_t *ctor_object_p = ecma_get_object_from_value (constructor);
  ecma_value_t species = ecma_op_object_get_by_symbol_id (ctor_object_p, LIT_GLOBAL_SYMBOL_SPECIES);
  ecma_deref_object (ctor_object_p);

  if (ECMA_IS_VALUE_ERROR (species))
  {
    return species;
  }

  if (ecma_is_value_undefined (species) || ecma_is_value_null (species))
  {
    ecma_ref_object (default_constructor_p);
    return ecma_make_object_value (default_constructor_p);
  }

  if (!ecma_is_constructor (species))
  {
    ecma_free_value (species);
    return ecma_raise_type_error (ECMA_ERR_SPECIES_MUST_BE_A_CONSTRUCTOR);
  }

  return species;
} /* ecma_op_species_constructor */

/**
 * 7.3.18 Abstract operation Invoke when property name is a magic string
 *
 * @return ecma_value result of the invoked function or raised error
 *         note: returned value must be freed with ecma_free_value
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_invoke_by_symbol_id (ecma_value_t object, /**< Object value */
                             lit_magic_string_id_t symbol_id, /**< Symbol ID */
                             ecma_value_t *args_p, /**< Argument list */
                             uint32_t args_len) /**< Argument list length */
{
  ecma_string_t *symbol_p = ecma_op_get_global_symbol (symbol_id);
  ecma_value_t ret_value = ecma_op_invoke (object, symbol_p, args_p, args_len);
  ecma_deref_ecma_string (symbol_p);

  return ret_value;
} /* ecma_op_invoke_by_symbol_id */
#endif /* JERRY_ESNEXT */

/**
 * 7.3.18 Abstract operation Invoke when property name is a magic string
 *
 * @return ecma_value result of the invoked function or raised error
 *         note: returned value must be freed with ecma_free_value
 */
extern inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_invoke_by_magic_id (ecma_value_t object, /**< Object value */
                            lit_magic_string_id_t magic_string_id, /**< Magic string ID */
                            ecma_value_t *args_p, /**< Argument list */
                            uint32_t args_len) /**< Argument list length */
{
  return ecma_op_invoke (object, ecma_get_magic_string (magic_string_id), args_p, args_len);
} /* ecma_op_invoke_by_magic_id */

/**
 * 7.3.18 Abstract operation Invoke
 *
 * @return ecma_value result of the invoked function or raised error
 *         note: returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_invoke (ecma_value_t object, /**< Object value */
                ecma_string_t *property_name_p, /**< Property name */
                ecma_value_t *args_p, /**< Argument list */
                uint32_t args_len) /**< Argument list length */
{
  /* 3. */
  ecma_value_t object_value = ecma_op_to_object (object);
  if (ECMA_IS_VALUE_ERROR (object_value))
  {
    return object_value;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (object_value);

#if JERRY_ESNEXT
  ecma_value_t this_arg = object;
#else /* !JERRY_ESNEXT */
  ecma_value_t this_arg = object_value;
#endif /* JERRY_ESNEXT */

  ecma_value_t func = ecma_internal_method_get (object_p, property_name_p, this_arg);

  if (ECMA_IS_VALUE_ERROR (func))
  {
    ecma_deref_object (object_p);
    return func;
  }

  /* 4. */
  ecma_value_t call_result = ecma_internal_method_validated_call (func, this_arg, args_p, args_len);
  ecma_free_value (func);

  ecma_deref_object (object_p);

  return call_result;
} /* ecma_op_invoke */

/**
 * Checks whether an object (excluding prototypes) has a named property
 *
 * @return true - if property is found
 *         false - otherwise
 */
ecma_value_t
ecma_op_object_has_own_property (ecma_object_t *object_p, /**< the object */
                                 ecma_string_t *property_name_p) /**< property name */
{
  ecma_property_descriptor_t prop_desc = ecma_internal_method_get_own_property (object_p, property_name_p);

  ecma_value_t ret_value;

  if (ecma_property_descriptor_error (&prop_desc))
  {
    ret_value = ECMA_VALUE_ERROR;
  }
  else if (ecma_property_descriptor_found (&prop_desc))
  {
    ret_value = ECMA_VALUE_TRUE;
    ecma_free_property_descriptor (&prop_desc);
  }
  else
  {
    ret_value = ECMA_VALUE_FALSE;
  }

  return ret_value;
} /* ecma_op_object_has_own_property */

#if JERRY_BUILTIN_WEAKREF || JERRY_BUILTIN_CONTAINER

/**
 * Set a weak reference from a container or WeakRefObject to a key object
 */
void
ecma_op_object_set_weak (ecma_object_t *object_p, /**< key object */
                         ecma_object_t *target_p) /**< target object */
{
  if (JERRY_UNLIKELY (ecma_op_object_is_fast_array (object_p)))
  {
    ecma_fast_array_convert_to_normal (object_p);
  }

  ecma_string_t *weak_refs_string_p = ecma_get_internal_string (LIT_INTERNAL_MAGIC_STRING_WEAK_REFS);
  ecma_property_t *property_p = ecma_find_named_property (object_p, weak_refs_string_p);
  ecma_collection_t *refs_p;

  if (property_p == NULL)
  {
    refs_p = ecma_new_collection ();

    ecma_property_value_t *value_p;
    ECMA_CREATE_INTERNAL_PROPERTY (object_p, weak_refs_string_p, property_p, value_p);
    ECMA_SET_INTERNAL_VALUE_POINTER (value_p->value, refs_p);
  }
  else
  {
    refs_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, (ECMA_PROPERTY_VALUE_PTR (property_p)->value));
  }

  const ecma_value_t target_value = ecma_make_object_value ((ecma_object_t *) target_p);
  for (uint32_t i = 0; i < refs_p->item_count; i++)
  {
    if (ecma_is_value_empty (refs_p->buffer_p[i]))
    {
      refs_p->buffer_p[i] = target_value;
      return;
    }
  }

  ecma_collection_push_back (refs_p, target_value);
} /* ecma_op_object_set_weak */

/**
 * Helper function to remove a weak reference to an object.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
void
ecma_op_object_unref_weak (ecma_object_t *object_p, /**< this argument */
                           ecma_value_t ref_holder) /**< key argument */
{
  ecma_string_t *weak_refs_string_p = ecma_get_internal_string (LIT_INTERNAL_MAGIC_STRING_WEAK_REFS);

  ecma_property_t *property_p = ecma_find_named_property (object_p, weak_refs_string_p);
  JERRY_ASSERT (property_p != NULL);

  ecma_collection_t *refs_p =
    ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  ecma_value_t *buffer_p = refs_p->buffer_p;

  while (true)
  {
    if (*buffer_p == ref_holder)
    {
      *buffer_p = ECMA_VALUE_EMPTY;
      return;
    }
    JERRY_ASSERT (buffer_p < refs_p->buffer_p + refs_p->item_count);
    buffer_p++;
  }
} /* ecma_op_object_unref_weak */

#endif /* JERRY_BUILTIN_WEAKREF || JERRY_BUILTIN_CONTAINER */

/**
 * Raise property redefinition error
 *
 * @return ECMA_VALUE_FALSE - if JERRY_PROP_SHOULD_THROW is not set
 *         raised TypeError - otherwise
 */
ecma_value_t
ecma_raise_property_redefinition (ecma_string_t *property_name_p, /**< property name */
                                  uint32_t flags) /**< property descriptor flags */
{
  JERRY_UNUSED (property_name_p);

  return ECMA_REJECT_WITH_FORMAT (flags & JERRY_PROP_SHOULD_THROW,
                                  "Cannot redefine property: %",
                                  ecma_make_prop_name_value (property_name_p));
} /* ecma_raise_property_redefinition */

/**
 * Raise readonly assignment error
 *
 * @return ECMA_VALUE_FALSE - if is_throw is true
 *         raised TypeError - otherwise
 */
ecma_value_t
ecma_raise_readonly_assignment (ecma_string_t *property_name_p, /**< property name */
                                bool is_throw) /**< is throw flag */
{
  JERRY_UNUSED (property_name_p);

  return ECMA_REJECT_WITH_FORMAT (is_throw,
                                  "Cannot assign to read only property '%'",
                                  ecma_make_prop_name_value (property_name_p));
} /* ecma_raise_readonly_assignment */

/**
 * Raise non confugurable property error
 *
 * @return ECMA_VALUE_FALSE - if is_throw is true
 *         raised TypeError - otherwise
 */
ecma_value_t
ecma_raise_non_configurable_property (ecma_string_t *property_name_p, /**< property name */
                                      bool is_throw) /**< is throw flag */
{
  JERRY_UNUSED (property_name_p);

  return ECMA_REJECT_WITH_FORMAT (is_throw,
                                  "property: % is not configurable",
                                  ecma_make_prop_name_value (property_name_p));
} /* ecma_raise_non_configurable_property */

/**
 * CanonicalNumericIndexString
 *
 * Checks whether the property name is a valid element index
 *
 * @return true, if valid
 *         false, otherwise
 */
bool
ecma_op_canonical_numeric_string (ecma_string_t *property_name_p) /**< property name */
{
  ecma_number_t num = ecma_string_to_number (property_name_p);

  if (num == 0)
  {
    return true;
  }

  ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (num);
  bool is_same = ecma_compare_ecma_strings (property_name_p, num_to_str);
  ecma_deref_ecma_string (num_to_str);
  return is_same;
} /* ecma_op_canonical_numeric_string */

/**
 * @}
 * @}
 */
