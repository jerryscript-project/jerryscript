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

#include "ecma-builtin-helpers.h"
#include "ecma-globals.h"
#include "ecma-try-catch-macro.h"

/**
 * Function used to reconstruct the ordered binary tree.
 * Shifts 'index' down in the tree until it is in the correct position.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_helper_array_to_heap (ecma_value_t *array_p, /**< heap data array */
                                   uint32_t index, /**< current item index */
                                   uint32_t right, /**< right index is a maximum index */
                                   ecma_value_t compare_func, /**< compare function */
                                   const ecma_builtin_helper_sort_compare_fn_t sort_cb) /**< sorting cb */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* Left child of the current index. */
  uint32_t child = index * 2 + 1;
  ecma_value_t swap = array_p[index];
  bool should_break = false;

  while (child <= right && ecma_is_value_empty (ret_value) && !should_break)
  {
    if (child < right)
    {
      /* Compare the two child nodes. */
      ECMA_TRY_CATCH (child_compare_value, sort_cb (array_p[child], array_p[child + 1], compare_func),
                      ret_value);

      JERRY_ASSERT (ecma_is_value_number (child_compare_value));

      /* Use the child that is greater. */
      if (ecma_get_number_from_value (child_compare_value) < ECMA_NUMBER_ZERO)
      {
        child++;
      }

      ECMA_FINALIZE (child_compare_value);
    }

    if (ecma_is_value_empty (ret_value))
    {
      JERRY_ASSERT (child <= right);

      /* Compare current child node with the swap (tree top). */
      ECMA_TRY_CATCH (swap_compare_value, sort_cb (array_p[child], swap, compare_func), ret_value);
      JERRY_ASSERT (ecma_is_value_number (swap_compare_value));

      if (ecma_get_number_from_value (swap_compare_value) <= ECMA_NUMBER_ZERO)
      {
        /* Break from loop if current child is less than swap (tree top) */
        should_break = true;
      }
      else
      {
        /* We have to move 'swap' lower in the tree, so shift current child up in the hierarchy. */
        uint32_t parent = (child - 1) / 2;
        JERRY_ASSERT (parent <= right);
        array_p[parent] = array_p[child];

        /* Update child to be the left child of the current node. */
        child = child * 2 + 1;
      }

      ECMA_FINALIZE (swap_compare_value);
    }
  }

  /*
   * Loop ended, either current child does not exist, or is less than swap.
   * This means that 'swap' should be placed in the parent node.
   */
  uint32_t parent = (child - 1) / 2;
  JERRY_ASSERT (parent <= right);
  array_p[parent] = swap;

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ECMA_VALUE_UNDEFINED;
  }

  return ret_value;
} /* ecma_builtin_helper_array_to_heap */

/**
 * Heapsort function
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_helper_array_heap_sort_helper (ecma_value_t *array_p, /**< array to sort */
                                            uint32_t right, /**< right index */
                                            ecma_value_t compare_func, /**< compare function */
                                            const ecma_builtin_helper_sort_compare_fn_t sort_cb) /**< sorting cb */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* First, construct the ordered binary tree from the array. */
  for (uint32_t i = (right / 2) + 1; i > 0 && ecma_is_value_empty (ret_value); i--)
  {
    ECMA_TRY_CATCH (value,
                    ecma_builtin_helper_array_to_heap (array_p, i - 1, right, compare_func, sort_cb),
                    ret_value);
    ECMA_FINALIZE (value);
  }

  /* Sorting elements. */
  for (uint32_t i = right; i > 0 && ecma_is_value_empty (ret_value); i--)
  {
    /*
     * The top element will always contain the largest value.
     * Move top to the end, and remove it from the tree.
     */
    ecma_value_t swap = array_p[0];
    array_p[0] = array_p[i];
    array_p[i] = swap;

    /* Rebuild binary tree from the remaining elements. */
    ECMA_TRY_CATCH (value,
                    ecma_builtin_helper_array_to_heap (array_p, 0, i - 1, compare_func, sort_cb),
                    ret_value);
    ECMA_FINALIZE (value);
  }

  return ret_value;
} /* ecma_builtin_helper_array_heap_sort_helper */
