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
#if ENABLED (JERRY_ES2015_BUILTIN_SYMBOL)
  ecma_free_string_list (JERRY_CONTEXT (symbol_list_first_p));
#endif /* ENABLED (JERRY_ES2015_BUILTIN_SYMBOL) */
  ecma_free_string_list (JERRY_CONTEXT (string_list_first_p));
  ecma_free_string_list (JERRY_CONTEXT (number_list_first_p));
} /* ecma_finalize_lit_storage */

/**
 * Find or create a literal string.
 *
 * @return ecma_string_t compressed pointer
 */
ecma_value_t
ecma_find_or_create_literal_string (const lit_utf8_byte_t *chars_p, /**< string to be searched */
                                    lit_utf8_size_t size) /**< size of the string */
{
  ecma_string_t *string_p = ecma_new_ecma_string_from_utf8 (chars_p, size);

  if (ECMA_IS_DIRECT_STRING (string_p))
  {
    return ecma_make_string_value (string_p);
  }

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
          return ecma_make_string_value (value_p);
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
    return ecma_make_string_value (string_p);
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

  return ecma_make_string_value (string_p);
} /* ecma_find_or_create_literal_string */

/**
 * Find or create a literal number.
 *
 * @return ecma_string_t compressed pointer
 */
ecma_value_t
ecma_find_or_create_literal_number (ecma_number_t number_arg) /**< number to be searched */
{
  ecma_value_t num = ecma_make_number_value (number_arg);

  if (ecma_is_value_integer_number (num))
  {
    return num;
  }

  JERRY_ASSERT (ecma_is_value_float_number (num));

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
        JERRY_ASSERT (ecma_is_value_float_number (value_p->u.lit_number));

        if (ecma_get_float_from_value (value_p->u.lit_number) == number_arg)
        {
          ecma_free_value (num);
          return value_p->u.lit_number;
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
    return num;
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

  return num;
} /* ecma_find_or_create_literal_number */

/**
 * Log2 of snapshot literal alignment.
 */
#define JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG 1

/**
 * Snapshot literal alignment.
 */
#define JERRY_SNAPSHOT_LITERAL_ALIGNMENT (1u << JERRY_SNAPSHOT_LITERAL_ALIGNMENT_LOG)

/**
 * Literal offset shift.
 */
#define JERRY_SNAPSHOT_LITERAL_SHIFT (ECMA_VALUE_SHIFT + 1)

/**
 * Literal value is number.
 */
#define JERRY_SNAPSHOT_LITERAL_IS_NUMBER (1u << ECMA_VALUE_SHIFT)

#if ENABLED (JERRY_SNAPSHOT_SAVE)

/**
 * Append the value at the end of the appropriate list if it is not present there.
 */
void ecma_save_literals_append_value (ecma_value_t value, /**< value to be appended */
                                      ecma_collection_header_t *lit_pool_p) /**< list of known values */
{
  /* Unlike direct numbers, direct strings are converted to character literals. */
  if (!ecma_is_value_string (value) && !ecma_is_value_float_number (value))
  {
    return;
  }

  ecma_value_t *iterator_p = ecma_collection_iterator_init (lit_pool_p);

  while (iterator_p != NULL)
  {
    /* Strings / numbers are direct strings or stored in the literal storage.
     * Therefore direct comparison is enough to find the same strings / numbers. */
    if (*iterator_p == value)
    {
      return;
    }

    iterator_p = ecma_collection_iterator_next (iterator_p);
  }

  ecma_append_to_values_collection (lit_pool_p, value, ECMA_COLLECTION_NO_COPY);
} /* ecma_save_literals_append_value */

/**
 * Add names from a byte-code data to a list.
 */
void
ecma_save_literals_add_compiled_code (const ecma_compiled_code_t *compiled_code_p, /**< byte-code data */
                                      ecma_collection_header_t *lit_pool_p) /**< list of known values */
{
  ecma_value_t *literal_p;
  uint32_t argument_end = 0;
  uint32_t register_end;
  uint32_t const_literal_end;
  uint32_t literal_end;

  JERRY_ASSERT (compiled_code_p->status_flags & CBC_CODE_FLAGS_FUNCTION);

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) compiled_code_p;
    uint8_t *byte_p = (uint8_t *) compiled_code_p;

    literal_p = (ecma_value_t *) (byte_p + sizeof (cbc_uint16_arguments_t));
    register_end = args_p->register_end;
    const_literal_end = args_p->const_literal_end - register_end;
    literal_end = args_p->literal_end - register_end;

    if (CBC_NON_STRICT_ARGUMENTS_NEEDED (compiled_code_p))
    {
      argument_end = args_p->argument_end;
    }
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) compiled_code_p;
    uint8_t *byte_p = (uint8_t *) compiled_code_p;

    literal_p = (ecma_value_t *) (byte_p + sizeof (cbc_uint8_arguments_t));
    register_end = args_p->register_end;
    const_literal_end = args_p->const_literal_end - register_end;
    literal_end = args_p->literal_end - register_end;

    if (CBC_NON_STRICT_ARGUMENTS_NEEDED (compiled_code_p))
    {
      argument_end = args_p->argument_end;
    }
  }

  for (uint32_t i = 0; i < argument_end; i++)
  {
    ecma_save_literals_append_value (literal_p[i], lit_pool_p);
  }

  for (uint32_t i = 0; i < const_literal_end; i++)
  {
    ecma_save_literals_append_value (literal_p[i], lit_pool_p);
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    ecma_compiled_code_t *bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t,
                                                                        literal_p[i]);

    if ((bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION)
        && bytecode_p != compiled_code_p)
    {
      ecma_save_literals_add_compiled_code (bytecode_p, lit_pool_p);
    }
  }

  if (argument_end != 0)
  {
    uint8_t *byte_p = (uint8_t *) compiled_code_p;
    byte_p += ((size_t) compiled_code_p->size) << JMEM_ALIGNMENT_LOG;
    literal_p = ((ecma_value_t *) byte_p) - argument_end;

    for (uint32_t i = 0; i < argument_end; i++)
    {
      ecma_save_literals_append_value (literal_p[i], lit_pool_p);
    }
  }
} /* ecma_save_literals_add_compiled_code */

/**
 * Save literals to specified snapshot buffer.
 *
 * Note:
 *      Frees 'lit_pool_p' regardless of success.
 *
 * @return true - if save was performed successfully (i.e. buffer size is sufficient),
 *         false - otherwise
 */
bool
ecma_save_literals_for_snapshot (ecma_collection_header_t *lit_pool_p, /**< list of known values */
                                 uint32_t *buffer_p, /**< [out] output snapshot buffer */
                                 size_t buffer_size, /**< size of the buffer */
                                 size_t *in_out_buffer_offset_p, /**< [in,out] write position in the buffer */
                                 lit_mem_to_snapshot_id_map_entry_t **out_map_p, /**< [out] map from literal identifiers
                                                                                  *   to the literal offsets
                                                                                  *   in snapshot */
                                 uint32_t *out_map_len_p) /**< [out] number of literals */
{
  if (lit_pool_p->item_count == 0)
  {
    *out_map_p = NULL;
    *out_map_len_p = 0;
  }

  uint32_t lit_table_size = 0;
  size_t max_lit_table_size = buffer_size - *in_out_buffer_offset_p;

  if (max_lit_table_size > (UINT32_MAX >> JERRY_SNAPSHOT_LITERAL_SHIFT))
  {
    max_lit_table_size = (UINT32_MAX >> JERRY_SNAPSHOT_LITERAL_SHIFT);
  }

  ecma_value_t *iterator_p = ecma_collection_iterator_init (lit_pool_p);

  /* Compute the size of the literal pool. */
  while (iterator_p != NULL)
  {
    if (ecma_is_value_float_number (*iterator_p))
    {
      lit_table_size += (uint32_t) sizeof (ecma_number_t);
    }
    else
    {
      ecma_string_t *string_p = ecma_get_string_from_value (*iterator_p);

      lit_table_size += (uint32_t) JERRY_ALIGNUP (sizeof (uint16_t) + ecma_string_get_size (string_p),
                                                  JERRY_SNAPSHOT_LITERAL_ALIGNMENT);
    }

    /* Check whether enough space is available and the maximum size is not reached. */
    if (lit_table_size > max_lit_table_size)
    {
      ecma_free_values_collection (lit_pool_p, ECMA_COLLECTION_NO_COPY);
      return false;
    }

    iterator_p = ecma_collection_iterator_next (iterator_p);
  }

  lit_mem_to_snapshot_id_map_entry_t *map_p;
  ecma_length_t total_count = lit_pool_p->item_count;

  map_p = jmem_heap_alloc_block (total_count * sizeof (lit_mem_to_snapshot_id_map_entry_t));

  /* Set return values (no error is possible from here). */
  JERRY_ASSERT ((*in_out_buffer_offset_p % sizeof (uint32_t)) == 0);

  uint8_t *destination_p = (uint8_t *) (buffer_p + (*in_out_buffer_offset_p / sizeof (uint32_t)));
  uint32_t literal_offset = 0;

  *in_out_buffer_offset_p += lit_table_size;
  *out_map_p = map_p;
  *out_map_len_p = total_count;

  iterator_p = ecma_collection_iterator_init (lit_pool_p);

  /* Generate literal pool data. */
  while (iterator_p != NULL)
  {
    map_p->literal_id = *iterator_p;
    map_p->literal_offset = (literal_offset << JERRY_SNAPSHOT_LITERAL_SHIFT) | ECMA_TYPE_SNAPSHOT_OFFSET;

    ecma_length_t length;

    if (ecma_is_value_float_number (*iterator_p))
    {
      map_p->literal_offset |= JERRY_SNAPSHOT_LITERAL_IS_NUMBER;

      ecma_number_t num = ecma_get_float_from_value (*iterator_p);
      memcpy (destination_p, &num, sizeof (ecma_number_t));

      length = JERRY_ALIGNUP (sizeof (ecma_number_t), JERRY_SNAPSHOT_LITERAL_ALIGNMENT);
    }
    else
    {
      ecma_string_t *string_p = ecma_get_string_from_value (*iterator_p);
      length = ecma_string_get_size (string_p);

      *(uint16_t *) destination_p = (uint16_t) length;

      ecma_string_to_utf8_bytes (string_p, destination_p + sizeof (uint16_t), length);

      length = JERRY_ALIGNUP (sizeof (uint16_t) + length, JERRY_SNAPSHOT_LITERAL_ALIGNMENT);
    }

    JERRY_ASSERT ((length % sizeof (uint16_t)) == 0);
    destination_p += length;
    literal_offset += length;

    iterator_p = ecma_collection_iterator_next (iterator_p);
    map_p++;
  }

  ecma_free_values_collection (lit_pool_p, ECMA_COLLECTION_NO_COPY);
  return true;
} /* ecma_save_literals_for_snapshot */

#endif /* ENABLED (JERRY_SNAPSHOT_SAVE) */

#if ENABLED (JERRY_SNAPSHOT_EXEC) || ENABLED (JERRY_SNAPSHOT_SAVE)

/**
 * Get the compressed pointer of a given literal.
 *
 * @return literal compressed pointer
 */
ecma_value_t
ecma_snapshot_get_literal (const uint8_t *literal_base_p, /**< literal start */
                           ecma_value_t literal_value) /**< string / number offset */
{
  JERRY_ASSERT ((literal_value & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET);

  const uint8_t *literal_p = literal_base_p + (literal_value >> JERRY_SNAPSHOT_LITERAL_SHIFT);

  if (literal_value & JERRY_SNAPSHOT_LITERAL_IS_NUMBER)
  {
    ecma_number_t num;
    memcpy (&num, literal_p, sizeof (ecma_number_t));
    return ecma_find_or_create_literal_number (num);
  }

  uint16_t length = *(const uint16_t *) literal_p;

  return ecma_find_or_create_literal_string (literal_p + sizeof (uint16_t), length);
} /* ecma_snapshot_get_literal */

#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) || ENABLED (JERRY_SNAPSHOT_SAVE) */

/**
 * @}
 * @}
 */
