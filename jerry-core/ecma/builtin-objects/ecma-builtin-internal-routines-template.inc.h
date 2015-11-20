/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef BUILTIN_UNDERSCORED_ID
# error "Please, define BUILTIN_UNDERSCORED_ID"
#endif /* !BUILTIN_UNDERSCORED_ID */

#ifndef BUILTIN_INC_HEADER_NAME
# error "Please, define BUILTIN_INC_HEADER_NAME"
#endif /* !BUILTIN_INC_HEADER_NAME */

#include "ecma-objects.h"

#define PASTE__(x, y) x ## y
#define PASTE_(x, y) PASTE__ (x, y)
#define PASTE(x, y) PASTE_ (x, y)

#define SORT_PROPERTY_NAMES_ROUTINE_NAME(builtin_underscored_id) \
  PASTE (PASTE (ecma_builtin_, builtin_underscored_id), _sort_property_names)
#define TRY_TO_INSTANTIATE_PROPERTY_ROUTINE_NAME(builtin_underscored_id) \
  PASTE (PASTE (ecma_builtin_, builtin_underscored_id), _try_to_instantiate_property)
#define LIST_LAZY_PROPERTY_NAMES_ROUTINE_NAME(builtin_underscored_id) \
  PASTE (PASTE (ecma_builtin_, builtin_underscored_id), _list_lazy_property_names)
#define DISPATCH_ROUTINE_ROUTINE_NAME(builtin_underscored_id) \
  PASTE (PASTE (ecma_builtin_, builtin_underscored_id), _dispatch_routine)

#define ROUTINE_ARG(n) , ecma_value_t arg ## n
#define ROUTINE_ARG_LIST_0 ecma_value_t this_arg
#define ROUTINE_ARG_LIST_1 ROUTINE_ARG_LIST_0 ROUTINE_ARG(1)
#define ROUTINE_ARG_LIST_2 ROUTINE_ARG_LIST_1 ROUTINE_ARG(2)
#define ROUTINE_ARG_LIST_3 ROUTINE_ARG_LIST_2 ROUTINE_ARG(3)
#define ROUTINE_ARG_LIST_NON_FIXED ROUTINE_ARG_LIST_0, \
  const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
  static ecma_completion_value_t c_function_name (ROUTINE_ARG_LIST_ ## args_number);
#include BUILTIN_INC_HEADER_NAME
#undef ROUTINE_ARG_LIST_NON_FIXED
#undef ROUTINE_ARG_LIST_3
#undef ROUTINE_ARG_LIST_2
#undef ROUTINE_ARG_LIST_1
#undef ROUTINE_ARG_LIST_0
#undef ROUTINE_ARG

#define ECMA_BUILTIN_PROPERTY_NAMES \
  PASTE (PASTE (ecma_builtin_property_names, _), BUILTIN_UNDERSCORED_ID)

static lit_magic_string_id_t ECMA_BUILTIN_PROPERTY_NAMES[] =
{
#define SIMPLE_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) name,
#define NUMBER_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) name,
#define STRING_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) name,
#define CP_UNIMPLEMENTED_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) name,
#define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) name,
#define ROUTINE(name, c_function_name, args_number, length_prop_value) name,
#include BUILTIN_INC_HEADER_NAME
};

/**
 * Sort builtin's property names array
 */
void
SORT_PROPERTY_NAMES_ROUTINE_NAME (BUILTIN_UNDERSCORED_ID) (void)
{
  bool swapped;

  do
  {
    swapped = false;

    for (ecma_length_t i = 1;
         i < (sizeof (ECMA_BUILTIN_PROPERTY_NAMES) / sizeof (ECMA_BUILTIN_PROPERTY_NAMES[0]));
         i++)
    {
      if (ECMA_BUILTIN_PROPERTY_NAMES[i] < ECMA_BUILTIN_PROPERTY_NAMES[i - 1])
      {
        lit_magic_string_id_t id_temp = ECMA_BUILTIN_PROPERTY_NAMES[i - 1];
        ECMA_BUILTIN_PROPERTY_NAMES[i - 1] = ECMA_BUILTIN_PROPERTY_NAMES[i];
        ECMA_BUILTIN_PROPERTY_NAMES[i] = id_temp;

        swapped = true;
      }
    }
  }
  while (swapped);
} /* SORT_PROPERTY_NAMES_ROUTINE_NAME */

/**
 * If the property's name is one of built-in properties of the built-in object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t *
TRY_TO_INSTANTIATE_PROPERTY_ROUTINE_NAME (BUILTIN_UNDERSCORED_ID) (ecma_object_t *obj_p, /**< object */
                                                                   ecma_string_t *prop_name_p) /**< property's name */
{
#define OBJECT_ID(builtin_id) const ecma_builtin_id_t builtin_object_id = builtin_id;
#include BUILTIN_INC_HEADER_NAME

  JERRY_ASSERT (ecma_builtin_is (obj_p, builtin_object_id));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  lit_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  const ecma_length_t property_numbers = (ecma_length_t) (sizeof (ECMA_BUILTIN_PROPERTY_NAMES) /
                                                          sizeof (ECMA_BUILTIN_PROPERTY_NAMES[0]));
  int32_t index;
  index = ecma_builtin_bin_search_for_magic_string_id_in_array (ECMA_BUILTIN_PROPERTY_NAMES,
                                                                property_numbers,
                                                                id);

  if (index == -1)
  {
    return NULL;
  }

  JERRY_ASSERT (index >= 0 && (uint32_t) index < sizeof (uint64_t) * JERRY_BITSINBYTE);

  uint32_t bit;
  ecma_internal_property_id_t mask_prop_id;

  if (index >= 32)
  {
    mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63;
    bit = (uint32_t) 1u << (index - 32);
  }
  else
  {
    mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31;
    bit = (uint32_t) 1u << index;
  }

  ecma_property_t *mask_prop_p = ecma_find_internal_property (obj_p, mask_prop_id);
  if (mask_prop_p == NULL)
  {
    mask_prop_p = ecma_create_internal_property (obj_p, mask_prop_id);
    mask_prop_p->u.internal_property.value = 0;
  }

  uint32_t bit_mask = mask_prop_p->u.internal_property.value;

  if (bit_mask & bit)
  {
    return NULL;
  }

  bit_mask |= bit;

  mask_prop_p->u.internal_property.value = bit_mask;

  ecma_value_t value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_property_writable_value_t writable;
  ecma_property_enumerable_value_t enumerable;
  ecma_property_configurable_value_t configurable;

  switch (id)
  {
    JERRY_ASSERT ((uint16_t) id == id);

#define ROUTINE(name, c_function_name, args_number, length_prop_value) case name: \
    { \
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (builtin_object_id, \
                                                                                 id, \
                                                                                 length_prop_value); \
      \
      writable = ECMA_PROPERTY_WRITABLE; \
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE; \
      configurable = ECMA_PROPERTY_CONFIGURABLE; \
      \
      value = ecma_make_object_value (func_obj_p); \
      \
      break; \
    }
#define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) case name: \
    { \
      value = ecma_make_object_value (obj_getter); \
      writable = prop_writable; \
      enumerable = prop_enumerable; \
      configurable = prop_configurable; \
      break; \
    }
#define SIMPLE_VALUE(name, simple_value, prop_writable, prop_enumerable, prop_configurable) case name: \
    { \
      value = ecma_make_simple_value (simple_value); \
      \
      writable = prop_writable; \
      enumerable = prop_enumerable; \
      configurable = prop_configurable; \
      \
      break; \
   }
#define NUMBER_VALUE(name, number_value, prop_writable, prop_enumerable, prop_configurable) case name: \
    { \
      ecma_number_t *num_p = ecma_alloc_number (); \
      *num_p = number_value; \
      \
      value = ecma_make_number_value (num_p); \
      \
      writable = prop_writable; \
      enumerable = prop_enumerable; \
      configurable = prop_configurable; \
      \
      break; \
    }
#define STRING_VALUE(name, magic_string_id, prop_writable, prop_enumerable, prop_configurable) case name: \
    { \
      ecma_string_t *magic_string_p = ecma_get_magic_string (magic_string_id); \
      \
      value = ecma_make_string_value (magic_string_p); \
      \
      writable = prop_writable; \
      enumerable = prop_enumerable; \
      configurable = prop_configurable; \
      \
      break; \
    }
#ifdef CONFIG_ECMA_COMPACT_PROFILE
#define CP_UNIMPLEMENTED_VALUE(name, value, prop_writable, prop_enumerable, prop_configurable) case name: \
    { \
      /* The object throws CompactProfileError upon invocation */ \
      ecma_object_t *get_set_p = ecma_builtin_get (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR); \
      ecma_property_t *compact_profile_thrower_property_p = ecma_create_named_accessor_property (obj_p, \
                                                                                                 prop_name_p, \
                                                                                                 get_set_p, \
                                                                                                 get_set_p, \
                                                                                                 true, \
                                                                                                 false); \
      ecma_deref_object (get_set_p); \
      \
      return compact_profile_thrower_property_p; \
    }
#else /* CONFIG_ECMA_COMPACT_PROFILE */
#define CP_UNIMPLEMENTED_VALUE(name, value, prop_writable, prop_enumerable, prop_configurable) case name: \
      { \
        JERRY_UNIMPLEMENTED ("The built-in is not implemented."); \
      }
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
#include BUILTIN_INC_HEADER_NAME

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_property_t *prop_p = ecma_create_named_data_property (obj_p,
                                                             prop_name_p,
                                                             writable,
                                                             enumerable,
                                                             configurable);

  ecma_named_data_property_assign_value (obj_p, prop_p, value);

  ecma_free_value (value, true);

  return prop_p;
} /* TRY_TO_INSTANTIATE_PROPERTY_ROUTINE_NAME */

/**
 * List names of the built-in object's lazy instantiated properties
 *
 * See also:
 *          TRY_TO_INSTANTIATE_PROPERTY_ROUTINE_NAME
 *
 * @return string values collection
 */
void
LIST_LAZY_PROPERTY_NAMES_ROUTINE_NAME (BUILTIN_UNDERSCORED_ID) (ecma_object_t *object_p, /**< a built-in object */
                                                                /** true -  list enumerable properties
                                                                 *           into main collection,
                                                                 *           and non-enumerable to
                                                                 *           collection of 'skipped
                                                                 *           non-enumerable'
                                                                 *           properties,
                                                                 *   false - list all properties into
                                                                 *           main collection.
                                                                 */
                                                                bool separate_enumerable,
                                                                /** 'main' collection */
                                                                ecma_collection_header_t *main_collection_p,
                                                                /** skipped 'non-enumerable' collection */
                                                                ecma_collection_header_t *non_enum_collection_p)
{
  ecma_collection_header_t *for_enumerable_p = main_collection_p;
  (void) for_enumerable_p;

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

#define OBJECT_ID(builtin_id) const ecma_builtin_id_t builtin_object_id = builtin_id;
#include BUILTIN_INC_HEADER_NAME

  JERRY_ASSERT (ecma_builtin_is (object_p, builtin_object_id));

  const ecma_length_t properties_number = (ecma_length_t) (sizeof (ECMA_BUILTIN_PROPERTY_NAMES) /
                                                           sizeof (ECMA_BUILTIN_PROPERTY_NAMES[0]));

  for (ecma_length_t i = 0;
       i < properties_number;
       i++)
  {
    lit_magic_string_id_t name = ECMA_BUILTIN_PROPERTY_NAMES[i];

    int32_t index;
    index = ecma_builtin_bin_search_for_magic_string_id_in_array (ECMA_BUILTIN_PROPERTY_NAMES,
                                                                  properties_number,
                                                                  name);

    uint32_t bit;
    ecma_internal_property_id_t mask_prop_id;

    if (index >= 32)
    {
      mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63;
      bit = (uint32_t) 1u << (index - 32);
    }
    else
    {
      mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31;
      bit = (uint32_t) 1u << index;
    }

    ecma_property_t *mask_prop_p = ecma_find_internal_property (object_p, mask_prop_id);
    bool is_instantiated = false;
    if (mask_prop_p == NULL)
    {
      is_instantiated = true;
    }
    else
    {
      uint32_t bit_mask = mask_prop_p->u.internal_property.value;

      if (bit_mask & bit)
      {
        is_instantiated = true;
      }
      else
      {
        is_instantiated = false;
      }
    }

    bool is_existing;

    ecma_string_t *name_p = ecma_get_magic_string (name);

    if (!is_instantiated)
    {
      /* will be instantiated upon first request */
      is_existing = true;
    }
    else
    {
      if (ecma_op_object_get_own_property (object_p, name_p) == NULL)
      {
        is_existing = false;
      }
      else
      {
        is_existing = true;
      }
    }

    if (is_existing)
    {
      ecma_append_to_values_collection (for_non_enumerable_p,
                                        ecma_make_string_value (name_p),
                                        true);
    }

    ecma_deref_ecma_string (name_p);
  }
} /* LIST_LAZY_PROPERTY_NAMES_ROUTINE_NAME */


/**
 * Dispatcher of the built-in's routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
DISPATCH_ROUTINE_ROUTINE_NAME (BUILTIN_UNDERSCORED_ID) (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                                          identifier */
                                                        ecma_value_t this_arg_value, /**< 'this' argument
                                                                                          value */
                                                        const ecma_value_t arguments_list[], /**< list of arguments
                                                                                                   passed to routine */
                                                        ecma_length_t arguments_number) /**< length of
                                                                                             arguments' list */
{
  /* the arguments may be unused for some built-ins */
  (void) this_arg_value;
  (void) arguments_list;
  (void) arguments_number;

  switch (builtin_routine_id)
  {
#define ROUTINE_ARG(n) (arguments_number >= n ? arguments_list[n - 1] \
                                              : ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED))
#define ROUTINE_ARG_LIST_0
#define ROUTINE_ARG_LIST_1 , ROUTINE_ARG(1)
#define ROUTINE_ARG_LIST_2 ROUTINE_ARG_LIST_1, ROUTINE_ARG(2)
#define ROUTINE_ARG_LIST_3 ROUTINE_ARG_LIST_2, ROUTINE_ARG(3)
#define ROUTINE_ARG_LIST_NON_FIXED , arguments_list, arguments_number
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
       case name: \
       { \
         return c_function_name (this_arg_value ROUTINE_ARG_LIST_ ## args_number); \
       }
#include BUILTIN_INC_HEADER_NAME
#undef ROUTINE_ARG
#undef ROUTINE_ARG_LIST_0
#undef ROUTINE_ARG_LIST_1
#undef ROUTINE_ARG_LIST_2
#undef ROUTINE_ARG_LIST_3
#undef ROUTINE_ARG_LIST_NON_FIXED

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* DISPATCH_ROUTINE_ROUTINE_NAME */

#undef PASTE__
#undef PASTE_
#undef PASTE
#undef SORT_PROPERTY_NAMES_ROUTINE_NAME
#undef DISPATCH_ROUTINE_ROUTINE_NAME
#undef TRY_TO_INSTANTIATE_PROPERTY_ROUTINE_NAME
#undef BUILTIN_UNDERSCORED_ID
#undef BUILTIN_INC_HEADER_NAME
#undef ECMA_BUILTIN_PROPERTY_NAMES

