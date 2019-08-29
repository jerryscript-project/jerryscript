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
#include "ecma-helpers.h"
#include "ecma-builtin-helpers.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_BUILTIN_JSON)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Check whether the object is pushed onto the occurence stack
 *
 * Used by:
 *         - ecma_builtin_json_object step 1
 *         - ecma_builtin_json_array step 1
 *
 * @return true - if the object is pushed onto the occurence stack
 *         false - otherwise
 */
bool
ecma_json_has_object_in_stack (ecma_json_occurence_stack_item_t *stack_p, /**< stack */
                               ecma_object_t *object_p) /**< object */
{
  while (stack_p != NULL)
  {
    if (stack_p->object_p == object_p)
    {
      return true;
    }

    stack_p = stack_p->next_p;
  }

  return false;
} /* ecma_json_has_object_in_stack */

/**
 * Check the string value existance in the collection.
 *
 * Used by:
 *         - ecma_builtin_json_stringify step 4.b.ii.5
 *
 * @return true, if the string is already in the collection.
 */
bool
ecma_has_string_value_in_collection (ecma_collection_t *collection_p, /**< collection */
                                     ecma_string_t *string_p) /**< string */
{
  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    ecma_string_t *current_p = ecma_get_string_from_value (buffer_p[i]);

    if (ecma_compare_ecma_strings (current_p, string_p))
    {
      return true;
    }
  }

  return false;
} /* ecma_has_string_value_in_collection */

#endif /* ENABLED (JERRY_BUILTIN_JSON) */

/**
 * @}
 * @}
 */
