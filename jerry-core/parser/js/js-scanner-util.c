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

#include "js-parser-internal.h"
#include "js-scanner-internal.h"

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
      case SCANNER_TYPE_WHILE:
      case SCANNER_TYPE_FOR_IN:
#if ENABLED (JERRY_ES2015_FOR_OF)
      case SCANNER_TYPE_FOR_OF:
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */
      case SCANNER_TYPE_CASE:
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
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
        JERRY_ASSERT (scanner_info_p->type == SCANNER_TYPE_END_ARGUMENTS
                      || scanner_info_p->type == SCANNER_TYPE_ARROW);
#else /* !ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
        JERRY_ASSERT (scanner_info_p->type == SCANNER_TYPE_END_ARGUMENTS);
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
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
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_PARSER) */
