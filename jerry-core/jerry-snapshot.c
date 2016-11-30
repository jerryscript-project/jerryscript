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
#include "jerry-api.h"
#include "jerry-snapshot.h"
#include "js-parser.h"
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
