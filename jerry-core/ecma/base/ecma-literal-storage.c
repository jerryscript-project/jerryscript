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
#include "ecma-literal-storage.h"
#include "ecma-helpers.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalitstorage Literal storage
 * @{
 */

/**
 * Free string list
 */
static void
ecma_free_string_list (ecma_lit_storage_item_t *string_list_p) /**< string list */
{
  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] != JMEM_CP_NULL)
      {
        ecma_string_t *string_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                string_list_p->values[i]);

        JERRY_ASSERT (ECMA_STRING_IS_REF_EQUALS_TO_ONE (string_p));
        ecma_deref_ecma_string (string_p);
      }
    }

    ecma_lit_storage_item_t *prev_item = string_list_p;
    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, string_list_p->next_cp);
    jmem_pools_free (prev_item, sizeof (ecma_lit_storage_item_t));
  }
} /* ecma_free_string_list */

/**
 * Finalize literal storage
 */
void
ecma_finalize_lit_storage (void)
{
  ecma_free_string_list (JERRY_CONTEXT (string_list_first_p));
  ecma_free_string_list (JERRY_CONTEXT (number_list_first_p));
} /* ecma_finalize_lit_storage */

/**
 * Find or create a literal string.
 *
 * @return ecma_string_t compressed pointer
 */
jmem_cpointer_t
ecma_find_or_create_literal_string (const lit_utf8_byte_t *chars_p, /**< string to be searched */
                                    lit_utf8_size_t size) /**< size of the string */
{
  ecma_string_t *string_p = ecma_new_ecma_string_from_utf8 (chars_p, size);

  ecma_lit_storage_item_t *string_list_p = JERRY_CONTEXT (string_list_first_p);
  jmem_cpointer_t *empty_cpointer_p = NULL;

  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] == JMEM_CP_NULL)
      {
        if (empty_cpointer_p == NULL)
        {
          empty_cpointer_p = string_list_p->values + i;
        }
      }
      else
      {
        ecma_string_t *value_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                               string_list_p->values[i]);

        if (ecma_compare_ecma_strings (string_p, value_p))
        {
          /* Return with string if found in the list. */
          ecma_deref_ecma_string (string_p);
          return string_list_p->values[i];
        }
      }
    }

    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, string_list_p->next_cp);
  }

  jmem_cpointer_t result;
  JMEM_CP_SET_NON_NULL_POINTER (result, string_p);

  if (empty_cpointer_p != NULL)
  {
    *empty_cpointer_p = result;
    return result;
  }

  ecma_lit_storage_item_t *new_item_p;
  new_item_p = (ecma_lit_storage_item_t *) jmem_pools_alloc (sizeof (ecma_lit_storage_item_t));

  new_item_p->values[0] = result;
  for (int i = 1; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
  {
    new_item_p->values[i] = JMEM_CP_NULL;
  }

  JMEM_CP_SET_POINTER (new_item_p->next_cp, JERRY_CONTEXT (string_list_first_p));
  JERRY_CONTEXT (string_list_first_p) = new_item_p;

  return result;
} /* ecma_find_or_create_literal_string */

/**
 * Find or create a literal number.
 *
 * @return ecma_string_t compressed pointer
 */
jmem_cpointer_t
ecma_find_or_create_literal_number (ecma_number_t number_arg) /**< number to be searched */
{
  ecma_value_t num = ecma_make_number_value (number_arg);

  ecma_lit_storage_item_t *number_list_p = JERRY_CONTEXT (number_list_first_p);
  jmem_cpointer_t *empty_cpointer_p = NULL;

  while (number_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (number_list_p->values[i] == JMEM_CP_NULL)
      {
        if (empty_cpointer_p == NULL)
        {
          empty_cpointer_p = number_list_p->values + i;
        }
      }
      else
      {
        ecma_string_t *value_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                               number_list_p->values[i]);

        JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (value_p) == ECMA_STRING_LITERAL_NUMBER);

        if (ecma_is_value_integer_number (num))
        {
          if (value_p->u.lit_number == num)
          {
            return number_list_p->values[i];
          }
        }
        else
        {
          if (ecma_is_value_float_number (value_p->u.lit_number)
              && ecma_get_float_from_value (value_p->u.lit_number) == ecma_get_float_from_value (num))
          {
            ecma_free_value (num);
            return number_list_p->values[i];
          }
        }
      }
    }

    number_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, number_list_p->next_cp);
  }

  ecma_string_t *string_p = ecma_alloc_string ();
  string_p->refs_and_container = ECMA_STRING_REF_ONE | ECMA_STRING_LITERAL_NUMBER;
  string_p->u.lit_number = num;

  jmem_cpointer_t result;
  JMEM_CP_SET_NON_NULL_POINTER (result, string_p);

  if (empty_cpointer_p != NULL)
  {
    *empty_cpointer_p = result;
    return result;
  }

  ecma_lit_storage_item_t *new_item_p;
  new_item_p = (ecma_lit_storage_item_t *) jmem_pools_alloc (sizeof (ecma_lit_storage_item_t));

  new_item_p->values[0] = result;
  for (int i = 1; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
  {
    new_item_p->values[i] = JMEM_CP_NULL;
  }

  JMEM_CP_SET_POINTER (new_item_p->next_cp, JERRY_CONTEXT (number_list_first_p));
  JERRY_CONTEXT (number_list_first_p) = new_item_p;

  return result;
} /* ecma_find_or_create_literal_number */

/**
 * Log2 of snapshot literal alignment.
 */
#define JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG 2

/**
 * Snapshot literal alignment.
 */
#define JERRY_SNAPSHOT_LITERAL_ALIGNMENT (1u << JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG)

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE

/**
 * Save literals to specified snapshot buffer.
 *
 * @return true - if save was performed successfully (i.e. buffer size is sufficient),
 *         false - otherwise
 */
bool
ecma_save_literals_for_snapshot (uint32_t *buffer_p, /**< [out] output snapshot buffer */
                                 size_t buffer_size, /**< size of the buffer */
                                 size_t *in_out_buffer_offset_p, /**< [in,out] write position in the buffer */
                                 lit_mem_to_snapshot_id_map_entry_t **out_map_p, /**< [out] map from literal identifiers
                                                                                  *   to the literal offsets
                                                                                  *   in snapshot */
                                 uint32_t *out_map_len_p) /**< [out] number of literals */
{
  /* Count literals and literal space. */
  uint32_t lit_table_size = sizeof (uint32_t);
  uint32_t total_count = 0;

  ecma_lit_storage_item_t *string_list_p = JERRY_CONTEXT (string_list_first_p);

  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] != JMEM_CP_NULL)
      {
        ecma_string_t *string_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                string_list_p->values[i]);

        lit_table_size += (uint32_t) JERRY_ALIGNUP (sizeof (uint16_t) + ecma_string_get_size (string_p),
                                                    JERRY_SNAPSHOT_LITERAL_ALIGNMENT);
        total_count++;
      }
    }

    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, string_list_p->next_cp);
  }

  uint32_t number_offset = lit_table_size;

  ecma_lit_storage_item_t *number_list_p = JERRY_CONTEXT (number_list_first_p);

  while (number_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (number_list_p->values[i] != JMEM_CP_NULL)
      {
        lit_table_size += (uint32_t) sizeof (ecma_number_t);
        total_count++;
      }
    }

    number_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, number_list_p->next_cp);
  }

  /* Check whether enough space is available. */
  if (*in_out_buffer_offset_p + lit_table_size > buffer_size)
  {
    return false;
  }

  /* Check whether the maximum literal table size is reached. */
  if ((lit_table_size >> JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG) > UINT16_MAX)
  {
    return false;
  }

  lit_mem_to_snapshot_id_map_entry_t *map_p;

  map_p = jmem_heap_alloc_block (total_count * sizeof (lit_mem_to_snapshot_id_map_entry_t));

  /* Set return values (no error is possible from here). */
  JERRY_ASSERT ((*in_out_buffer_offset_p % sizeof (uint32_t)) == 0);
  buffer_p += *in_out_buffer_offset_p / sizeof (uint32_t);
  *in_out_buffer_offset_p += lit_table_size;
  *out_map_p = map_p;
  *out_map_len_p = total_count;

  /* Write data into the buffer. */

  /* The zero value is reserved for NULL (no literal)
   * constant so the first literal must have offset one. */
  uint32_t literal_offset = JERRY_SNAPSHOT_LITERAL_ALIGNMENT;

  *buffer_p++ = number_offset;

  string_list_p = JERRY_CONTEXT (string_list_first_p);

  uint16_t *destination_p = (uint16_t *) buffer_p;

  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] != JMEM_CP_NULL)
      {
        map_p->literal_id = string_list_p->values[i];
        map_p->literal_offset = (jmem_cpointer_t) (literal_offset >> JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG);
        map_p++;

        ecma_string_t *string_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                string_list_p->values[i]);

        ecma_length_t length = ecma_string_get_size (string_p);

        *destination_p = (uint16_t) length;
        ecma_string_to_utf8_bytes (string_p, ((lit_utf8_byte_t *) destination_p) + sizeof (uint16_t), length);

        length = JERRY_ALIGNUP (sizeof (uint16_t) + length,
                                JERRY_SNAPSHOT_LITERAL_ALIGNMENT);

        JERRY_ASSERT ((length % sizeof (uint16_t)) == 0);
        destination_p += length / sizeof (uint16_t);
        literal_offset += length;
      }
    }

    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, string_list_p->next_cp);
  }

  number_list_p = JERRY_CONTEXT (number_list_first_p);

  while (number_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (number_list_p->values[i] != JMEM_CP_NULL)
      {
        map_p->literal_id = number_list_p->values[i];
        map_p->literal_offset = (jmem_cpointer_t) (literal_offset >> JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG);
        map_p++;

        ecma_string_t *value_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                               number_list_p->values[i]);

        JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (value_p) == ECMA_STRING_LITERAL_NUMBER);

        ecma_number_t num = ecma_get_number_from_value (value_p->u.lit_number);
        memcpy (destination_p, &num, sizeof (ecma_number_t));

        ecma_length_t length = JERRY_ALIGNUP (sizeof (ecma_number_t),
                                              JERRY_SNAPSHOT_LITERAL_ALIGNMENT);

        JERRY_ASSERT ((length % sizeof (uint16_t)) == 0);
        destination_p += length / sizeof (uint16_t);
        literal_offset += length;
      }
    }

    number_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, number_list_p->next_cp);
  }

  return true;
} /* ecma_save_literals_for_snapshot */

#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

#if defined JERRY_ENABLE_SNAPSHOT_EXEC || defined JERRY_ENABLE_SNAPSHOT_SAVE

/**
 * Computes the base pointer of the literals and starting offset of numbers.
 *
 * @return the base pointer of the literals
 */
const uint8_t *
ecma_snapshot_get_literals_base (uint32_t *buffer_p, /**< literal buffer start */
                                 const uint8_t **number_base_p) /**< [out] literal number start */
{
  *number_base_p = ((uint8_t *) buffer_p) + buffer_p[0];

  return ((uint8_t *) (buffer_p + 1)) - JERRY_SNAPSHOT_LITERAL_ALIGNMENT;
} /* ecma_snapshot_get_literals_base */

/**
 * Get the compressed pointer of a given literal.
 *
 * @return literal compressed pointer
 */
jmem_cpointer_t
ecma_snapshot_get_literal (const uint8_t *literal_base_p, /**< literal start */
                           const uint8_t *number_base_p, /**< literal number start */
                           jmem_cpointer_t offset)
{
  if (offset == 0)
  {
    return ECMA_NULL_POINTER;
  }

  const uint8_t *literal_p = literal_base_p + (((size_t) offset) << JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG);

  if (literal_p >= number_base_p)
  {
    ecma_number_t num;
    memcpy (&num, literal_p, sizeof (ecma_number_t));
    return ecma_find_or_create_literal_number (num);
  }

  uint16_t length = *(const uint16_t *) literal_p;

  return ecma_find_or_create_literal_string (literal_p + sizeof (uint16_t), length);
} /* ecma_snapshot_get_literal */

#endif /* JERRY_ENABLE_SNAPSHOT_EXEC || JERRY_ENABLE_SNAPSHOT_SAVE */

/**
 * @}
 * @}
 */
