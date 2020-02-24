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

#include "ecma-helpers.h"
#include "jcontext.h"
#include "js-parser-internal.h"
#include "js-scanner-internal.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_PARSER)

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_scanner Scanner
 * @{
 */

JERRY_STATIC_ASSERT (PARSER_MAXIMUM_NUMBER_OF_LITERALS + PARSER_MAXIMUM_NUMBER_OF_REGISTERS < PARSER_REGISTER_START,
                     maximum_number_of_literals_plus_registers_must_be_less_than_register_start);

#if ENABLED (JERRY_ES2015)

JERRY_STATIC_ASSERT ((SCANNER_LITERAL_IS_ARROW_DESTRUCTURED_ARG & SCANNER_LITERAL_IS_LOCAL) == 0,
                     is_arrow_arg_binding_flag_must_not_use_local_flags);

JERRY_STATIC_ASSERT ((SCANNER_LITERAL_ARROW_DESTRUCTURED_ARG_NO_REG & SCANNER_LITERAL_IS_LOCAL) == 0,
                     arrow_arg_binding_no_reg_flag_must_not_use_local_flags);

JERRY_STATIC_ASSERT ((SCANNER_LITERAL_IS_LET & SCANNER_LITERAL_IS_LOCAL) != 0,
                     is_let_flag_must_use_local_flags);

JERRY_STATIC_ASSERT ((SCANNER_LITERAL_IS_CONST & SCANNER_LITERAL_IS_LOCAL) != 0,
                     is_const_flag_must_use_local_flags);

JERRY_STATIC_ASSERT ((SCANNER_LITERAL_IS_FUNC_DECLARATION & SCANNER_LITERAL_IS_LOCAL) != 0,
                     is_func_declaration_flag_must_use_local_flags);

JERRY_STATIC_ASSERT ((SCANNER_LITERAL_IS_DESTRUCTURED_ARG & SCANNER_LITERAL_IS_LOCAL) != 0,
                     is_arg_binding_flag_must_use_local_flags);

JERRY_STATIC_ASSERT (SCANNER_LITERAL_IS_FUNC_DECLARATION != SCANNER_LITERAL_IS_DESTRUCTURED_ARG,
                     is_func_declaration_must_be_different_from_is_arg_binding);

#endif /* ENABLED (JERRY_ES2015) */

/**
 * Raise a scanner error.
 */
void
scanner_raise_error (parser_context_t *context_p) /**< context */
{
  PARSER_THROW (context_p->try_buffer);
  /* Should never been reached. */
  JERRY_ASSERT (0);
} /* scanner_raise_error */

#if ENABLED (JERRY_ES2015)

/**
 * Raise a variable redeclaration error.
 */
void
scanner_raise_redeclaration_error (parser_context_t *context_p)
{
  scanner_info_t *info_p = scanner_insert_info (context_p, context_p->source_p, sizeof (scanner_info_t));
  info_p->type = SCANNER_TYPE_ERR_REDECLARED;

  scanner_raise_error (context_p);
} /* scanner_raise_redeclaration_error */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * Allocate memory for scanner.
 *
 * @return allocated memory
 */
void *
scanner_malloc (parser_context_t *context_p, /**< context */
                size_t size) /**< size of the memory block */
{
  void *result;

  JERRY_ASSERT (size > 0);
  result = jmem_heap_alloc_block_null_on_error (size);

  if (result == NULL)
  {
    scanner_cleanup (context_p);

    /* This is the only error which specify its reason. */
    context_p->error = PARSER_ERR_OUT_OF_MEMORY;
    PARSER_THROW (context_p->try_buffer);
  }
  return result;
} /* scanner_malloc */

/**
 * Free memory allocated by scanner_malloc.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_free (void *ptr, /**< pointer to free */
              size_t size) /**< size of the memory block */
{
  jmem_heap_free_block (ptr, size);
} /* scanner_free */

/**
 * Count the size of a stream after an info block.
 *
 * @return the size in bytes
 */
size_t
scanner_get_stream_size (scanner_info_t *info_p, /**< scanner info block */
                         size_t size) /**< size excluding the stream */
{
  const uint8_t *data_p = ((const uint8_t *) info_p) + size;
  const uint8_t *data_p_start = data_p;

  while (data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    switch (data_p[0] & SCANNER_STREAM_TYPE_MASK)
    {
      case SCANNER_STREAM_TYPE_VAR:
#if ENABLED (JERRY_ES2015)
      case SCANNER_STREAM_TYPE_LET:
      case SCANNER_STREAM_TYPE_CONST:
      case SCANNER_STREAM_TYPE_LOCAL:
      case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG:
#endif /* ENABLED (JERRY_ES2015) */
      case SCANNER_STREAM_TYPE_ARG:
      case SCANNER_STREAM_TYPE_ARG_FUNC:
#if ENABLED (JERRY_ES2015)
      case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC:
#endif /* ENABLED (JERRY_ES2015) */
      case SCANNER_STREAM_TYPE_FUNC:
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
      case SCANNER_STREAM_TYPE_IMPORT:
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
      {
        break;
      }
      default:
      {
        JERRY_ASSERT ((data_p[0] & SCANNER_STREAM_TYPE_MASK) == SCANNER_STREAM_TYPE_HOLE);
        data_p++;
        continue;
      }
    }

    data_p += 3;

    if (data_p[-3] & SCANNER_STREAM_UINT16_DIFF)
    {
      data_p++;
    }
    else if (data_p[-1] == 0)
    {
      data_p += sizeof (const uint8_t *);
    }
  }

  return size + 1 + (size_t) (data_p - data_p_start);
} /* scanner_get_stream_size */

/**
 * Insert a scanner info block into the scanner info chain.
 *
 * @return newly allocated scanner info
 */
scanner_info_t *
scanner_insert_info (parser_context_t *context_p, /**< context */
                     const uint8_t *source_p, /**< triggering position */
                     size_t size) /**< size of the memory block */
{
  scanner_info_t *new_scanner_info_p = (scanner_info_t *) scanner_malloc (context_p, size);
  scanner_info_t *scanner_info_p = context_p->next_scanner_info_p;
  scanner_info_t *prev_scanner_info_p = NULL;

  JERRY_ASSERT (scanner_info_p != NULL);
  JERRY_ASSERT (source_p != NULL);

  new_scanner_info_p->source_p = source_p;

  while (source_p < scanner_info_p->source_p)
  {
    prev_scanner_info_p = scanner_info_p;
    scanner_info_p = scanner_info_p->next_p;

    JERRY_ASSERT (scanner_info_p != NULL);
  }

  /* Multiple scanner info blocks cannot be assigned to the same position. */
  JERRY_ASSERT (source_p != scanner_info_p->source_p);

  new_scanner_info_p->next_p = scanner_info_p;

  if (JERRY_LIKELY (prev_scanner_info_p == NULL))
  {
    context_p->next_scanner_info_p = new_scanner_info_p;
  }
  else
  {
    prev_scanner_info_p->next_p = new_scanner_info_p;
  }

  return new_scanner_info_p;
} /* scanner_insert_info */

/**
 * Insert a scanner info block into the scanner info chain before a given info block.
 *
 * @return newly allocated scanner info
 */
scanner_info_t *
scanner_insert_info_before (parser_context_t *context_p, /**< context */
                            const uint8_t *source_p, /**< triggering position */
                            scanner_info_t *start_info_p, /**< first info position */
                            size_t size) /**< size of the memory block */
{
  JERRY_ASSERT (start_info_p != NULL);

  scanner_info_t *new_scanner_info_p = (scanner_info_t *) scanner_malloc (context_p, size);
  scanner_info_t *scanner_info_p = start_info_p->next_p;
  scanner_info_t *prev_scanner_info_p = start_info_p;

  new_scanner_info_p->source_p = source_p;

  while (source_p < scanner_info_p->source_p)
  {
    prev_scanner_info_p = scanner_info_p;
    scanner_info_p = scanner_info_p->next_p;

    JERRY_ASSERT (scanner_info_p != NULL);
  }

  /* Multiple scanner info blocks cannot be assigned to the same position. */
  JERRY_ASSERT (source_p != scanner_info_p->source_p);

  new_scanner_info_p->next_p = scanner_info_p;

  prev_scanner_info_p->next_p = new_scanner_info_p;
  return new_scanner_info_p;
} /* scanner_insert_info_before */

/**
 * Release the next scanner info.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_release_next (parser_context_t *context_p, /**< context */
                      size_t size) /**< size of the memory block */
{
  scanner_info_t *next_p = context_p->next_scanner_info_p->next_p;

  jmem_heap_free_block (context_p->next_scanner_info_p, size);
  context_p->next_scanner_info_p = next_p;
} /* scanner_release_next */

/**
 * Set the active scanner info to the next scanner info.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_set_active (parser_context_t *context_p) /**< context */
{
  scanner_info_t *scanner_info_p = context_p->next_scanner_info_p;

  context_p->next_scanner_info_p = scanner_info_p->next_p;
  scanner_info_p->next_p = context_p->active_scanner_info_p;
  context_p->active_scanner_info_p = scanner_info_p;
} /* scanner_set_active */

/**
 * Set the next scanner info to the active scanner info.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_revert_active (parser_context_t *context_p) /**< context */
{
  scanner_info_t *scanner_info_p = context_p->active_scanner_info_p;

  context_p->active_scanner_info_p = scanner_info_p->next_p;
  scanner_info_p->next_p = context_p->next_scanner_info_p;
  context_p->next_scanner_info_p = scanner_info_p;
} /* scanner_revert_active */

/**
 * Release the active scanner info.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_release_active (parser_context_t *context_p, /**< context */
                        size_t size) /**< size of the memory block */
{
  scanner_info_t *next_p = context_p->active_scanner_info_p->next_p;

  jmem_heap_free_block (context_p->active_scanner_info_p, size);
  context_p->active_scanner_info_p = next_p;
} /* scanner_release_active */

/**
 * Release switch cases.
 */
void
scanner_release_switch_cases (scanner_case_info_t *case_p) /**< case list */
{
  while (case_p != NULL)
  {
    scanner_case_info_t *next_p = case_p->next_p;

    jmem_heap_free_block (case_p, sizeof (scanner_case_info_t));
    case_p = next_p;
  }
} /* scanner_release_switch_cases */

/**
 * Seek to correct position in the scanner info list.
 */
void
scanner_seek (parser_context_t *context_p) /**< context */
{
  const uint8_t *source_p = context_p->source_p;
  scanner_info_t *prev_p;

  if (context_p->skipped_scanner_info_p != NULL)
  {
    JERRY_ASSERT (context_p->skipped_scanner_info_p->source_p != NULL);

    context_p->skipped_scanner_info_end_p->next_p = context_p->next_scanner_info_p;

    if (context_p->skipped_scanner_info_end_p->source_p <= source_p)
    {
      prev_p = context_p->skipped_scanner_info_end_p;
    }
    else
    {
      prev_p = context_p->skipped_scanner_info_p;

      if (prev_p->source_p > source_p)
      {
        context_p->next_scanner_info_p = prev_p;
        context_p->skipped_scanner_info_p = NULL;
        return;
      }

      context_p->skipped_scanner_info_p = prev_p;
    }
  }
  else
  {
    prev_p = context_p->next_scanner_info_p;

    if (prev_p->source_p == NULL || prev_p->source_p > source_p)
    {
      return;
    }

    context_p->skipped_scanner_info_p = prev_p;
  }

  while (prev_p->next_p->source_p != NULL && prev_p->next_p->source_p <= source_p)
  {
    prev_p = prev_p->next_p;
  }

  context_p->skipped_scanner_info_end_p = prev_p;
  context_p->next_scanner_info_p = prev_p->next_p;
} /* scanner_seek */

/**
 * Push a new literal pool.
 *
 * @return the newly created literal pool
 */
scanner_literal_pool_t *
scanner_push_literal_pool (parser_context_t *context_p, /**< context */
                           scanner_context_t *scanner_context_p, /**< scanner context */
                           uint16_t status_flags) /**< combination of scanner_literal_pool_flags_t flags */
{
  scanner_literal_pool_t *prev_literal_pool_p = scanner_context_p->active_literal_pool_p;
  scanner_literal_pool_t *literal_pool_p;

  literal_pool_p = (scanner_literal_pool_t *) scanner_malloc (context_p, sizeof (scanner_literal_pool_t));

  if (!(status_flags & SCANNER_LITERAL_POOL_FUNCTION))
  {
    JERRY_ASSERT (prev_literal_pool_p != NULL);
    status_flags |= SCANNER_LITERAL_POOL_NO_ARGUMENTS;

#if ENABLED (JERRY_ES2015)
    const uint16_t copied_flags = SCANNER_LITERAL_POOL_IN_WITH | SCANNER_LITERAL_POOL_GENERATOR;
#else /* !ENABLED (JERRY_ES2015) */
    const uint16_t copied_flags = SCANNER_LITERAL_POOL_IN_WITH;
#endif /* ENABLED (JERRY_ES2015) */

    status_flags |= (uint16_t) (prev_literal_pool_p->status_flags & copied_flags);
  }

  if (prev_literal_pool_p != NULL)
  {
    const uint16_t copied_flags = SCANNER_LITERAL_POOL_IS_STRICT;
    status_flags |= (uint16_t) (prev_literal_pool_p->status_flags & copied_flags);

    /* The logical value of these flags must be the same. */
    JERRY_ASSERT (!(status_flags & SCANNER_LITERAL_POOL_IS_STRICT) == !(context_p->status_flags & PARSER_IS_STRICT));
  }

  parser_list_init (&literal_pool_p->literal_pool,
                    sizeof (lexer_lit_location_t),
                    (uint32_t) ((128 - sizeof (void *)) / sizeof (lexer_lit_location_t)));
  literal_pool_p->source_p = NULL;
  literal_pool_p->status_flags = status_flags;
  literal_pool_p->no_declarations = 0;

  literal_pool_p->prev_p = prev_literal_pool_p;
  scanner_context_p->active_literal_pool_p = literal_pool_p;

  return literal_pool_p;
} /* scanner_push_literal_pool */

JERRY_STATIC_ASSERT (PARSER_MAXIMUM_IDENT_LENGTH <= UINT8_MAX,
                     maximum_ident_length_must_fit_in_a_byte);

/**
 * Checks whether a literal is equal to "arguments".
 */
static inline bool JERRY_ATTR_ALWAYS_INLINE
scanner_literal_is_arguments (lexer_lit_location_t *literal_p) /**< literal */
{
  return lexer_compare_identifier_to_string (literal_p, (const uint8_t *) "arguments", 9);
} /* scanner_literal_is_arguments */

/**
 * Pop the last literal pool from the end.
 */
void
scanner_pop_literal_pool (parser_context_t *context_p, /**< context */
                          scanner_context_t *scanner_context_p) /**< scanner context */
{
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;
  scanner_literal_pool_t *prev_literal_pool_p = literal_pool_p->prev_p;

  if (literal_pool_p->source_p == NULL)
  {
    JERRY_ASSERT (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION);
    JERRY_ASSERT (literal_pool_p->literal_pool.data.first_p == NULL
                  && literal_pool_p->literal_pool.data.last_p == NULL);

    scanner_context_p->active_literal_pool_p = literal_pool_p->prev_p;
    scanner_free (literal_pool_p, sizeof (scanner_literal_pool_t));
    return;
  }

  parser_list_iterator_t literal_iterator;
  lexer_lit_location_t *literal_p;
  bool is_function = (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION) != 0;
  bool no_reg = (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_NO_REG) != 0;
  bool search_arguments = (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_NO_ARGUMENTS) == 0;
  bool arguments_required = (no_reg && search_arguments);

  uint8_t no_reg_types = 0;
#if ENABLED (JERRY_ES2015)
  if (prev_literal_pool_p == NULL && !(context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL))
  {
    no_reg_types |= SCANNER_LITERAL_IS_FUNC;
  }
#endif /* !ENABLED (JERRY_ES2015) */

  if (no_reg && prev_literal_pool_p != NULL)
  {
    prev_literal_pool_p->status_flags |= SCANNER_LITERAL_POOL_NO_REG;
  }

#if ENABLED (JERRY_DEBUGGER)
  if (scanner_context_p->debugger_enabled)
  {
    /* When debugger is enabled, identifiers are not stored in registers. However,
     * this does not affect 'eval' detection, so 'arguments' object is not created. */
    no_reg = true;
  }
#endif /* ENABLED (JERRY_DEBUGGER) */

  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);

  const uint8_t *prev_source_p = literal_pool_p->source_p - 1;
  size_t compressed_size = 1;
  uint32_t no_declarations = literal_pool_p->no_declarations;

  while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    uint8_t type = literal_p->type;

    if (JERRY_UNLIKELY (no_declarations > PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK))
    {
      continue;
    }

    if (search_arguments && scanner_literal_is_arguments (literal_p))
    {
      search_arguments = false;

      if (type & (SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_IS_FUNC | SCANNER_LITERAL_IS_LOCAL))
      {
        arguments_required = false;
      }
      else
      {
        literal_p->type = 0;
        arguments_required = true;
        continue;
      }
    }

#if ENABLED (JERRY_ES2015)
    if (is_function && (type & (SCANNER_LITERAL_IS_FUNC | SCANNER_LITERAL_IS_LOCAL)) == SCANNER_LITERAL_IS_FUNC)
    {
      if (prev_literal_pool_p == NULL
          && (context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL)
          && scanner_scope_find_let_declaration (context_p, literal_p))
      {
        literal_p->type = 0;
        continue;
      }

      type = (uint8_t) ((type & ~SCANNER_LITERAL_IS_FUNC) | SCANNER_LITERAL_IS_VAR);
      literal_p->type = type;
    }
#endif /* ENABLED (JERRY_ES2015) */

    if ((is_function && (type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_ARG)))
        || (type & SCANNER_LITERAL_IS_LOCAL))
    {
      JERRY_ASSERT (is_function || !(literal_p->type & SCANNER_LITERAL_IS_ARG));

      if (literal_p->length == 0)
      {
        compressed_size += 1;
        continue;
      }

      no_declarations++;

      if (type & SCANNER_LITERAL_IS_FUNC)
      {
        no_declarations++;
      }

      if (no_reg || (type & no_reg_types))
      {
        type |= SCANNER_LITERAL_NO_REG;
        literal_p->type = type;
      }

      intptr_t diff = (intptr_t) (literal_p->char_p - prev_source_p);

      if (diff >= 1 && diff <= UINT8_MAX)
      {
        compressed_size += 2 + 1;
      }
      else if (diff >= -UINT8_MAX && diff <= UINT16_MAX)
      {
        compressed_size += 2 + 2;
      }
      else
      {
        compressed_size += 2 + 1 + sizeof (const uint8_t *);
      }

      prev_source_p = literal_p->char_p + literal_p->length;

      if (is_function
#if ENABLED (JERRY_ES2015)
          || ((type & SCANNER_LITERAL_IS_FUNC) && (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_IS_STRICT))
#endif /* ENABLED (JERRY_ES2015) */
          || !(type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_FUNC)))
      {
        continue;
      }
    }

    if (prev_literal_pool_p != NULL && literal_p->length > 0)
    {
      /* Propagate literal to upper level. */
      lexer_lit_location_t *literal_location_p = scanner_add_custom_literal (context_p,
                                                                             prev_literal_pool_p,
                                                                             literal_p);
      uint8_t extended_type = literal_location_p->type;

      if (is_function || (type & SCANNER_LITERAL_NO_REG))
      {
        extended_type |= SCANNER_LITERAL_NO_REG;
      }

#if ENABLED (JERRY_ES2015)
      extended_type |= SCANNER_LITERAL_IS_USED;

      if ((type & SCANNER_LITERAL_IS_ARG)
          || (!(literal_location_p->type & SCANNER_LITERAL_IS_ARG)
              && (literal_location_p->type & SCANNER_LITERAL_IS_LOCAL)))
      {
        /* Clears the SCANNER_LITERAL_IS_VAR and SCANNER_LITERAL_IS_FUNC flags
         * for speculative arrow paramters and local (non-var) functions. */
        type = 0;
      }
#endif /* ENABLED (JERRY_ES2015) */

      type = (uint8_t) (type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_FUNC));
      JERRY_ASSERT (type == 0 || !is_function);

      literal_location_p->type = (uint8_t) (extended_type | type);
    }
  }

  if (is_function || (compressed_size > 1))
  {
    compressed_size += sizeof (scanner_info_t);

    scanner_info_t *info_p;

    if (prev_literal_pool_p != NULL || scanner_context_p->end_arguments_p == NULL)
    {
      info_p = scanner_insert_info (context_p, literal_pool_p->source_p, compressed_size);
    }
    else
    {
      scanner_info_t *start_info_p = scanner_context_p->end_arguments_p;
      info_p = scanner_insert_info_before (context_p, literal_pool_p->source_p, start_info_p, compressed_size);
    }

    if (no_declarations > PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK)
    {
      no_declarations = PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK;
    }

    uint8_t *data_p = (uint8_t *) (info_p + 1);

    if (is_function)
    {
      info_p->type = SCANNER_TYPE_FUNCTION;

      uint8_t status_flags = 0;

      if (arguments_required)
      {
        status_flags |= SCANNER_FUNCTION_ARGUMENTS_NEEDED;

        if (no_declarations < PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK)
        {
          no_declarations++;
        }

#if ENABLED (JERRY_ES2015)
        const uint16_t is_unmapped = SCANNER_LITERAL_POOL_IS_STRICT | SCANNER_LITERAL_POOL_ARGUMENTS_UNMAPPED;
#else /* !ENABLED (JERRY_ES2015) */
        const uint16_t is_unmapped = SCANNER_LITERAL_POOL_IS_STRICT;
#endif /* ENABLED (JERRY_ES2015) */

        if (literal_pool_p->status_flags & is_unmapped)
        {
          arguments_required = false;
        }
      }

#if ENABLED (JERRY_ES2015)
      if (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_ASYNC)
      {
        status_flags |= SCANNER_FUNCTION_ASYNC;

        if (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION_STATEMENT)
        {
          status_flags |= SCANNER_FUNCTION_STATEMENT;
        }
      }
#endif /* ENABLED (JERRY_ES2015) */

      info_p->u8_arg = status_flags;
      info_p->u16_arg = (uint16_t) no_declarations;
    }
    else
    {
      info_p->type = SCANNER_TYPE_BLOCK;

      JERRY_ASSERT (prev_literal_pool_p != NULL);
    }

    parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
    prev_source_p = literal_pool_p->source_p - 1;
    no_declarations = literal_pool_p->no_declarations;

    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (JERRY_UNLIKELY (no_declarations > PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK)
          || (!(is_function && (literal_p->type & (SCANNER_LITERAL_IS_VAR | SCANNER_LITERAL_IS_ARG)))
              && !(literal_p->type & SCANNER_LITERAL_IS_LOCAL)))
      {
        continue;
      }

      if (literal_p->length == 0)
      {
        *data_p++ = SCANNER_STREAM_TYPE_HOLE;
        continue;
      }

      no_declarations++;

      uint8_t type = SCANNER_STREAM_TYPE_VAR;

      if (literal_p->type & SCANNER_LITERAL_IS_FUNC)
      {
        no_declarations++;
        type = SCANNER_STREAM_TYPE_FUNC;

        if (literal_p->type & SCANNER_LITERAL_IS_ARG)
        {
          type = SCANNER_STREAM_TYPE_ARG_FUNC;

#if ENABLED (JERRY_ES2015)
          if (literal_p->type & SCANNER_LITERAL_IS_DESTRUCTURED_ARG)
          {
            type = SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC;
          }
#endif /* ENABLED (JERRY_ES2015) */
        }
      }
      else if (literal_p->type & SCANNER_LITERAL_IS_ARG)
      {
        type = SCANNER_STREAM_TYPE_ARG;

#if ENABLED (JERRY_ES2015)
        if (literal_p->type & SCANNER_LITERAL_IS_DESTRUCTURED_ARG)
        {
          type = SCANNER_STREAM_TYPE_DESTRUCTURED_ARG;
        }
#endif /* ENABLED (JERRY_ES2015) */
      }
#if ENABLED (JERRY_ES2015)
      else if (literal_p->type & SCANNER_LITERAL_IS_LET)
      {
        if (!(literal_p->type & SCANNER_LITERAL_IS_CONST))
        {
          type = SCANNER_STREAM_TYPE_LET;
        }
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
        else if (prev_literal_pool_p == NULL)
        {
          type = SCANNER_STREAM_TYPE_IMPORT;
        }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
        else
        {
          type = SCANNER_STREAM_TYPE_LOCAL;
        }
      }
      else if (literal_p->type & SCANNER_LITERAL_IS_CONST)
      {
        type = SCANNER_STREAM_TYPE_CONST;
      }
#endif /* ENABLED (JERRY_ES2015) */

      if (literal_p->has_escape)
      {
        type |= SCANNER_STREAM_HAS_ESCAPE;
      }

      if ((literal_p->type & SCANNER_LITERAL_NO_REG)
          || (arguments_required && (literal_p->type & SCANNER_LITERAL_IS_ARG)))
      {
        type |= SCANNER_STREAM_NO_REG;
      }

      data_p[0] = type;
      data_p[1] = (uint8_t) literal_p->length;
      data_p += 3;

      intptr_t diff = (intptr_t) (literal_p->char_p - prev_source_p);

      if (diff >= 1 && diff <= UINT8_MAX)
      {
        data_p[-1] = (uint8_t) diff;
      }
      else if (diff >= -UINT8_MAX && diff <= UINT16_MAX)
      {
        if (diff < 0)
        {
          diff = -diff;
        }

        data_p[-3] |= SCANNER_STREAM_UINT16_DIFF;
        data_p[-1] = (uint8_t) diff;
        data_p[0] = (uint8_t) (diff >> 8);
        data_p += 1;
      }
      else
      {
        data_p[-1] = 0;
        memcpy (data_p, &literal_p->char_p, sizeof (const uint8_t *));
        data_p += sizeof (const uint8_t *);
      }

      prev_source_p = literal_p->char_p + literal_p->length;
    }

    data_p[0] = SCANNER_STREAM_TYPE_END;

    JERRY_ASSERT (((uint8_t *) info_p) + compressed_size == data_p + 1);
  }

  if (!is_function && prev_literal_pool_p->no_declarations < no_declarations)
  {
    prev_literal_pool_p->no_declarations = (uint16_t) no_declarations;
  }

  if (is_function && prev_literal_pool_p != NULL)
  {
    if (prev_literal_pool_p->status_flags & SCANNER_LITERAL_POOL_IS_STRICT)
    {
      context_p->status_flags |= PARSER_IS_STRICT;
    }
    else
    {
      context_p->status_flags &= (uint32_t) ~PARSER_IS_STRICT;
    }

#if ENABLED (JERRY_ES2015)
    if (prev_literal_pool_p->status_flags & SCANNER_LITERAL_POOL_GENERATOR)
    {
      context_p->status_flags |= PARSER_IS_GENERATOR_FUNCTION;
    }
    else
    {
      context_p->status_flags &= (uint32_t) ~PARSER_IS_GENERATOR_FUNCTION;
    }
#endif /* ENABLED (JERRY_ES2015) */
  }

  scanner_context_p->active_literal_pool_p = literal_pool_p->prev_p;

  parser_list_free (&literal_pool_p->literal_pool);
  scanner_free (literal_pool_p, sizeof (scanner_literal_pool_t));
} /* scanner_pop_literal_pool */

/**
 * Filter out the arguments from a literal pool.
 */
void
scanner_filter_arguments (parser_context_t *context_p, /**< context */
                          scanner_context_t *scanner_context_p) /**< scanner context */
{
  /* Fast case: check whether all literals are arguments. */
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;
  scanner_literal_pool_t *prev_literal_pool_p = literal_pool_p->prev_p;
  parser_list_iterator_t literal_iterator;
  lexer_lit_location_t *literal_p;
  bool no_reg = (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_NO_REG) != 0;
  bool has_arguments = (literal_pool_p->status_flags & SCANNER_LITERAL_POOL_NO_ARGUMENTS) == 0;

  if (no_reg && prev_literal_pool_p != NULL)
  {
    prev_literal_pool_p->status_flags |= SCANNER_LITERAL_POOL_NO_REG;
  }

  literal_pool_p->status_flags &= (uint16_t) ~SCANNER_LITERAL_POOL_NO_REG;

  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (no_reg)
    {
      literal_p->type |= SCANNER_LITERAL_NO_REG;
    }

    uint8_t type = literal_p->type;

    if (!(type & SCANNER_LITERAL_IS_ARG) && !(has_arguments && scanner_literal_is_arguments (literal_p)))
    {
      break;
    }

#if ENABLED (JERRY_ES2015)
    if (type & (SCANNER_LITERAL_IS_DESTRUCTURED_ARG | SCANNER_LITERAL_IS_ARROW_DESTRUCTURED_ARG))
    {
      break;
    }
#endif /* ENABLED (JERRY_ES2015) */
  }

  if (literal_p == NULL)
  {
    return;
  }

  scanner_literal_pool_t *new_literal_pool_p;

  new_literal_pool_p = (scanner_literal_pool_t *) scanner_malloc (context_p, sizeof (scanner_literal_pool_t));

  new_literal_pool_p->prev_p = literal_pool_p;
  scanner_context_p->active_literal_pool_p = new_literal_pool_p;

  *new_literal_pool_p = *literal_pool_p;
  parser_list_init (&new_literal_pool_p->literal_pool,
                    sizeof (lexer_lit_location_t),
                    (uint32_t) ((128 - sizeof (void *)) / sizeof (lexer_lit_location_t)));

  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);

#if ENABLED (JERRY_ES2015)
  /* Destructured args are placed after the other arguments because of register assignments. */
  bool has_destructured_arg = false;
#endif /* ENABLED (JERRY_ES2015) */

  while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    uint8_t type = literal_p->type;

    if ((type & SCANNER_LITERAL_IS_ARG) || (has_arguments && scanner_literal_is_arguments (literal_p)))
    {
      if (no_reg)
      {
        literal_p->type |= SCANNER_LITERAL_NO_REG;
      }

#if ENABLED (JERRY_ES2015)
      if (type & (SCANNER_LITERAL_IS_DESTRUCTURED_ARG | SCANNER_LITERAL_IS_ARROW_DESTRUCTURED_ARG))
      {
        has_destructured_arg = true;

        if (type & SCANNER_LITERAL_IS_DESTRUCTURED_ARG)
        {
          continue;
        }

        type &= (uint8_t) ~SCANNER_LITERAL_IS_ARROW_DESTRUCTURED_ARG;
        type |= SCANNER_LITERAL_IS_DESTRUCTURED_ARG;

        if (type & SCANNER_LITERAL_ARROW_DESTRUCTURED_ARG_NO_REG)
        {
          type &= (uint8_t) ~SCANNER_LITERAL_ARROW_DESTRUCTURED_ARG_NO_REG;
          type |= SCANNER_LITERAL_NO_REG;
        }

        literal_p->type = type;
        continue;
      }
#endif /* ENABLED (JERRY_ES2015) */

      lexer_lit_location_t *new_literal_p;
      new_literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &new_literal_pool_p->literal_pool);
      *new_literal_p = *literal_p;
    }
    else if (prev_literal_pool_p != NULL)
    {
      /* Propagate literal to upper level. */
      lexer_lit_location_t *literal_location_p = scanner_add_custom_literal (context_p,
                                                                             prev_literal_pool_p,
                                                                             literal_p);
      type |= SCANNER_LITERAL_NO_REG;

#if ENABLED (JERRY_ES2015)
      type |= SCANNER_LITERAL_IS_USED;
#endif /* ENABLED (JERRY_ES2015) */

      literal_location_p->type |= type;
    }
  }

#if ENABLED (JERRY_ES2015)
  if (has_destructured_arg)
  {
    parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);

    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      const uint8_t expected_flags = SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_IS_DESTRUCTURED_ARG;

      if ((literal_p->type & expected_flags) == expected_flags)
      {
        lexer_lit_location_t *new_literal_p;
        new_literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &new_literal_pool_p->literal_pool);
        *new_literal_p = *literal_p;
      }
    }
  }
#endif /* ENABLED (JERRY_ES2015) */

  new_literal_pool_p->prev_p = prev_literal_pool_p;

  parser_list_free (&literal_pool_p->literal_pool);
  scanner_free (literal_pool_p, sizeof (scanner_literal_pool_t));
} /* scanner_filter_arguments */

/**
 * Add any literal to the specified literal pool.
 *
 * @return pointer to the literal
 */
lexer_lit_location_t *
scanner_add_custom_literal (parser_context_t *context_p, /**< context */
                            scanner_literal_pool_t *literal_pool_p, /**< literal pool */
                            const lexer_lit_location_t *literal_location_p) /**< literal */
{
  parser_list_iterator_t literal_iterator;
  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
  lexer_lit_location_t *literal_p;

  const uint8_t *char_p = literal_location_p->char_p;
  prop_length_t length = literal_location_p->length;

  if (JERRY_LIKELY (!literal_location_p->has_escape))
  {
    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (literal_p->length == length)
      {
        if (JERRY_LIKELY (!literal_p->has_escape))
        {
          if (memcmp (literal_p->char_p, char_p, length) == 0)
          {
            return literal_p;
          }
        }
        else if (lexer_compare_identifier_to_string (literal_p, char_p, length))
        {
          /* The non-escaped version is preferred. */
          literal_p->char_p = char_p;
          literal_p->has_escape = 0;
          return literal_p;
        }
      }
    }
  }
  else
  {
    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (lexer_compare_identifiers (context_p, literal_p, literal_location_p))
      {
        return literal_p;
      }
    }
  }

  literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &literal_pool_p->literal_pool);
  *literal_p = *literal_location_p;

  literal_p->type = 0;

  return literal_p;
} /* scanner_add_custom_literal */

/**
 * Add the current literal token to the current literal pool.
 *
 * @return pointer to the literal
 */
inline lexer_lit_location_t * JERRY_ATTR_ALWAYS_INLINE
scanner_add_literal (parser_context_t *context_p, /**< context */
                     scanner_context_t *scanner_context_p) /**< scanner context */
{
  return scanner_add_custom_literal (context_p,
                                     scanner_context_p->active_literal_pool_p,
                                     &context_p->token.lit_location);
} /* scanner_add_literal */

/**
 * Add the current literal token to the current literal pool and
 * set SCANNER_LITERAL_NO_REG if it is inside a with statement.
 *
 * @return pointer to the literal
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_add_reference (parser_context_t *context_p, /**< context */
                       scanner_context_t *scanner_context_p) /**< scanner context */
{
  lexer_lit_location_t *lit_location_p = scanner_add_custom_literal (context_p,
                                                                     scanner_context_p->active_literal_pool_p,
                                                                     &context_p->token.lit_location);
#if ENABLED (JERRY_ES2015)
  lit_location_p->type |= SCANNER_LITERAL_IS_USED;
#endif /* ENABLED (JERRY_ES2015) */

  if (scanner_context_p->active_literal_pool_p->status_flags & SCANNER_LITERAL_POOL_IN_WITH)
  {
    lit_location_p->type |= SCANNER_LITERAL_NO_REG;
  }

  scanner_detect_eval_call (context_p, scanner_context_p);
} /* scanner_add_reference */

/**
 * Append an argument to the literal pool. If the argument is already present, make it a "hole".
 */
void
scanner_append_argument (parser_context_t *context_p, /**< context */
                         scanner_context_t *scanner_context_p) /**< scanner context */
{
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;
  parser_list_iterator_t literal_iterator;
  parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
  lexer_lit_location_t *literal_location_p = &context_p->token.lit_location;
  lexer_lit_location_t *literal_p;

  const uint8_t *char_p = literal_location_p->char_p;
  prop_length_t length = literal_location_p->length;

  if (JERRY_LIKELY (!context_p->token.lit_location.has_escape))
  {
    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (literal_p->length == length)
      {
        if (JERRY_LIKELY (!literal_p->has_escape))
        {
          if (memcmp (literal_p->char_p, char_p, length) == 0)
          {
            literal_p->length = 0;
            break;
          }
        }
        else if (lexer_compare_identifier_to_string (literal_p, char_p, length))
        {
          literal_p->length = 0;
          break;
        }
      }
    }
  }
  else
  {
    while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
    {
      if (lexer_compare_identifiers (context_p, literal_p, literal_location_p))
      {
        literal_p->length = 0;
        break;
      }
    }
  }

  literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &literal_pool_p->literal_pool);
  *literal_p = context_p->token.lit_location;

  literal_p->type = SCANNER_LITERAL_IS_ARG;
} /* scanner_append_argument */

/**
 * Check whether an eval call is performed and update the status flags accordingly.
 */
void
scanner_detect_eval_call (parser_context_t *context_p, /**< context */
                          scanner_context_t *scanner_context_p) /**< scanner context */
{
  if (context_p->token.keyword_type == LEXER_KEYW_EVAL
      && lexer_check_next_character (context_p, LIT_CHAR_LEFT_PAREN))
  {
    scanner_context_p->active_literal_pool_p->status_flags |= SCANNER_LITERAL_POOL_NO_REG;
  }
} /* scanner_detect_eval_call */

#if ENABLED (JERRY_ES2015)

/**
 * Find a let/const declaration of a given literal.
 *
 * @return true - if the literal is found, false - otherwise
 */
bool
scanner_scope_find_let_declaration (parser_context_t *context_p, /**< context */
                                    lexer_lit_location_t *literal_p) /**< literal */
{
  ecma_string_t *name_p;

  if (JERRY_LIKELY (!literal_p->has_escape))
  {
    name_p = ecma_new_ecma_string_from_utf8 (literal_p->char_p, literal_p->length);
  }
  else
  {
    uint8_t *destination_p = (uint8_t *) scanner_malloc (context_p, literal_p->length);

    lexer_convert_ident_to_cesu8 (destination_p, literal_p->char_p, literal_p->length);

    name_p = ecma_new_ecma_string_from_utf8 (destination_p, literal_p->length);
    scanner_free (destination_p, literal_p->length);
  }

  ecma_object_t *lex_env_p = JERRY_CONTEXT (vm_top_context_p)->lex_env_p;

  while (lex_env_p->type_flags_refs & ECMA_OBJECT_FLAG_BLOCK)
  {
    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

      if (property_p != NULL && ecma_is_property_enumerable (*property_p))
      {
        ecma_deref_ecma_string (name_p);
        return true;
      }
    }

    JERRY_ASSERT (lex_env_p->u2.outer_reference_cp != JMEM_CP_NULL);
    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

    if (property_p != NULL && ecma_is_property_enumerable (*property_p))
    {
      ecma_deref_ecma_string (name_p);
      return true;
    }
  }

  ecma_deref_ecma_string (name_p);
  return false;
} /* scanner_scope_find_let_declaration */

/**
 * Throws an error for invalid var statements.
 */
void
scanner_detect_invalid_var (parser_context_t *context_p, /**< context */
                            scanner_context_t *scanner_context_p, /**< scanner context */
                            lexer_lit_location_t *var_literal_p) /**< var literal */
{
  if (var_literal_p->type & SCANNER_LITERAL_IS_LOCAL
      && !(var_literal_p->type & (SCANNER_LITERAL_IS_FUNC | SCANNER_LITERAL_IS_ARG))
      && (var_literal_p->type & SCANNER_LITERAL_IS_LOCAL) != SCANNER_LITERAL_IS_LOCAL)
  {
    scanner_raise_redeclaration_error (context_p);
  }

  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;
  const uint8_t *char_p = var_literal_p->char_p;
  prop_length_t length = var_literal_p->length;

  while (!(literal_pool_p->status_flags & SCANNER_LITERAL_POOL_FUNCTION))
  {
    literal_pool_p = literal_pool_p->prev_p;

    parser_list_iterator_t literal_iterator;
    parser_list_iterator_init (&literal_pool_p->literal_pool, &literal_iterator);
    lexer_lit_location_t *literal_p;

    if (JERRY_LIKELY (!context_p->token.lit_location.has_escape))
    {
      while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
      {
        if (literal_p->type & SCANNER_LITERAL_IS_LOCAL
            && !(literal_p->type & SCANNER_LITERAL_IS_ARG)
            && (literal_p->type & SCANNER_LITERAL_IS_LOCAL) != SCANNER_LITERAL_IS_LOCAL
            && literal_p->length == length)
        {
          if (JERRY_LIKELY (!literal_p->has_escape))
          {
            if (memcmp (literal_p->char_p, char_p, length) == 0)
            {
              scanner_raise_redeclaration_error (context_p);
              return;
            }
          }
          else if (lexer_compare_identifier_to_string (literal_p, char_p, length))
          {
            scanner_raise_redeclaration_error (context_p);
            return;
          }
        }
      }
    }
    else
    {
      while ((literal_p = (lexer_lit_location_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
      {
        if (literal_p->type & SCANNER_LITERAL_IS_LOCAL
            && !(literal_p->type & SCANNER_LITERAL_IS_ARG)
            && (literal_p->type & SCANNER_LITERAL_IS_LOCAL) != SCANNER_LITERAL_IS_LOCAL
            && lexer_compare_identifiers (context_p, literal_p, var_literal_p))
        {
          scanner_raise_redeclaration_error (context_p);
          return;
        }
      }
    }
  }

  if ((context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL)
      && scanner_scope_find_let_declaration (context_p, var_literal_p))
  {
    scanner_raise_redeclaration_error (context_p);
  }
} /* scanner_detect_invalid_var */

/**
 * Throws an error for invalid let statements.
 */
void
scanner_detect_invalid_let (parser_context_t *context_p, /**< context */
                            lexer_lit_location_t *let_literal_p) /**< let literal */
{
  if (let_literal_p->type & (SCANNER_LITERAL_IS_ARG
                             | SCANNER_LITERAL_IS_VAR
                             | SCANNER_LITERAL_IS_LOCAL))
  {
    scanner_raise_redeclaration_error (context_p);
  }

  if (let_literal_p->type & SCANNER_LITERAL_IS_FUNC)
  {
    let_literal_p->type &= (uint8_t) ~SCANNER_LITERAL_IS_FUNC;
  }
} /* scanner_detect_invalid_let */

/**
 * Push the values required for destructuring assignment or binding parsing.
 */
void
scanner_push_destructuring_pattern (parser_context_t *context_p, /**< context */
                                    scanner_context_t *scanner_context_p, /**< scanner context */
                                    uint8_t binding_type, /**< type of destructuring binding pattern */
                                    bool is_nested) /**< nested declaration */
{
  JERRY_ASSERT (binding_type != SCANNER_BINDING_NONE || !is_nested);

  scanner_source_start_t source_start;
  source_start.source_p = context_p->source_p;

  parser_stack_push (context_p, &source_start, sizeof (scanner_source_start_t));
  parser_stack_push_uint8 (context_p, scanner_context_p->binding_type);
  scanner_context_p->binding_type = binding_type;

  if (SCANNER_NEEDS_BINDING_LIST (binding_type))
  {
    scanner_binding_list_t *binding_list_p;
    binding_list_p = (scanner_binding_list_t *) scanner_malloc (context_p, sizeof (scanner_binding_list_t));

    binding_list_p->prev_p = scanner_context_p->active_binding_list_p;
    binding_list_p->items_p = NULL;
    binding_list_p->is_nested = is_nested;

    scanner_context_p->active_binding_list_p = binding_list_p;
  }
} /* scanner_push_destructuring_pattern */

/**
 * Pop binding list.
 */
void
scanner_pop_binding_list (scanner_context_t *scanner_context_p) /**< scanner context */
{
  scanner_binding_list_t *binding_list_p = scanner_context_p->active_binding_list_p;
  scanner_binding_item_t *item_p = binding_list_p->items_p;
  scanner_binding_list_t *prev_binding_list_p = binding_list_p->prev_p;
  bool is_nested = binding_list_p->is_nested;

  scanner_free (binding_list_p, sizeof (scanner_binding_list_t));
  scanner_context_p->active_binding_list_p = prev_binding_list_p;

  JERRY_ASSERT (binding_list_p != NULL);

  if (!is_nested)
  {
    while (item_p != NULL)
    {
      scanner_binding_item_t *next_p = item_p->next_p;

      JERRY_ASSERT (item_p->literal_p->type & (SCANNER_LITERAL_IS_LOCAL | SCANNER_LITERAL_IS_ARG));

      scanner_free (item_p, sizeof (scanner_binding_item_t));
      item_p = next_p;
    }
    return;
  }

  JERRY_ASSERT (prev_binding_list_p != NULL);

  while (item_p != NULL)
  {
    scanner_binding_item_t *next_p = item_p->next_p;

    item_p->next_p = prev_binding_list_p->items_p;
    prev_binding_list_p->items_p = item_p;

    item_p = next_p;
  }
} /* scanner_pop_binding_list */

/**
 * Append a hole into the literal pool.
 */
void
scanner_append_hole (parser_context_t *context_p, scanner_context_t *scanner_context_p)
{
  scanner_literal_pool_t *literal_pool_p = scanner_context_p->active_literal_pool_p;

  lexer_lit_location_t *literal_p;
  literal_p = (lexer_lit_location_t *) parser_list_append (context_p, &literal_pool_p->literal_pool);

  literal_p->char_p = NULL;
  literal_p->length = 0;
  literal_p->type = SCANNER_LITERAL_IS_ARG;
  literal_p->has_escape = 0;
} /* scanner_append_hole */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * Reverse the scanner info chain after the scanning is completed.
 */
void
scanner_reverse_info_list (parser_context_t *context_p) /**< context */
{
  scanner_info_t *scanner_info_p = context_p->next_scanner_info_p;
  scanner_info_t *last_scanner_info_p = NULL;

  if (scanner_info_p->type == SCANNER_TYPE_END)
  {
    return;
  }

  do
  {
    scanner_info_t *next_scanner_info_p = scanner_info_p->next_p;
    scanner_info_p->next_p = last_scanner_info_p;

    last_scanner_info_p = scanner_info_p;
    scanner_info_p = next_scanner_info_p;
  }
  while (scanner_info_p->type != SCANNER_TYPE_END);

  context_p->next_scanner_info_p->next_p = scanner_info_p;
  context_p->next_scanner_info_p = last_scanner_info_p;
} /* scanner_reverse_info_list */

/**
 * Release unused scanner info blocks.
 * This should happen only if an error is occured.
 */
void
scanner_cleanup (parser_context_t *context_p) /**< context */
{
  if (context_p->skipped_scanner_info_p != NULL)
  {
    context_p->skipped_scanner_info_end_p->next_p = context_p->next_scanner_info_p;
    context_p->next_scanner_info_p = context_p->skipped_scanner_info_p;
    context_p->skipped_scanner_info_p = NULL;
  }

  scanner_info_t *scanner_info_p = context_p->next_scanner_info_p;

  while (scanner_info_p != NULL)
  {
    scanner_info_t *next_scanner_info_p = scanner_info_p->next_p;

    size_t size = sizeof (scanner_info_t);

    switch (scanner_info_p->type)
    {
      case SCANNER_TYPE_END:
      {
        scanner_info_p = context_p->active_scanner_info_p;
        continue;
      }
      case SCANNER_TYPE_FUNCTION:
      case SCANNER_TYPE_BLOCK:
      {
        size = scanner_get_stream_size (scanner_info_p, sizeof (scanner_info_t));
        break;
      }
      case SCANNER_TYPE_WHILE:
      case SCANNER_TYPE_FOR_IN:
#if ENABLED (JERRY_ES2015)
      case SCANNER_TYPE_FOR_OF:
#endif /* ENABLED (JERRY_ES2015) */
      case SCANNER_TYPE_CASE:
#if ENABLED (JERRY_ES2015)
      case SCANNER_TYPE_INITIALIZER:
#endif /* ENABLED (JERRY_ES2015) */
      {
        size = sizeof (scanner_location_info_t);
        break;
      }
      case SCANNER_TYPE_FOR:
      {
        size = sizeof (scanner_for_info_t);
        break;
      }
      case SCANNER_TYPE_SWITCH:
      {
        scanner_release_switch_cases (((scanner_switch_info_t *) scanner_info_p)->case_p);
        size = sizeof (scanner_switch_info_t);
        break;
      }
      default:
      {
#if ENABLED (JERRY_ES2015)
        JERRY_ASSERT (scanner_info_p->type == SCANNER_TYPE_END_ARGUMENTS
                      || scanner_info_p->type == SCANNER_TYPE_LET_EXPRESSION
                      || scanner_info_p->type == SCANNER_TYPE_ERR_REDECLARED);
#else /* !ENABLED (JERRY_ES2015) */
        JERRY_ASSERT (scanner_info_p->type == SCANNER_TYPE_END_ARGUMENTS);
#endif /* ENABLED (JERRY_ES2015) */
        break;
      }
    }

    scanner_free (scanner_info_p, size);
    scanner_info_p = next_scanner_info_p;
  }

  context_p->next_scanner_info_p = NULL;
  context_p->active_scanner_info_p = NULL;
} /* scanner_cleanup */

/**
 * Checks whether a context needs to be created for a block.
 *
 * @return true - if context is needed,
 *         false - otherwise
 */
bool
scanner_is_context_needed (parser_context_t *context_p) /**< context */
{
  scanner_info_t *info_p = context_p->next_scanner_info_p;
  const uint8_t *data_p = ((const uint8_t *) info_p) + sizeof (scanner_info_t);

  JERRY_ASSERT (info_p->type == SCANNER_TYPE_BLOCK);

  uint32_t scope_stack_reg_top = context_p->scope_stack_reg_top;

  while (data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    uint32_t type = data_p[0] & SCANNER_STREAM_TYPE_MASK;

#if ENABLED (JERRY_ES2015)
    JERRY_ASSERT (type == SCANNER_STREAM_TYPE_VAR
                  || type == SCANNER_STREAM_TYPE_LET
                  || type == SCANNER_STREAM_TYPE_CONST
                  || type == SCANNER_STREAM_TYPE_LOCAL
                  || type == SCANNER_STREAM_TYPE_FUNC);
#else /* !ENABLED (JERRY_ES2015) */
    JERRY_ASSERT (type == SCANNER_STREAM_TYPE_VAR);
#endif /* ENABLED (JERRY_ES2015) */

    size_t length;

    if (!(data_p[0] & SCANNER_STREAM_UINT16_DIFF))
    {
      if (data_p[2] != 0)
      {
        length = 2 + 1;
      }
      else
      {
        length = 2 + 1 + sizeof (const uint8_t *);
      }
    }
    else
    {
      length = 2 + 2;
    }

    if (!(data_p[0] & SCANNER_STREAM_NO_REG)
        && scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
    {
      scope_stack_reg_top++;
    }
    else
    {
      return true;
    }

    data_p += length;
  }

  return false;
} /* scanner_is_context_needed */

#if ENABLED (JERRY_ES2015)

/**
 * Checks whether a global context needs to be created for a script.
 *
 * @return true - if context is needed,
 *         false - otherwise
 */
bool
scanner_is_global_context_needed (parser_context_t *context_p) /**< context */
{
  scanner_info_t *info_p = context_p->next_scanner_info_p;
  const uint8_t *data_p = (const uint8_t *) (info_p + 1);
  uint32_t scope_stack_reg_top = 0;

  JERRY_ASSERT (info_p->type == SCANNER_TYPE_FUNCTION);

  while (data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    uint8_t data = data_p[0];
    uint32_t type = data & SCANNER_STREAM_TYPE_MASK;

    /* FIXME: a private declarative lexical environment should always be present
     * for modules. Remove SCANNER_STREAM_TYPE_IMPORT after it is implemented. */
    JERRY_ASSERT (type == SCANNER_STREAM_TYPE_VAR
                  || type == SCANNER_STREAM_TYPE_LET
                  || type == SCANNER_STREAM_TYPE_CONST
                  || type == SCANNER_STREAM_TYPE_FUNC
                  || type == SCANNER_STREAM_TYPE_IMPORT);

    /* Only let/const can be stored in registers */
    JERRY_ASSERT ((data & SCANNER_STREAM_NO_REG)
                  || (type == SCANNER_STREAM_TYPE_FUNC && (context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL))
                  || type == SCANNER_STREAM_TYPE_LET
                  || type == SCANNER_STREAM_TYPE_CONST);

    if (!(data & SCANNER_STREAM_UINT16_DIFF))
    {
      if (data_p[2] != 0)
      {
        data_p += 2 + 1;
      }
      else
      {
        data_p += 2 + 1 + sizeof (const uint8_t *);
      }
    }
    else
    {
      data_p += 2 + 2;
    }

    if (type == SCANNER_STREAM_TYPE_VAR
        || (type == SCANNER_STREAM_TYPE_FUNC && !(context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL))
        || type == SCANNER_STREAM_TYPE_IMPORT)
    {
      continue;
    }

    if (!(data & SCANNER_STREAM_NO_REG)
        && scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
    {
      scope_stack_reg_top++;
    }
    else
    {
      return true;
    }
  }

  return false;
} /* scanner_is_global_context_needed */

/**
 * Try to scan/parse the ".target" part in the "new.target" expression.
 *
 * Upon exiting with "true" the current token will point to the "target"
 * literal.
 *
 * If the "target" literal is not after the "new." then a scanner/parser
 * error will be raised.
 *
 * @returns true if the ".target" part was found
 *          false if there is no "." after the new.
 */
bool
scanner_try_scan_new_target (parser_context_t *context_p) /**< parser/scanner context */
{
  JERRY_ASSERT (context_p->token.type == LEXER_KEYW_NEW);

  if (lexer_check_next_character (context_p, LIT_CHAR_DOT))
  {
    lexer_next_token (context_p);
    if (context_p->token.type != LEXER_DOT)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_CHARACTER);
    }

    lexer_next_token (context_p);
    if (!lexer_token_is_identifier (context_p, "target", 6))
    {
      parser_raise_error (context_p, PARSER_ERR_NEW_TARGET_EXPECTED);
    }

    return true;
  }
  return false;
} /* scanner_try_scan_new_target */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * Description of "arguments" literal string.
 */
const lexer_lit_location_t lexer_arguments_literal =
{
  (const uint8_t *) "arguments", 9, LEXER_IDENT_LITERAL, false
};

/**
 * Create an unused literal.
 */
static void
scanner_create_unused_literal (parser_context_t *context_p, /**< context */
                               uint8_t status_flags) /**< initial status flags */
{
  if (JERRY_UNLIKELY (context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS))
  {
    parser_raise_error (context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  lexer_literal_t *literal_p = (lexer_literal_t *) parser_list_append (context_p, &context_p->literal_pool);

  literal_p->type = LEXER_UNUSED_LITERAL;
  literal_p->status_flags = status_flags;

  context_p->literal_count++;
} /* scanner_create_unused_literal */

#if ENABLED (JERRY_ES2015)
/**
 * Emit checks for redeclared bindings in the global lexical scope.
 */
void
scanner_check_variables (parser_context_t *context_p) /**< context */
{
  scanner_info_t *info_p = context_p->next_scanner_info_p;
  const uint8_t *next_data_p = (const uint8_t *) (info_p + 1);
  lexer_lit_location_t literal;

  JERRY_ASSERT (info_p->type == SCANNER_TYPE_FUNCTION);

  literal.char_p = info_p->source_p - 1;

  while (next_data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    uint32_t type = next_data_p[0] & SCANNER_STREAM_TYPE_MASK;
    const uint8_t *data_p = next_data_p;

    JERRY_ASSERT (type != SCANNER_STREAM_TYPE_HOLE
                  && !SCANNER_STREAM_TYPE_IS_ARG (type)
                  && !SCANNER_STREAM_TYPE_IS_ARG_FUNC (type));
    JERRY_ASSERT (data_p[0] & SCANNER_STREAM_NO_REG);

    if (!(data_p[0] & SCANNER_STREAM_UINT16_DIFF))
    {
      if (data_p[2] != 0)
      {
        literal.char_p += data_p[2];
        next_data_p += 2 + 1;
      }
      else
      {
        memcpy (&literal.char_p, data_p + 2 + 1, sizeof (const uint8_t *));
        next_data_p += 2 + 1 + sizeof (const uint8_t *);
      }
    }
    else
    {
      int32_t diff = ((int32_t) data_p[2]) | ((int32_t) data_p[3]) << 8;

      if (diff <= UINT8_MAX)
      {
        diff = -diff;
      }

      literal.char_p += diff;
      next_data_p += 2 + 2;
    }

    literal.length = data_p[1];
    literal.type = LEXER_IDENT_LITERAL;
    literal.has_escape = (data_p[0] & SCANNER_STREAM_HAS_ESCAPE) ? 1 : 0;

    lexer_construct_literal_object (context_p, &literal, LEXER_NEW_IDENT_LITERAL);
    literal.char_p += data_p[1];

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
    if (type == SCANNER_STREAM_TYPE_IMPORT)
    {
      continue;
    }
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

    context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_USED;

    uint16_t opcode;
    if (type == SCANNER_STREAM_TYPE_VAR || type == SCANNER_STREAM_TYPE_FUNC)
    {
      opcode = CBC_CHECK_VAR;
    }
    else
    {
      opcode = CBC_CHECK_LET;
    }

    parser_emit_cbc_literal (context_p, opcode, context_p->lit_object.index);
  }

  parser_flush_cbc (context_p);
} /* scanner_check_variables */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Create and/or initialize var/let/const/function/etc. variables.
 */
void
scanner_create_variables (parser_context_t *context_p, /**< context */
                          uint32_t option_flags) /**< combination of scanner_create_variables_flags_t bits */
{
  scanner_info_t *info_p = context_p->next_scanner_info_p;
  const uint8_t *next_data_p = (const uint8_t *) (info_p + 1);
  uint8_t info_type = info_p->type;
  lexer_lit_location_t literal;
  parser_scope_stack_t *scope_stack_p;
  parser_scope_stack_t *scope_stack_end_p;

  JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION || info_type == SCANNER_TYPE_BLOCK);
  JERRY_ASSERT (!(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS)
                || !(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY));
  JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION
                || !(option_flags & (SCANNER_CREATE_VARS_IS_FUNCTION_ARGS | SCANNER_CREATE_VARS_IS_FUNCTION_BODY)));

  if (info_type == SCANNER_TYPE_FUNCTION && !(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY))
  {
    JERRY_ASSERT (context_p->scope_stack_p == NULL);

    size_t stack_size = info_p->u16_arg * sizeof (parser_scope_stack_t);
    context_p->scope_stack_size = info_p->u16_arg;

    scope_stack_p = NULL;

    if (stack_size > 0)
    {
      scope_stack_p = (parser_scope_stack_t *) parser_malloc (context_p, stack_size);
    }

    context_p->scope_stack_p = scope_stack_p;
    scope_stack_end_p = scope_stack_p + context_p->scope_stack_size;
  }
  else
  {
    JERRY_ASSERT (context_p->scope_stack_p != NULL || context_p->scope_stack_size == 0);

    scope_stack_p = context_p->scope_stack_p;
    scope_stack_end_p = scope_stack_p + context_p->scope_stack_size;
    scope_stack_p += context_p->scope_stack_top;
  }

  uint32_t scope_stack_reg_top = context_p->scope_stack_reg_top;

  literal.char_p = info_p->source_p - 1;

  while (next_data_p[0] != SCANNER_STREAM_TYPE_END)
  {
    uint32_t type = next_data_p[0] & SCANNER_STREAM_TYPE_MASK;
    const uint8_t *data_p = next_data_p;

    JERRY_ASSERT ((option_flags & (SCANNER_CREATE_VARS_IS_FUNCTION_BODY | SCANNER_CREATE_VARS_IS_FUNCTION_ARGS))
                  || (type != SCANNER_STREAM_TYPE_HOLE
                      && !SCANNER_STREAM_TYPE_IS_ARG (type)
                      && !SCANNER_STREAM_TYPE_IS_ARG_FUNC (type)));

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
    JERRY_ASSERT (type != SCANNER_STREAM_TYPE_IMPORT || (data_p[0] & SCANNER_STREAM_NO_REG));
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

    if (type == SCANNER_STREAM_TYPE_HOLE)
    {
      next_data_p++;

      if (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY)
      {
        continue;
      }

      if (info_p->u8_arg & SCANNER_FUNCTION_ARGUMENTS_NEEDED)
      {
        scanner_create_unused_literal (context_p, LEXER_FLAG_FUNCTION_ARGUMENT);
      }

      if (scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
      {
        scope_stack_reg_top++;
      }
      continue;
    }

    JERRY_ASSERT (context_p->scope_stack_size != 0);

    if (!(data_p[0] & SCANNER_STREAM_UINT16_DIFF))
    {
      if (data_p[2] != 0)
      {
        literal.char_p += data_p[2];
        next_data_p += 2 + 1;
      }
      else
      {
        memcpy (&literal.char_p, data_p + 2 + 1, sizeof (const uint8_t *));
        next_data_p += 2 + 1 + sizeof (const uint8_t *);
      }
    }
    else
    {
      int32_t diff = ((int32_t) data_p[2]) | ((int32_t) data_p[3]) << 8;

      if (diff <= UINT8_MAX)
      {
        diff = -diff;
      }

      literal.char_p += diff;
      next_data_p += 2 + 2;
    }

    if (SCANNER_STREAM_TYPE_IS_ARG (type))
    {
      if (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY)
      {
        literal.char_p += data_p[1];
        continue;
      }
    }
    else if ((option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS)
             && !SCANNER_STREAM_TYPE_IS_ARG_FUNC (type))
    {
      /* Function arguments must come first. */
      break;
    }

    literal.length = data_p[1];
    literal.type = LEXER_IDENT_LITERAL;
    literal.has_escape = (data_p[0] & SCANNER_STREAM_HAS_ESCAPE) ? 1 : 0;

    lexer_construct_literal_object (context_p, &literal, LEXER_NEW_IDENT_LITERAL);
    literal.char_p += data_p[1];

    if (SCANNER_STREAM_TYPE_IS_ARG_FUNC (type) && (option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY))
    {
      JERRY_ASSERT (scope_stack_p >= context_p->scope_stack_p + 2);

      parser_scope_stack_t *function_map_p = scope_stack_p - 2;
      uint16_t literal_index = context_p->lit_object.index;

      while (literal_index != function_map_p->map_from)
      {
        function_map_p--;

        JERRY_ASSERT (function_map_p >= context_p->scope_stack_p);
      }

      JERRY_ASSERT (function_map_p[1].map_from == PARSER_SCOPE_STACK_FUNC);

      parser_emit_cbc_literal_value (context_p,
                                     CBC_SET_VAR_FUNC,
                                     function_map_p[1].map_to,
                                     scanner_decode_map_to (function_map_p));
      continue;
    }

    if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
    {
      JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
      parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
    }

    scope_stack_p->map_from = context_p->lit_object.index;

#if ENABLED (JERRY_ES2015)
    if (info_type == SCANNER_TYPE_FUNCTION)
    {
      if (type != SCANNER_STREAM_TYPE_LET
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
          && type != SCANNER_STREAM_TYPE_IMPORT
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
          && type != SCANNER_STREAM_TYPE_CONST)
      {
        context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_GLOBAL;
      }
    }
#endif /* ENABLED (JERRY_ES2015) */

    uint16_t map_to;
    uint16_t func_init_opcode = CBC_INIT_LOCAL;

    if (!(data_p[0] & SCANNER_STREAM_NO_REG)
        && scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
    {
      map_to = (uint16_t) (PARSER_REGISTER_START + scope_stack_reg_top);

#if ENABLED (JERRY_ES2015)
      scope_stack_p->map_to = (uint16_t) (scope_stack_reg_top + 1);
#else /* !ENABLED (JERRY_ES2015) */
      scope_stack_p->map_to = map_to;
#endif /* ENABLED (JERRY_ES2015) */

      scope_stack_reg_top++;
#if ENABLED (JERRY_ES2015)
      switch (type)
      {
        case SCANNER_STREAM_TYPE_CONST:
        {
          scope_stack_p->map_to |= PARSER_SCOPE_STACK_IS_CONST;
          /* FALLTHRU */
        }
        case SCANNER_STREAM_TYPE_LET:
        {
          scope_stack_p->map_to |= PARSER_SCOPE_STACK_IS_LEXICAL;
          break;
        }
      }

      func_init_opcode = CBC_SET_VAR_FUNC;
#endif /* ENABLED (JERRY_ES2015) */
    }
    else
    {
      context_p->lit_object.literal_p->status_flags |= LEXER_FLAG_USED;
      map_to = context_p->lit_object.index;

#if ENABLED (JERRY_ES2015)
      scope_stack_p->map_to = 0;
#else /* !ENABLED (JERRY_ES2015) */
      scope_stack_p->map_to = map_to;
#endif /* ENABLED (JERRY_ES2015) */

      if (info_type == SCANNER_TYPE_FUNCTION)
      {
        context_p->status_flags |= PARSER_LEXICAL_ENV_NEEDED;
      }

      switch (type)
      {
#if ENABLED (JERRY_ES2015)
        case SCANNER_STREAM_TYPE_CONST:
        {
          scope_stack_p->map_to |= PARSER_SCOPE_STACK_IS_CONST;
          /* FALLTHRU */
        }
        case SCANNER_STREAM_TYPE_LET:
        {
          scope_stack_p->map_to |= PARSER_SCOPE_STACK_IS_LEXICAL;
          /* FALLTHRU */
        }
        case SCANNER_STREAM_TYPE_LOCAL:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG:
        case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC:
#endif /* ENABLED (JERRY_ES2015) */
        case SCANNER_STREAM_TYPE_VAR:
        {
#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
          context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

          uint16_t opcode = ((option_flags & SCANNER_CREATE_VARS_IS_SCRIPT) ? CBC_CREATE_VAR_EVAL
                                                                            : CBC_CREATE_VAR);
#if ENABLED (JERRY_ES2015)
          switch (type)
          {
            case SCANNER_STREAM_TYPE_LET:
            {
              opcode = CBC_CREATE_LET;
              break;
            }
            case SCANNER_STREAM_TYPE_CONST:
            {
              opcode = CBC_CREATE_CONST;
              break;
            }
            case SCANNER_STREAM_TYPE_LOCAL:
            case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG:
            case SCANNER_STREAM_TYPE_DESTRUCTURED_ARG_FUNC:
            {
              opcode = CBC_CREATE_LOCAL;
              break;
            }
          }
#endif /* ENABLED (JERRY_ES2015) */

          parser_emit_cbc_literal (context_p, opcode, map_to);
          break;
        }
        case SCANNER_STREAM_TYPE_ARG:
        case SCANNER_STREAM_TYPE_ARG_FUNC:
        {
#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
          context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

          parser_emit_cbc_literal_value (context_p,
                                         CBC_INIT_LOCAL,
                                         (uint16_t) (PARSER_REGISTER_START + scope_stack_reg_top),
                                         map_to);

          if (scope_stack_reg_top < PARSER_MAXIMUM_NUMBER_OF_REGISTERS)
          {
            scope_stack_reg_top++;
          }
          break;
        }
      }
    }

    scope_stack_p++;

    if (!SCANNER_STREAM_TYPE_IS_FUNCTION (type))
    {
      continue;
    }

    if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
    {
      JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
      parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
    }

#if ENABLED (JERRY_PARSER_DUMP_BYTE_CODE)
    context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
#endif /* ENABLED (JERRY_PARSER_DUMP_BYTE_CODE) */

    if (!SCANNER_STREAM_TYPE_IS_ARG_FUNC (type))
    {
      if (func_init_opcode == CBC_INIT_LOCAL && (option_flags & SCANNER_CREATE_VARS_IS_SCRIPT))
      {
#if ENABLED (JERRY_ES2015)
        if (!(context_p->global_status_flags & ECMA_PARSE_DIRECT_EVAL))
        {
          func_init_opcode = CBC_CREATE_VAR_FUNC_EVAL;
        }
#else /* !ENABLED (JERRY_ES2015) */
        func_init_opcode = CBC_CREATE_VAR_FUNC_EVAL;
#endif /* ENABLED (JERRY_ES2015) */
      }

      parser_emit_cbc_literal_value (context_p, func_init_opcode, context_p->literal_count, map_to);
    }

    scope_stack_p->map_from = PARSER_SCOPE_STACK_FUNC;
    scope_stack_p->map_to = context_p->literal_count;
    scope_stack_p++;

    scanner_create_unused_literal (context_p, 0);
  }

  if (info_type == SCANNER_TYPE_FUNCTION
      && !(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_BODY)
      && (info_p->u8_arg & SCANNER_FUNCTION_ARGUMENTS_NEEDED))
  {
    JERRY_ASSERT (info_type == SCANNER_TYPE_FUNCTION);

    if (JERRY_UNLIKELY (scope_stack_p >= scope_stack_end_p))
    {
      JERRY_ASSERT (context_p->scope_stack_size == PARSER_MAXIMUM_DEPTH_OF_SCOPE_STACK);
      parser_raise_error (context_p, PARSER_ERR_SCOPE_STACK_LIMIT_REACHED);
    }

    context_p->status_flags |= PARSER_ARGUMENTS_NEEDED | PARSER_LEXICAL_ENV_NEEDED;

    lexer_construct_literal_object (context_p, &lexer_arguments_literal, lexer_arguments_literal.type);

    scope_stack_p->map_from = context_p->lit_object.index;
#if ENABLED (JERRY_ES2015)
    scope_stack_p->map_to = 0;
#else /* !ENABLED (JERRY_ES2015) */
    scope_stack_p->map_to = context_p->lit_object.index;
#endif /* ENABLED (JERRY_ES2015) */
    scope_stack_p++;
  }

  context_p->scope_stack_top = (uint16_t) (scope_stack_p - context_p->scope_stack_p);
  context_p->scope_stack_reg_top = (uint16_t) scope_stack_reg_top;

#if ENABLED (JERRY_ES2015)
  if (info_type == SCANNER_TYPE_FUNCTION)
  {
    context_p->scope_stack_global_end = context_p->scope_stack_top;
  }
#endif /* ENABLED (JERRY_ES2015) */

  if (context_p->register_count < scope_stack_reg_top)
  {
    context_p->register_count = (uint16_t) scope_stack_reg_top;
  }

  if (!(option_flags & SCANNER_CREATE_VARS_IS_FUNCTION_ARGS))
  {
    scanner_release_next (context_p, (size_t) (next_data_p + 1 - ((const uint8_t *) info_p)));
  }
  parser_flush_cbc (context_p);
} /* scanner_create_variables */

/**
 * Get location from context.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_get_location (scanner_location_t *location_p, /**< location */
                      parser_context_t *context_p) /**< context */
{
  location_p->source_p = context_p->source_p;
  location_p->line = context_p->line;
  location_p->column = context_p->column;
} /* scanner_get_location */

/**
 * Set context location.
 */
inline void JERRY_ATTR_ALWAYS_INLINE
scanner_set_location (parser_context_t *context_p, /**< context */
                      scanner_location_t *location_p) /**< location */
{
  context_p->source_p = location_p->source_p;
  context_p->line = location_p->line;
  context_p->column = location_p->column;
} /* scanner_set_location */

/**
 * Get the real map_to value.
 */
inline uint16_t JERRY_ATTR_ALWAYS_INLINE
scanner_decode_map_to (parser_scope_stack_t *stack_item_p) /**< scope stack item */
{
  JERRY_ASSERT (stack_item_p->map_from != PARSER_SCOPE_STACK_FUNC);

#if ENABLED (JERRY_ES2015)
  uint16_t value = (stack_item_p->map_to & PARSER_SCOPE_STACK_REGISTER_MASK);
  return (value == 0) ? stack_item_p->map_from : (uint16_t) (value + (PARSER_REGISTER_START - 1));
#else /* !ENABLED (JERRY_ES2015) */
  return stack_item_p->map_to;
#endif /* ENABLED (JERRY_ES2015) */
} /* scanner_decode_map_to */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_PARSER) */
