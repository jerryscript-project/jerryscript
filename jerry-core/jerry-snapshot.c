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

#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-helpers.h"
#include "ecma-literal-storage.h"
#include "jcontext.h"
#include "jerryscript.h"
#include "jerry-snapshot.h"
#include "js-parser.h"
#include "lit-char-helpers.h"
#include "re-compiler.h"

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE

/**
 * Variables required to take a snapshot.
 */
typedef struct
{
  bool snapshot_error_occured;
  size_t snapshot_buffer_write_offset;
} snapshot_globals_t;

/** \addtogroup jerrysnapshot Jerry snapshot operations
 * @{
 */

/**
 * Write data into the specified buffer.
 *
 * Note:
 *      Offset is in-out and is incremented if the write operation completes successfully.
 *
 * @return true, if write was successful, i.e. offset + data_size doesn't exceed buffer size,
 *         false - otherwise.
 */
static inline bool __attr_always_inline___
snapshot_write_to_buffer_by_offset (uint8_t *buffer_p, /**< buffer */
                                    size_t buffer_size, /**< size of buffer */
                                    size_t *in_out_buffer_offset_p,  /**< [in,out] offset to write to
                                                                      * incremented with data_size */
                                    const void *data_p, /**< data */
                                    size_t data_size) /**< size of the writable data */
{
  if (*in_out_buffer_offset_p + data_size > buffer_size)
  {
    return false;
  }

  memcpy (buffer_p + *in_out_buffer_offset_p, data_p, data_size);
  *in_out_buffer_offset_p += data_size;

  return true;
} /* snapshot_write_to_buffer_by_offset */

/**
 * Snapshot callback for byte codes.
 *
 * @return start offset
 */
static uint16_t
snapshot_add_compiled_code (ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                            uint8_t *snapshot_buffer_p, /**< snapshot buffer */
                            size_t snapshot_buffer_size, /**< snapshot buffer size */
                            snapshot_globals_t *globals_p) /**< snapshot globals */
{
  if (globals_p->snapshot_error_occured)
  {
    return 0;
  }

  JERRY_ASSERT ((globals_p->snapshot_buffer_write_offset & (JMEM_ALIGNMENT - 1)) == 0);

  if ((globals_p->snapshot_buffer_write_offset >> JMEM_ALIGNMENT_LOG) > 0xffffu)
  {
    globals_p->snapshot_error_occured = true;
    return 0;
  }

  uint16_t start_offset = (uint16_t) (globals_p->snapshot_buffer_write_offset >> JMEM_ALIGNMENT_LOG);

  uint8_t *copied_code_start_p = snapshot_buffer_p + globals_p->snapshot_buffer_write_offset;
  ecma_compiled_code_t *copied_code_p = (ecma_compiled_code_t *) copied_code_start_p;

  if (!(compiled_code_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
    /* Regular expression. */
    if (globals_p->snapshot_buffer_write_offset + sizeof (ecma_compiled_code_t) > snapshot_buffer_size)
    {
      globals_p->snapshot_error_occured = true;
      return 0;
    }

    globals_p->snapshot_buffer_write_offset += sizeof (ecma_compiled_code_t);

    jmem_cpointer_t pattern_cp = ((re_compiled_code_t *) compiled_code_p)->pattern_cp;
    ecma_string_t *pattern_string_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                 pattern_cp);

    ecma_length_t pattern_size = 0;

    ECMA_STRING_TO_UTF8_STRING (pattern_string_p, buffer_p, buffer_size);

    pattern_size = buffer_size;

    if (!snapshot_write_to_buffer_by_offset (snapshot_buffer_p,
                                             snapshot_buffer_size,
                                             &globals_p->snapshot_buffer_write_offset,
                                             buffer_p,
                                             buffer_size))
    {
      globals_p->snapshot_error_occured = true;
    }

    ECMA_FINALIZE_UTF8_STRING (buffer_p, buffer_size);

    globals_p->snapshot_buffer_write_offset = JERRY_ALIGNUP (globals_p->snapshot_buffer_write_offset,
                                                             JMEM_ALIGNMENT);

    /* Regexp character size is stored in refs. */
    copied_code_p->refs = (uint16_t) pattern_size;

    pattern_size += (ecma_length_t) sizeof (ecma_compiled_code_t);
    copied_code_p->size = (uint16_t) ((pattern_size + JMEM_ALIGNMENT - 1) >> JMEM_ALIGNMENT_LOG);

    copied_code_p->status_flags = compiled_code_p->status_flags;

#else /* CONFIG_DISABLE_REGEXP_BUILTIN */
    JERRY_UNREACHABLE (); /* RegExp is not supported in the selected profile. */
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
    return start_offset;
  }

  if (!snapshot_write_to_buffer_by_offset (snapshot_buffer_p,
                                           snapshot_buffer_size,
                                           &globals_p->snapshot_buffer_write_offset,
                                           compiled_code_p,
                                           ((size_t) compiled_code_p->size) << JMEM_ALIGNMENT_LOG))
  {
    globals_p->snapshot_error_occured = true;
    return 0;
  }

  /* Sub-functions and regular expressions are stored recursively. */
  uint8_t *src_buffer_p = (uint8_t *) compiled_code_p;
  uint8_t *dst_buffer_p = (uint8_t *) copied_code_p;
  jmem_cpointer_t *src_literal_start_p;
  jmem_cpointer_t *dst_literal_start_p;
  uint32_t const_literal_end;
  uint32_t literal_end;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    src_literal_start_p = (jmem_cpointer_t *) (src_buffer_p + sizeof (cbc_uint16_arguments_t));
    dst_literal_start_p = (jmem_cpointer_t *) (dst_buffer_p + sizeof (cbc_uint16_arguments_t));

    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) src_buffer_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
  }
  else
  {
    src_literal_start_p = (jmem_cpointer_t *) (src_buffer_p + sizeof (cbc_uint8_arguments_t));
    dst_literal_start_p = (jmem_cpointer_t *) (dst_buffer_p + sizeof (cbc_uint8_arguments_t));

    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) src_buffer_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    ecma_compiled_code_t *bytecode_p = ECMA_GET_NON_NULL_POINTER (ecma_compiled_code_t,
                                                                  src_literal_start_p[i]);

    if (bytecode_p == compiled_code_p)
    {
      dst_literal_start_p[i] = start_offset;
    }
    else
    {
      dst_literal_start_p[i] = snapshot_add_compiled_code (bytecode_p,
                                                           snapshot_buffer_p,
                                                           snapshot_buffer_size,
                                                           globals_p);
    }
  }

  return start_offset;
} /* snapshot_add_compiled_code */

/**
 * Set the uint16_t offsets in the code area.
 */
static void
jerry_snapshot_set_offsets (uint8_t *buffer_p, /**< buffer */
                            uint32_t size, /**< buffer size */
                            lit_mem_to_snapshot_id_map_entry_t *lit_map_p) /**< literal map */
{
  JERRY_ASSERT (size > 0);

  do
  {
    ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) buffer_p;
    uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

    if (bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION)
    {
      jmem_cpointer_t *literal_start_p;
      uint32_t argument_end;
      uint32_t register_end;
      uint32_t const_literal_end;

      if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
      {
        literal_start_p = (jmem_cpointer_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

        cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
        argument_end = args_p->argument_end;
        register_end = args_p->register_end;
        const_literal_end = args_p->const_literal_end;
      }
      else
      {
        literal_start_p = (jmem_cpointer_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

        cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
        argument_end = args_p->argument_end;
        register_end = args_p->register_end;
        const_literal_end = args_p->const_literal_end;
      }

      uint32_t register_clear_start = 0;

      if ((bytecode_p->status_flags & CBC_CODE_FLAGS_ARGUMENTS_NEEDED)
          && !(bytecode_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE))
      {
        for (uint32_t i = 0; i < argument_end; i++)
        {
          lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

          if (literal_start_p[i] != JMEM_CP_NULL)
          {
            while (current_p->literal_id != literal_start_p[i])
            {
              current_p++;
            }

            literal_start_p[i] = current_p->literal_offset;
          }
        }

        register_clear_start = argument_end;
      }

      for (uint32_t i = register_clear_start; i < register_end; i++)
      {
        literal_start_p[i] = JMEM_CP_NULL;
      }

      for (uint32_t i = register_end; i < const_literal_end; i++)
      {
        lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

        if (literal_start_p[i] != JMEM_CP_NULL)
        {
          while (current_p->literal_id != literal_start_p[i])
          {
            current_p++;
          }

          literal_start_p[i] = current_p->literal_offset;
        }
      }

      /* Set reference counter to 1. */
      bytecode_p->refs = 1;
    }

    buffer_p += code_size;
    size -= code_size;
  }
  while (size > 0);
} /* jerry_snapshot_set_offsets */

#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC

/**
 * Byte code blocks shorter than this threshold are always copied into the memory.
 * The memory / performance trade-of of byte code redirection does not worth
 * in such cases.
 */
#define BYTECODE_NO_COPY_THRESHOLD 8

/**
 * Load byte code from snapshot.
 *
 * @return byte code
 */
static ecma_compiled_code_t *
snapshot_load_compiled_code (const uint8_t *snapshot_data_p, /**< snapshot data */
                             size_t offset, /**< byte code offset */
                             lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< literal map */
                             bool copy_bytecode) /**< byte code should be copied to memory */
{
  ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) (snapshot_data_p + offset);
  uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

  if (!(bytecode_p->status_flags & CBC_CODE_FLAGS_FUNCTION))
  {
#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
    const re_compiled_code_t *re_bytecode_p = NULL;

    const uint8_t *regex_start_p = ((const uint8_t *) bytecode_p) + sizeof (ecma_compiled_code_t);

    /* Real size is stored in refs. */
    ecma_string_t *pattern_str_p = ecma_new_ecma_string_from_utf8 (regex_start_p,
                                                                   bytecode_p->refs);

    re_compile_bytecode (&re_bytecode_p,
                         pattern_str_p,
                         bytecode_p->status_flags);

    ecma_deref_ecma_string (pattern_str_p);

    return (ecma_compiled_code_t *) re_bytecode_p;
#else /* CONFIG_DISABLE_REGEXP_BUILTIN */
    JERRY_UNREACHABLE (); /* RegExp is not supported in the selected profile. */
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
  }

  size_t header_size;
  uint32_t literal_end;
  uint32_t const_literal_end;

  if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    uint8_t *byte_p = (uint8_t *) bytecode_p;
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) byte_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
    header_size = sizeof (cbc_uint16_arguments_t);
  }
  else
  {
    uint8_t *byte_p = (uint8_t *) bytecode_p;
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) byte_p;
    literal_end = args_p->literal_end;
    const_literal_end = args_p->const_literal_end;
    header_size = sizeof (cbc_uint8_arguments_t);
  }

  if (copy_bytecode
      || (header_size + (literal_end * sizeof (uint16_t)) + BYTECODE_NO_COPY_THRESHOLD > code_size))
  {
    bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block (code_size);

    memcpy (bytecode_p, snapshot_data_p + offset, code_size);
  }
  else
  {
    code_size = (uint32_t) (header_size + literal_end * sizeof (jmem_cpointer_t));

    uint8_t *real_bytecode_p = ((uint8_t *) bytecode_p) + code_size;
    uint32_t total_size = JERRY_ALIGNUP (code_size + 1 + sizeof (uint8_t *), JMEM_ALIGNMENT);

    bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block (total_size);

    memcpy (bytecode_p, snapshot_data_p + offset, code_size);

    bytecode_p->size = (uint16_t) (total_size >> JMEM_ALIGNMENT_LOG);

    uint8_t *instructions_p = ((uint8_t *) bytecode_p);

    instructions_p[code_size] = CBC_SET_BYTECODE_PTR;
    memcpy (instructions_p + code_size + 1, &real_bytecode_p, sizeof (uint8_t *));
  }

  JERRY_ASSERT (bytecode_p->refs == 1);

  jmem_cpointer_t *literal_start_p = (jmem_cpointer_t *) (((uint8_t *) bytecode_p) + header_size);

  for (uint32_t i = 0; i < const_literal_end; i++)
  {
    lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

    if (literal_start_p[i] != 0)
    {
      while (current_p->literal_offset != literal_start_p[i])
      {
        current_p++;
      }

      literal_start_p[i] = current_p->literal_id;
    }
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    size_t literal_offset = ((size_t) literal_start_p[i]) << JMEM_ALIGNMENT_LOG;

    if (literal_offset == offset)
    {
      /* Self reference */
      ECMA_SET_NON_NULL_POINTER (literal_start_p[i],
                                 bytecode_p);
    }
    else
    {
      ecma_compiled_code_t *literal_bytecode_p;
      literal_bytecode_p = snapshot_load_compiled_code (snapshot_data_p,
                                                        literal_offset,
                                                        lit_map_p,
                                                        copy_bytecode);

      ECMA_SET_NON_NULL_POINTER (literal_start_p[i],
                                 literal_bytecode_p);
    }
  }

  return bytecode_p;
} /* snapshot_load_compiled_code */

#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */

/**
 * Generate snapshot from specified source
 *
 * @return size of snapshot, if it was generated succesfully
 *          (i.e. there are no syntax errors in source code, buffer size is sufficient,
 *           and snapshot support is enabled in current configuration through JERRY_ENABLE_SNAPSHOT_SAVE),
 *         0 - otherwise.
 */
size_t
jerry_parse_and_save_snapshot (const jerry_char_t *source_p, /**< script source */
                               size_t source_size, /**< script source size */
                               bool is_for_global, /**< snapshot would be executed as global (true)
                                                    *   or eval (false) */
                               bool is_strict, /**< strict mode */
                               uint8_t *buffer_p, /**< buffer to save snapshot to */
                               size_t buffer_size) /**< the buffer's size */
{
#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
  snapshot_globals_t globals;
  ecma_value_t parse_status;
  ecma_compiled_code_t *bytecode_data_p;

  globals.snapshot_buffer_write_offset = JERRY_ALIGNUP (sizeof (jerry_snapshot_header_t),
                                                        JMEM_ALIGNMENT);
  globals.snapshot_error_occured = false;

  parse_status = parser_parse_script (source_p,
                                      source_size,
                                      is_strict,
                                      &bytecode_data_p);

  if (ECMA_IS_VALUE_ERROR (parse_status))
  {
    ecma_free_value (parse_status);
    return 0;
  }

  snapshot_add_compiled_code (bytecode_data_p, buffer_p, buffer_size, &globals);

  if (globals.snapshot_error_occured)
  {
    return 0;
  }

  jerry_snapshot_header_t header;
  header.version = JERRY_SNAPSHOT_VERSION;
  header.lit_table_offset = (uint32_t) globals.snapshot_buffer_write_offset;
  header.is_run_global = is_for_global;

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num;

  if (!ecma_save_literals_for_snapshot (buffer_p,
                                        buffer_size,
                                        &globals.snapshot_buffer_write_offset,
                                        &lit_map_p,
                                        &literals_num,
                                        &header.lit_table_size))
  {
    JERRY_ASSERT (lit_map_p == NULL);
    return 0;
  }

  jerry_snapshot_set_offsets (buffer_p + JERRY_ALIGNUP (sizeof (jerry_snapshot_header_t), JMEM_ALIGNMENT),
                              (uint32_t) (header.lit_table_offset - sizeof (jerry_snapshot_header_t)),
                              lit_map_p);

  size_t header_offset = 0;

  snapshot_write_to_buffer_by_offset (buffer_p,
                                      buffer_size,
                                      &header_offset,
                                      &header,
                                      sizeof (header));

  if (lit_map_p != NULL)
  {
    jmem_heap_free_block (lit_map_p, literals_num * sizeof (lit_mem_to_snapshot_id_map_entry_t));
  }

  ecma_bytecode_deref (bytecode_data_p);

  return globals.snapshot_buffer_write_offset;
#else /* !JERRY_ENABLE_SNAPSHOT_SAVE */
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (is_for_global);
  JERRY_UNUSED (is_strict);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (buffer_size);

  return 0;
#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */
} /* jerry_parse_and_save_snapshot */

/**
 * Execute snapshot from specified buffer
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of bytecode - if run was successful
 *         thrown error - otherwise
 */
jerry_value_t
jerry_exec_snapshot (const void *snapshot_p, /**< snapshot */
                     size_t snapshot_size, /**< size of snapshot */
                     bool copy_bytecode) /**< flag, indicating whether the passed snapshot
                                          *   buffer should be copied to the engine's memory.
                                          *   If set the engine should not reference the buffer
                                          *   after the function returns (in this case, the passed
                                          *   buffer could be freed after the call).
                                          *   Otherwise (if the flag is not set) - the buffer could only be
                                          *   freed after the engine stops (i.e. after call to jerry_cleanup). */
{
#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
  JERRY_ASSERT (snapshot_p != NULL);

  static const char * const invalid_version_error_p = "Invalid snapshot version";
  static const char * const invalid_format_error_p = "Invalid snapshot format";
  const uint8_t *snapshot_data_p = (uint8_t *) snapshot_p;

  if (snapshot_size <= sizeof (jerry_snapshot_header_t))
  {
    return ecma_raise_type_error (invalid_format_error_p);
  }

  const jerry_snapshot_header_t *header_p = (const jerry_snapshot_header_t *) snapshot_data_p;

  if (header_p->version != JERRY_SNAPSHOT_VERSION)
  {
    return ecma_raise_type_error (invalid_version_error_p);
  }

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num;

  if (header_p->lit_table_offset >= snapshot_size)
  {
    return ecma_raise_type_error (invalid_version_error_p);
  }

  if (!ecma_load_literals_from_snapshot (snapshot_data_p + header_p->lit_table_offset,
                                         header_p->lit_table_size,
                                         &lit_map_p,
                                         &literals_num))
  {
    JERRY_ASSERT (lit_map_p == NULL);
    return ecma_raise_type_error (invalid_format_error_p);
  }

  ecma_compiled_code_t *bytecode_p;
  bytecode_p = snapshot_load_compiled_code (snapshot_data_p,
                                            sizeof (jerry_snapshot_header_t),
                                            lit_map_p,
                                            copy_bytecode);

  if (lit_map_p != NULL)
  {
    jmem_heap_free_block (lit_map_p, literals_num * sizeof (lit_mem_to_snapshot_id_map_entry_t));
  }

  if (bytecode_p == NULL)
  {
    return ecma_raise_type_error (invalid_format_error_p);
  }

  ecma_value_t ret_val;

  if (header_p->is_run_global)
  {
    ret_val = vm_run_global (bytecode_p);
    ecma_bytecode_deref (bytecode_p);
  }
  else
  {
    ret_val = vm_run_eval (bytecode_p, false);
  }

  return ret_val;
#else /* !JERRY_ENABLE_SNAPSHOT_EXEC */
  JERRY_UNUSED (snapshot_p);
  JERRY_UNUSED (snapshot_size);
  JERRY_UNUSED (copy_bytecode);

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */
} /* jerry_exec_snapshot */

/**
 * @}
 */

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE

/**
 * ====================== Functions for literal saving ==========================
 */

/**
 * Compare two ecma_strings by size, then lexicographically.
 *
 * @return true - if the first string is less than the second one,
 *         false - otherwise.
 */
static bool
jerry_save_literals_compare (ecma_string_t *literal1, /**< first literal */
                             ecma_string_t *literal2) /**< second literal */
{
  const lit_utf8_size_t lit1_size = ecma_string_get_size (literal1);
  const lit_utf8_size_t lit2_size = ecma_string_get_size (literal2);

  if (lit1_size == lit2_size)
  {
    return ecma_compare_ecma_strings_relational (literal1, literal2);
  }

  return (lit1_size < lit2_size);
} /* jerry_save_literals_compare */

/**
 * Helper function for the heapsort algorithm.
 *
 * @return index of the maximum value
 */
static lit_utf8_size_t
jerry_save_literals_heap_max (ecma_string_t *literals[], /**< array of literals */
                              lit_utf8_size_t num_of_nodes, /**< number of nodes */
                              lit_utf8_size_t node_idx, /**< index of parent node */
                              lit_utf8_size_t child_idx1, /**< index of the first child */
                              lit_utf8_size_t child_idx2) /**< index of the second child */
{
  lit_utf8_size_t max_idx = node_idx;

  if (child_idx1 < num_of_nodes
      && jerry_save_literals_compare (literals[max_idx], literals[child_idx1]))
  {
    max_idx = child_idx1;
  }

  if (child_idx2 < num_of_nodes
      && jerry_save_literals_compare (literals[max_idx], literals[child_idx2]))
  {
    max_idx = child_idx2;
  }

  return max_idx;
} /* jerry_save_literals_heap_max */

/**
 * Helper function for the heapsort algorithm.
 */
static void
jerry_save_literals_down_heap (ecma_string_t *literals[], /**< array of literals */
                               lit_utf8_size_t num_of_nodes, /**< number of nodes */
                               lit_utf8_size_t node_idx) /**< index of parent node */
{
  while (true)
  {
    lit_utf8_size_t max_idx = jerry_save_literals_heap_max (literals,
                                                            num_of_nodes,
                                                            node_idx,
                                                            2 * node_idx + 1,
                                                            2 * node_idx + 2);
    if (max_idx == node_idx)
    {
      break;
    }

    ecma_string_t *tmp_str_p  = literals[node_idx];
    literals[node_idx] = literals[max_idx];
    literals[max_idx] = tmp_str_p;

    node_idx = max_idx;
  }
} /* jerry_save_literals_down_heap */

/**
 * Helper function for a heapsort algorithm.
 */
static void
jerry_save_literals_sort (ecma_string_t *literals[], /**< array of literals */
                          lit_utf8_size_t num_of_literals) /**< number of literals */
{
  if (num_of_literals < 2)
  {
    return;
  }

  lit_utf8_size_t lit_idx = (num_of_literals - 2) / 2;

  while (lit_idx <= (num_of_literals - 2) / 2)
  {
    jerry_save_literals_down_heap (literals, num_of_literals, lit_idx--);
  }

  for (lit_idx = 0; lit_idx < num_of_literals; lit_idx++)
  {
    const lit_utf8_size_t last_idx = num_of_literals - lit_idx - 1;

    ecma_string_t *tmp_str_p = literals[last_idx];
    literals[last_idx] = literals[0];
    literals[0] = tmp_str_p;

    jerry_save_literals_down_heap (literals, last_idx, 0);
  }
} /* jerry_save_literals_sort */

/**
 * Append characters to the specified buffer.
 *
 * @return the position of the buffer pointer after copy.
 */
static uint8_t *
jerry_append_chars_to_buffer (uint8_t *buffer_p, /**< buffer */
                              uint8_t *buffer_end_p, /**< the end of the buffer */
                              const char *chars, /**< string */
                              lit_utf8_size_t string_size) /**< string size */
{
  if (buffer_p > buffer_end_p)
  {
    return buffer_p;
  }

  if (string_size == 0)
  {
    string_size = (lit_utf8_size_t) strlen (chars);
  }

  if (buffer_p + string_size <= buffer_end_p)
  {
    memcpy ((char *) buffer_p, chars, string_size);

    return buffer_p + string_size;
  }

  /* Move the pointer behind the buffer to prevent further writes. */
  return buffer_end_p + 1;
} /* jerry_append_chars_to_buffer */

/**
 * Append an ecma-string to the specified buffer.
 *
 * @return the position of the buffer pointer after copy.
 */
static uint8_t *
jerry_append_ecma_string_to_buffer (uint8_t *buffer_p, /**< buffer */
                                    uint8_t *buffer_end_p, /**< the end of the buffer */
                                    ecma_string_t *string_p) /**< ecma-string */
{
  uint8_t *new_buffer_p = NULL;

  ECMA_STRING_TO_UTF8_STRING (string_p, str_buffer_p, str_buffer_size);

  /* Append the string to the buffer. */
  new_buffer_p = jerry_append_chars_to_buffer (buffer_p,
                                               buffer_end_p,
                                               (const char *) str_buffer_p,
                                               str_buffer_size);

  ECMA_FINALIZE_UTF8_STRING (str_buffer_p, str_buffer_size);

  return new_buffer_p;
} /* jerry_append_ecma_string_to_buffer */

/**
 * Append an unsigned number to the specified buffer.
 *
 * @return the position of the buffer pointer after copy.
 */
static uint8_t *
jerry_append_number_to_buffer (uint8_t *buffer_p, /**< buffer */
                               uint8_t *buffer_end_p, /**< the end of the buffer */
                               lit_utf8_size_t number) /**< number */
{
  lit_utf8_byte_t uint32_to_str_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  lit_utf8_size_t utf8_str_size = ecma_uint32_to_utf8_string (number,
                                                              uint32_to_str_buffer,
                                                              ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);

  JERRY_ASSERT (utf8_str_size <= ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);

  return jerry_append_chars_to_buffer (buffer_p,
                                       buffer_end_p,
                                       (const char *) uint32_to_str_buffer,
                                       utf8_str_size);
} /* jerry_append_number_to_buffer */

/**
 * Check whether the passed ecma-string is a valid identifier.
 *
 * @return true, if the ecma-string is a valid identifier,
 *         false - otherwise.
 */
static bool
ecma_string_is_valid_identifier (const ecma_string_t *string_p)
{
  bool result = false;

  ECMA_STRING_TO_UTF8_STRING (string_p, str_buffer_p, str_buffer_size);

  if (lit_char_is_identifier_start (str_buffer_p))
  {
    const uint8_t *str_start_p = str_buffer_p;
    const uint8_t *str_end_p = str_buffer_p + str_buffer_size;

    result = true;

    while (str_start_p < str_end_p)
    {
      if (!lit_char_is_identifier_part (str_start_p))
      {
        result = false;
        break;
      }
      lit_utf8_incr (&str_start_p);
    }
  }

  ECMA_FINALIZE_UTF8_STRING (str_buffer_p, str_buffer_size);

  return result;
} /* ecma_string_is_valid_identifier */

#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

/**
 * Copy certain string literals into the given buffer in a specified format,
 * which are valid identifiers and none of them are magic string.
 *
 * @return size of the literal-list in bytes, at most equal to the buffer size,
 *         if the source parsed successfully and the list of the literals isn't empty,
 *         0 - otherwise.
 */
size_t
jerry_parse_and_save_literals (const jerry_char_t *source_p, /**< script source */
                               size_t source_size, /**< script source size */
                               bool is_strict, /**< strict mode */
                               uint8_t *buffer_p, /**< [out] buffer to save literals to */
                               size_t buffer_size, /**< the buffer's size */
                               bool is_c_format) /**< format-flag */
{
#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
  ecma_value_t parse_status;
  ecma_compiled_code_t *bytecode_data_p;
  parse_status = parser_parse_script (source_p,
                                      source_size,
                                      is_strict,
                                      &bytecode_data_p);

  const bool error = ECMA_IS_VALUE_ERROR (parse_status);
  ecma_free_value (parse_status);

  if (error)
  {
    return 0;
  }

  ecma_bytecode_deref (bytecode_data_p);

  ecma_lit_storage_item_t *string_list_p = JERRY_CONTEXT (string_list_first_p);
  lit_utf8_size_t literal_count = 0;

  /* Count the valid and non-magic identifiers in the list. */
  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] != JMEM_CP_NULL)
      {
        ecma_string_t *literal_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                 string_list_p->values[i]);
        /* We don't save a literal which isn't a valid identifier
           or it's a magic string. */
        if (ecma_get_string_magic (literal_p) == LIT_MAGIC_STRING__COUNT
            && ecma_string_is_valid_identifier (literal_p))
        {
          literal_count++;
        }
      }
    }

    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, string_list_p->next_cp);
  }

  if (literal_count == 0)
  {
    return 0;
  }

  uint8_t * const buffer_start_p = buffer_p;
  uint8_t * const buffer_end_p = buffer_p + buffer_size;

  JMEM_DEFINE_LOCAL_ARRAY (literal_array, literal_count, ecma_string_t *);
  lit_utf8_size_t literal_idx = 0;

  string_list_p = JERRY_CONTEXT (string_list_first_p);

  while (string_list_p != NULL)
  {
    for (int i = 0; i < ECMA_LIT_STORAGE_VALUE_COUNT; i++)
    {
      if (string_list_p->values[i] != JMEM_CP_NULL)
      {
        ecma_string_t *literal_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                 string_list_p->values[i]);

        if (ecma_get_string_magic (literal_p) == LIT_MAGIC_STRING__COUNT
            && ecma_string_is_valid_identifier (literal_p))
        {
          literal_array[literal_idx++] = literal_p;
        }
      }
    }

    string_list_p = JMEM_CP_GET_POINTER (ecma_lit_storage_item_t, string_list_p->next_cp);
  }

  /* Sort the strings by size at first, then lexicographically. */
  jerry_save_literals_sort (literal_array, literal_count);

  if (is_c_format)
  {
    /* Save literal count. */
    buffer_p = jerry_append_chars_to_buffer (buffer_p,
                                             buffer_end_p,
                                             (const char *) "jerry_length_t literal_count = ",
                                             0);

    buffer_p = jerry_append_number_to_buffer (buffer_p, buffer_end_p, literal_count);

    /* Save the array of literals. */
    buffer_p = jerry_append_chars_to_buffer (buffer_p,
                                             buffer_end_p,
                                             ";\n\njerry_char_ptr_t literals[",
                                             0);

    buffer_p = jerry_append_number_to_buffer (buffer_p, buffer_end_p, literal_count);
    buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "] =\n{\n", 0);

    for (lit_utf8_size_t i = 0; i < literal_count; i++)
    {
      buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "  \"", 0);
      buffer_p = jerry_append_ecma_string_to_buffer (buffer_p, buffer_end_p, literal_array[i]);
      buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "\"", 0);

      if (i < literal_count - 1)
      {
        buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, ",", 0);
      }

      buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "\n", 0);
    }

    buffer_p = jerry_append_chars_to_buffer (buffer_p,
                                             buffer_end_p,
                                             (const char *) "};\n\njerry_length_t literal_sizes[",
                                             0);

    buffer_p = jerry_append_number_to_buffer (buffer_p, buffer_end_p, literal_count);
    buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "] =\n{\n", 0);
  }

  /* Save the literal sizes respectively. */
  for (lit_utf8_size_t i = 0; i < literal_count; i++)
  {
    lit_utf8_size_t str_size = ecma_string_get_size (literal_array[i]);

    if (is_c_format)
    {
      buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "  ", 0);
    }

    buffer_p = jerry_append_number_to_buffer (buffer_p, buffer_end_p, str_size);
    buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, " ", 0);

    if (is_c_format)
    {
      /* Show the given string as a comment. */
      buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "/* ", 0);
      buffer_p = jerry_append_ecma_string_to_buffer (buffer_p, buffer_end_p, literal_array[i]);
      buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, " */", 0);

      if (i < literal_count - 1)
      {
        buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, ",", 0);
      }
    }
    else
    {
      buffer_p = jerry_append_ecma_string_to_buffer (buffer_p, buffer_end_p, literal_array[i]);
    }

    buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, "\n", 0);
  }

  if (is_c_format)
  {
    buffer_p = jerry_append_chars_to_buffer (buffer_p, buffer_end_p, (const char *) "};\n", 0);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (literal_array);

  return buffer_p <= buffer_end_p ? (size_t) (buffer_p - buffer_start_p) : 0;
#else /* !JERRY_ENABLE_SNAPSHOT_SAVE */
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (is_strict);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (buffer_size);
  JERRY_UNUSED (is_c_format);

  return 0;
#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */
} /* jerry_parse_and_save_literals */
