/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged
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

#define PROPERTY_DESCRIPTOR_LIST_NAME \
  PASTE (PASTE (ecma_builtin_, BUILTIN_UNDERSCORED_ID), _property_descriptor_list)
#define LIST_LAZY_PROPERTY_NAMES_ROUTINE_NAME \
  PASTE (PASTE (ecma_builtin_, BUILTIN_UNDERSCORED_ID), _list_lazy_property_names)
#define DISPATCH_ROUTINE_ROUTINE_NAME \
  PASTE (PASTE (ecma_builtin_, BUILTIN_UNDERSCORED_ID), _dispatch_routine)

#define ROUTINE_ARG(n) , ecma_value_t arg ## n
#define ROUTINE_ARG_LIST_0 ecma_value_t this_arg
#define ROUTINE_ARG_LIST_1 ROUTINE_ARG_LIST_0 ROUTINE_ARG(1)
#define ROUTINE_ARG_LIST_2 ROUTINE_ARG_LIST_1 ROUTINE_ARG(2)
#define ROUTINE_ARG_LIST_3 ROUTINE_ARG_LIST_2 ROUTINE_ARG(3)
#define ROUTINE_ARG_LIST_NON_FIXED ROUTINE_ARG_LIST_0, \
  const ecma_value_t *arguments_list_p, ecma_length_t arguments_list_len
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
  static ecma_value_t c_function_name (ROUTINE_ARG_LIST_ ## args_number);
#include BUILTIN_INC_HEADER_NAME
#undef ROUTINE_ARG_LIST_NON_FIXED
#undef ROUTINE_ARG_LIST_3
#undef ROUTINE_ARG_LIST_2
#undef ROUTINE_ARG_LIST_1
#undef ROUTINE_ARG_LIST_0
#undef ROUTINE_ARG

#define ECMA_BUILTIN_PROPERTY_NAMES \
  PASTE (PASTE (ecma_builtin_property_names, _), BUILTIN_UNDERSCORED_ID)

static const lit_magic_string_id_t ECMA_BUILTIN_PROPERTY_NAMES[] =
{
#define SIMPLE_VALUE(name, simple_value, prop_attributes) name,
#define NUMBER_VALUE(name, number_value, prop_attributes) name,
#define STRING_VALUE(name, magic_string_id, prop_attributes) name,
#define OBJECT_VALUE(name, obj_builtin_id, prop_attributes) name,
#define ROUTINE(name, c_function_name, args_number, length_prop_value) name,
#include BUILTIN_INC_HEADER_NAME
};

#define ECMA_BUILTIN_PROPERTY_NAME_INDEX(name) \
  PASTE (PASTE (PASTE (PASTE (ecma_builtin_property_names, _), BUILTIN_UNDERSCORED_ID), _), name)

enum
{
#define SIMPLE_VALUE(name, simple_value, prop_attributes) \
  ECMA_BUILTIN_PROPERTY_NAME_INDEX(name),
#define NUMBER_VALUE(name, number_value, prop_attributes) \
  ECMA_BUILTIN_PROPERTY_NAME_INDEX(name),
#define STRING_VALUE(name, magic_string_id, prop_attributes) \
  ECMA_BUILTIN_PROPERTY_NAME_INDEX(name),
#define OBJECT_VALUE(name, obj_builtin_id, prop_attributes) \
  ECMA_BUILTIN_PROPERTY_NAME_INDEX(name),
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
  ECMA_BUILTIN_PROPERTY_NAME_INDEX(name),
#include BUILTIN_INC_HEADER_NAME
};

/**
 * Built-in property list of the built-in object.
 */
const ecma_builtin_property_descriptor_t PROPERTY_DESCRIPTOR_LIST_NAME[] =
{
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
  { \
    name, \
    ECMA_BUILTIN_PROPERTY_ROUTINE, \
    ECMA_PROPERTY_CONFIGURABLE_WRITABLE, \
    length_prop_value \
  },
#define OBJECT_VALUE(name, obj_builtin_id, prop_attributes) \
  { \
    name, \
    ECMA_BUILTIN_PROPERTY_OBJECT, \
    prop_attributes, \
    obj_builtin_id \
  },
#define SIMPLE_VALUE(name, simple_value, prop_attributes) \
  { \
    name, \
    ECMA_BUILTIN_PROPERTY_SIMPLE, \
    prop_attributes, \
    simple_value \
  },
#define NUMBER_VALUE(name, number_value, prop_attributes) \
  { \
    name, \
    ECMA_BUILTIN_PROPERTY_NUMBER, \
    prop_attributes, \
    number_value \
  },
#define STRING_VALUE(name, magic_string_id, prop_attributes) \
  { \
    name, \
    ECMA_BUILTIN_PROPERTY_STRING, \
    prop_attributes, \
    magic_string_id \
  },
#include BUILTIN_INC_HEADER_NAME
  {
    LIT_MAGIC_STRING__COUNT,
    ECMA_BUILTIN_PROPERTY_END,
    0,
    0
  }
};

/**
 * List names of the built-in object's lazy instantiated properties
 *
 * See also:
 *          TRY_TO_INSTANTIATE_PROPERTY_ROUTINE_NAME
 *
 * @return string values collection
 */
void
LIST_LAZY_PROPERTY_NAMES_ROUTINE_NAME (ecma_object_t *object_p, /**< a built-in object */
                                       bool separate_enumerable, /**< true -  list enumerable properties
                                                                              into main collection,
                                                                              and non-enumerable to
                                                                              collection of 'skipped
                                                                              non-enumerable'
                                                                              properties,
                                                                      false - list all properties into
                                                                              main collection. */
                                       ecma_collection_header_t *main_collection_p, /**< 'main' collection */
                                       ecma_collection_header_t *non_enum_collection_p) /**< skipped 'non-enumerable'
                                                                                             collection */
{
  ecma_collection_header_t *for_enumerable_p = main_collection_p;
  (void) for_enumerable_p;

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

#define OBJECT_ID(builtin_id) const ecma_builtin_id_t builtin_object_id = builtin_id;
#include BUILTIN_INC_HEADER_NAME

  JERRY_ASSERT (ecma_builtin_is (object_p, builtin_object_id));

  const ecma_length_t properties_number = (ecma_length_t) (sizeof (ECMA_BUILTIN_PROPERTY_NAMES) /
                                                           sizeof (ECMA_BUILTIN_PROPERTY_NAMES[0]));

  for (ecma_length_t index = 0;
       index < properties_number;
       index++)
  {
    lit_magic_string_id_t name = ECMA_BUILTIN_PROPERTY_NAMES[index];

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
      uint32_t bit_mask = ecma_get_internal_property_value (mask_prop_p);

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
DISPATCH_ROUTINE_ROUTINE_NAME (uint16_t builtin_routine_id, /**< built-in wide routine
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
#undef PROPERTY_DESCRIPTOR_LIST_NAME
#undef LIST_LAZY_PROPERTY_NAMES_ROUTINE_NAME
#undef DISPATCH_ROUTINE_ROUTINE_NAME
#undef BUILTIN_UNDERSCORED_ID
#undef BUILTIN_INC_HEADER_NAME
#undef ECMA_BUILTIN_PROPERTY_NAMES
#undef ECMA_BUILTIN_PROPERTY_NAME_INDEX
