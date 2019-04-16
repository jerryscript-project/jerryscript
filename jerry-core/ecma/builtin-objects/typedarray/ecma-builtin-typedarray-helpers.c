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

#include "ecma-builtin-typedarray-helpers.h"

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-objects.h"
#include "ecma-typedarray-object.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * Returns true if the builtin is a TypedArray type.
 *
 * @return bool
 */
bool
ecma_typedarray_helper_is_typedarray (uint8_t builtin_id) /**< the builtin id of a type **/
{
  switch (builtin_id)
  {
    case ECMA_BUILTIN_ID_INT8ARRAY:
    case ECMA_BUILTIN_ID_UINT8ARRAY:
    case ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY:
    case ECMA_BUILTIN_ID_INT16ARRAY:
    case ECMA_BUILTIN_ID_UINT16ARRAY:
    case ECMA_BUILTIN_ID_INT32ARRAY:
    case ECMA_BUILTIN_ID_UINT32ARRAY:
    case ECMA_BUILTIN_ID_FLOAT32ARRAY:
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
    case ECMA_BUILTIN_ID_FLOAT64ARRAY:
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
    {
      return true;
    }
    default:
    {
      return false;
    }
  }
} /* ecma_typedarray_helper_is_typedarray */

/**
 * Get the element shift size of a TypedArray type.
 *
 * @return uint8_t
 */
uint8_t
ecma_typedarray_helper_get_shift_size (uint8_t builtin_id) /**< the builtin id of the typedarray **/
{
  switch (builtin_id)
  {
    case ECMA_BUILTIN_ID_INT8ARRAY:
    case ECMA_BUILTIN_ID_UINT8ARRAY:
    case ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY:
    {
      return 0;
    }
    case ECMA_BUILTIN_ID_INT16ARRAY:
    case ECMA_BUILTIN_ID_UINT16ARRAY:
    {
      return 1;
    }
    case ECMA_BUILTIN_ID_INT32ARRAY:
    case ECMA_BUILTIN_ID_UINT32ARRAY:
    case ECMA_BUILTIN_ID_FLOAT32ARRAY:
    {
      return 2;
    }
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
    case ECMA_BUILTIN_ID_FLOAT64ARRAY:
    {
      return 3;
    }
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_typedarray_helper_get_shift_size */

/**
 * Get the built-in TypedArray type from a magic string.
 *
 * @return uint8_t
 */
uint8_t
ecma_typedarray_helper_get_builtin_id (ecma_object_t *obj_p) /**< typedarray object **/
{
#define TYPEDARRAY_ID_CASE(magic_id, builtin_id) \
  case magic_id: \
  { \
    return builtin_id; \
  }

  switch (ecma_object_get_class_name (obj_p))
  {
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_INT8_ARRAY_UL, ECMA_BUILTIN_ID_INT8ARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_UINT8_ARRAY_UL, ECMA_BUILTIN_ID_UINT8ARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL, ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_INT16_ARRAY_UL, ECMA_BUILTIN_ID_INT16ARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_UINT16_ARRAY_UL, ECMA_BUILTIN_ID_UINT16ARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_INT32_ARRAY_UL, ECMA_BUILTIN_ID_INT32ARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_UINT32_ARRAY_UL, ECMA_BUILTIN_ID_UINT32ARRAY)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_FLOAT32_ARRAY_UL, ECMA_BUILTIN_ID_FLOAT32ARRAY)
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
    TYPEDARRAY_ID_CASE (LIT_MAGIC_STRING_FLOAT64_ARRAY_UL, ECMA_BUILTIN_ID_FLOAT64ARRAY)
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
#undef TYPEDARRAY_ID_CASE
} /* ecma_typedarray_helper_get_builtin_id */

/**
 * Get the magic string of a TypedArray type.
 *
 * @return lit_magic_string_id_t
 */
lit_magic_string_id_t
ecma_typedarray_helper_get_magic_string (uint8_t builtin_id) /**< the builtin id of the typedarray **/
{
#define TYPEDARRAY_ID_CASE(builtin_id, magic_id) \
  case builtin_id: \
  { \
    return (lit_magic_string_id_t) magic_id; \
  }

  switch (builtin_id)
  {
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_INT8ARRAY, LIT_MAGIC_STRING_INT8_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT8ARRAY, LIT_MAGIC_STRING_UINT8_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY, LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_INT16ARRAY, LIT_MAGIC_STRING_INT16_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT16ARRAY, LIT_MAGIC_STRING_UINT16_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_INT32ARRAY, LIT_MAGIC_STRING_INT32_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT32ARRAY, LIT_MAGIC_STRING_UINT32_ARRAY_UL)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_FLOAT32ARRAY, LIT_MAGIC_STRING_FLOAT32_ARRAY_UL)
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_FLOAT64ARRAY, LIT_MAGIC_STRING_FLOAT64_ARRAY_UL)
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
#undef TYPEDARRAY_ID_CASE
} /* ecma_typedarray_helper_get_magic_string */

/**
 * Get the prototype ID of a TypedArray type.
 *
 * @return uint8_t
 */
uint8_t
ecma_typedarray_helper_get_prototype_id (uint8_t builtin_id) /**< the builtin id of the typedarray **/
{
#define TYPEDARRAY_ID_CASE(builtin_id) \
  case builtin_id: \
  { \
    return builtin_id ## _PROTOTYPE; \
  }

  switch (builtin_id)
  {
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_INT8ARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT8ARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_INT16ARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT16ARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_INT32ARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_UINT32ARRAY)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_FLOAT32ARRAY)
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
    TYPEDARRAY_ID_CASE (ECMA_BUILTIN_ID_FLOAT64ARRAY)
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
#undef TYPEDARRAY_ID_CASE
} /* ecma_typedarray_helper_get_prototype_id */

/**
 * Common implementation of the [[Construct]] call of TypedArrays.
 *
 * @return ecma value of the new TypedArray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_typedarray_helper_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                           ecma_length_t arguments_list_len, /**< number of arguments */
                                           uint8_t builtin_id) /**< the builtin id of the typedarray */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);
  JERRY_ASSERT (ecma_typedarray_helper_is_typedarray (builtin_id));

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ecma_typedarray_helper_get_prototype_id (builtin_id));
  ecma_value_t val = ecma_op_create_typedarray (arguments_list_p,
                                                arguments_list_len,
                                                prototype_obj_p,
                                                ecma_typedarray_helper_get_shift_size (builtin_id),
                                                ecma_typedarray_helper_get_magic_string (builtin_id));

  return val;
} /* ecma_typedarray_helper_dispatch_construct */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
