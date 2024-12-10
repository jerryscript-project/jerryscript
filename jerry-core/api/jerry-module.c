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

#include "jerryscript-core.h"
#include "jerryscript-port.h"

#include "ecma-errors.h"

#include "lit-magic-strings.h"
#include "lit-strings.h"

#if JERRY_MODULE_SYSTEM

/**
 * A module descriptor.
 */
typedef struct jerry_module_t
{
  struct jerry_module_t *next_p; /**< next_module */
  jerry_char_t *path_p; /**< path to the module */
  jerry_size_t path_size; /**< size of path to the module, excluding '\0' terminator  */
  jerry_size_t basename_offset; /**< offset of the basename in the module path*/
  jerry_value_t realm; /**< the realm of the module */
  jerry_value_t module; /**< the module itself */
} jerry_module_t;

/**
 * Native info descriptor for modules.
 */
static const jerry_object_native_info_t jerry_module_native_info JERRY_ATTR_CONST_DATA = {
  .free_cb = NULL,
};

/**
 * Default module manager.
 */
typedef struct
{
  jerry_module_t *module_head_p; /**< first module */
} jerry_module_manager_t;

typedef enum
{
  JERRY_PATH_JOIN_ITERATOR_STATE_ONE_DOT,
  JERRY_PATH_JOIN_ITERATOR_STATE_TWO_DOT,
  JERRY_PATH_JOIN_ITERATOR_STATE_SEPARATOR,
  JERRY_PATH_JOIN_ITERATOR_STATE_SEGMENT,
  JERRY_PATH_JOIN_ITERATOR_STATE_ROOT,
} jerry_path_iterator_state_t;

typedef struct
{
  jerry_path_style_t style;
  jerry_char_t *buffer_p;
  jerry_size_t buffer_size;
  jerry_size_t buffer_size_expected;
  jerry_size_t buffer_index;
  jerry_size_t buffer_index_init;
  jerry_size_t segment_length;

  const jerry_string_t *path_list_p;
  jerry_size_t path_list_count;
  jerry_size_t path_total_size;

  jerry_size_t segment_eat_count;
  jerry_path_iterator_state_t iterator_state;
  bool only_first_path_is_root;
} jerry_path_join_state_t;

typedef struct
{
  int32_t list_pos;
  jerry_size_t item_pos;
  jerry_size_t root_length;
} jerry_path_iterator_t;

static bool
jerry_module_path_is_separator (jerry_path_style_t style, const jerry_char_t ch)
{
  if (style == JERRY_PATH_STYLE_WINDOWS)
  {
    return ch == '/' || ch == '\\';
  }
  else
  {
    return ch == '/';
  }
} /* jerry_module_path_is_separator */

static const jerry_char_t *
jerry_module_path_find_next_stop (jerry_path_style_t path_style, const jerry_char_t *c)
{
  // We just move forward until we find a '\0' or a separator, which will be our
  // next "stop".
  while (*c != '\0' && !jerry_module_path_is_separator (path_style, *c))
  {
    ++c;
  }

  // Return the pointer of the next stop.
  return c;
} /* jerry_module_path_find_next_stop */

static void
jerry_module_path_get_root_windows (const jerry_char_t *path, jerry_size_t *length)
{
  const jerry_char_t *c;

  // We can not determine the root if this is an empty string. So we set the
  // root to NULL and the length to zero and cancel the whole thing.
  c = path;
  *length = 0;
  if (!*c)
  {
    return;
  }

  // Now we have to verify whether this is a windows network path (UNC), which
  // we will consider our root.
  if (jerry_module_path_is_separator (JERRY_PATH_STYLE_WINDOWS, *c))
  {
    bool is_device_path;
    ++c;

    // Check whether the path starts with a single backslash, which means this
    // is not a network path - just a normal path starting with a backslash.
    if (!jerry_module_path_is_separator (JERRY_PATH_STYLE_WINDOWS, *c))
    {
      // Okay, this is not a network path but we still use the backslash as a
      // root.
      ++(*length);
      return;
    }

    // A device path is a path which starts with "\\." or "\\?". A device path
    // can be a UNC path as well, in which case it will take up one more
    // segment. So, this is a network or device path. Skip the previous
    // separator. Now we need to determine whether this is a device path. We
    // might advance one character here if the server name starts with a '?' or
    // a '.', but that's fine since we will search for a separator afterwards
    // anyway.
    ++c;
    is_device_path = (*c == '?' || *c == '.') && jerry_module_path_is_separator (JERRY_PATH_STYLE_WINDOWS, *(++c));
    if (is_device_path)
    {
      // That's a device path, and the root must be either "\\.\" or "\\?\"
      // which is 4 characters long. (at least that's how Windows
      // GetFullPathName behaves.)
      *length = 4;
      return;
    }

    // We will grab anything up to the next stop. The next stop might be a '\0'
    // or another separator. That will be the server name.
    c = jerry_module_path_find_next_stop (JERRY_PATH_STYLE_WINDOWS, c);

    // If this is a separator and not the end of a string we wil have to include
    // it. However, if this is a '\0' we must not skip it.
    while (jerry_module_path_is_separator (JERRY_PATH_STYLE_WINDOWS, *c))
    {
      ++c;
    }

    // We are now skipping the shared folder name, which will end after the
    // next stop.
    c = jerry_module_path_find_next_stop (JERRY_PATH_STYLE_WINDOWS, c);

    // Then there might be a separator at the end. We will include that as well,
    // it will mark the path as absolute.
    if (jerry_module_path_is_separator (JERRY_PATH_STYLE_WINDOWS, *c))
    {
      ++c;
    }

    // Finally, calculate the size of the root.
    *length = (jerry_size_t) (c - path);
    return;
  }

  // Move to the next and check whether this is a colon.
  if (*++c == ':')
  {
    *length = 2;

    // Now check whether this is a backslash (or slash). If it is not, we could
    // assume that the next character is a '\0' if it is a valid path. However,
    // we will not assume that - since ':' is not valid in a path it must be a
    // mistake by the caller than. We will try to understand it anyway.
    if (jerry_module_path_is_separator (JERRY_PATH_STYLE_WINDOWS, *(++c)))
    {
      *length = 3;
    }
  }
} /* jerry_module_path_get_root_windows */

static void
jerry_module_path_get_root_unix (const jerry_char_t *path, jerry_size_t *length)
{
  // The slash of the unix path represents the root. There is no root if there
  // is no slash.
  if (jerry_module_path_is_separator (JERRY_PATH_STYLE_POSIX, *path))
  {
    *length = 1;
  }
  else
  {
    *length = 0;
  }
} /* jerry_module_path_get_root_unix */

static void
jerry_module_path_get_root (jerry_path_style_t path_style, const jerry_char_t *path, jerry_size_t *length)
{
  // We use a different implementation here based on the configuration of the
  // library.
  if (path_style == JERRY_PATH_STYLE_WINDOWS)
  {
    jerry_module_path_get_root_windows (path, length);
  }
  else
  {
    jerry_module_path_get_root_unix (path, length);
  }
} /* jerry_module_path_get_root */

static jerry_char_t
jerry_module_path_get_char (jerry_path_join_state_t *state, jerry_path_iterator_t *it)
{
  if (it->list_pos < 0)
  {
    return 0;
  }
  if (it->item_pos == state->path_list_p[it->list_pos].size)
  {
    return '/';
  }
  return state->path_list_p[it->list_pos].ptr[it->item_pos];
} /* jerry_module_path_get_char */

static void
jerry_module_path_update_root_length (jerry_path_join_state_t *state, jerry_path_iterator_t *it)
{
  state->path_total_size += state->path_list_p[it->list_pos].size;
  if (!state->only_first_path_is_root || it->list_pos == 0)
  {
    jerry_module_path_get_root (state->style, state->path_list_p[it->list_pos].ptr, &it->root_length);
  }
} /* jerry_module_path_update_root_length */

static void
jerry_module_path_interator_prev (jerry_path_join_state_t *state, jerry_path_iterator_t *it)
{
  if (it->list_pos < 0)
  {
    return;
  }
  if (it->item_pos > 0)
  {
    --it->item_pos;
    return;
  }
  if (it->root_length > 0)
  {
    it->list_pos = -1;
    return;
  }
  it->list_pos -= 1;
  if (it->list_pos >= 0)
  {
    jerry_module_path_update_root_length (state, it);
    /**
     * The tail '\0' treat as '/' that is a path separator for both
     * POSIX/WINDOWS
     */
    it->item_pos = state->path_list_p[it->list_pos].size;
  }
} /* jerry_module_path_interator_prev */

static void
jerry_module_path_push_front_direct (jerry_path_join_state_t *state, jerry_char_t ch)
{
  --state->buffer_index;
  if (state->buffer_p)
  {
    if (state->buffer_index < state->buffer_size)
    {
      state->buffer_p[state->buffer_index] = ch;
    }
  }
} /* jerry_module_path_push_front_direct */

static void
jerry_module_path_push_front (jerry_path_join_state_t *state, jerry_char_t ch)
{
  jerry_module_path_push_front_direct (state, ch);
  if (jerry_module_path_is_separator (state->style, ch))
  {
    state->segment_length = 0;
  }
  else
  {
    state->segment_length += 1;
  }
} /* jerry_module_path_push_front */

static jerry_char_t
jerry_module_path_get_separator (jerry_path_style_t path_style)
{
  return path_style == JERRY_PATH_STYLE_POSIX ? '/' : '\\';
} /* jerry_module_path_get_separator */

static jerry_size_t
jerry_module_path_join_and_normalize (jerry_path_style_t style,
                                      bool only_first_path_is_root,
                                      const jerry_string_t *path_list_p,
                                      jerry_size_t path_list_count,
                                      jerry_char_t *buffer_p,
                                      jerry_size_t buffer_size,
                                      jerry_size_t buffer_size_expected)
{
  jerry_path_join_state_t state = { 0 };
  state.style = style;
  state.only_first_path_is_root = only_first_path_is_root;
  state.buffer_p = buffer_p;
  state.buffer_size = buffer_size;
  state.buffer_size_expected = buffer_size_expected;
  if (buffer_p)
  {
    if (buffer_size_expected > buffer_size)
    {
      /**
       * When `buffer_p` exist and `buffer_size_expected` > `buffer_size` that
       * means there is not enough buffer to storage the final generated path
       * so set buffer_index_init to buffer_size_expected to ensure only the
       * heading part of the generated part are stroaged into `buffer_p`
       */
      state.buffer_index_init = buffer_size_expected;
    }
    else
    {
      /**
       * Always storage at the tail of `buffer_p`. So that when normalize the
       * path inplace, the path own't be corrupted.
       */
      state.buffer_index_init = buffer_size;
    }
  }
  else
  {
    /* For calculating the path size */
    state.buffer_index_init = UINT32_MAX;
  }
  state.buffer_index = state.buffer_index_init;
  state.path_list_p = path_list_p;
  state.path_list_count = path_list_count;
  state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEPARATOR;

  jerry_size_t path_list_tail_pos = path_list_count - 1;
  jerry_path_iterator_t it = { (int32_t) path_list_tail_pos, path_list_p[path_list_tail_pos].size, 0 };
  jerry_module_path_update_root_length (&state, &it);
  jerry_module_path_push_front_direct (&state, '\0');
  for (;;)
  {
    jerry_char_t ch = jerry_module_path_get_char (&state, &it);
    if (ch == 0)
    {
      break;
    }
    switch (state.iterator_state)
    {
      case JERRY_PATH_JOIN_ITERATOR_STATE_ROOT:
      {
        jerry_module_path_push_front_direct (&state, ch);
        state.segment_length += 1;
        break;
      }
      case JERRY_PATH_JOIN_ITERATOR_STATE_SEPARATOR:
      {
        if (ch == '.')
        {
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_ONE_DOT;
        }
        else if (!jerry_module_path_is_separator (style, ch))
        {
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEGMENT;
          jerry_module_path_push_front (&state, ch);
        }
        break;
      }
      case JERRY_PATH_JOIN_ITERATOR_STATE_ONE_DOT:
      {
        if (ch == '.')
        {
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_TWO_DOT;
        }
        else if (jerry_module_path_is_separator (style, ch))
        {
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEPARATOR;
        }
        else
        {
          jerry_module_path_push_front (&state, '.');
          jerry_module_path_push_front (&state, ch);
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEGMENT;
        }
        break;
      }
      case JERRY_PATH_JOIN_ITERATOR_STATE_TWO_DOT:
      {
        if (jerry_module_path_is_separator (style, ch))
        {
          state.segment_eat_count += 1;
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEPARATOR;
        }
        else
        {
          jerry_module_path_push_front (&state, ch);
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEGMENT;
        }
        break;
      }
      case JERRY_PATH_JOIN_ITERATOR_STATE_SEGMENT:
      {
        if (jerry_module_path_is_separator (style, ch))
        {
          if (state.segment_eat_count > 0)
          {
            state.segment_eat_count -= 1;
            state.buffer_index += state.segment_length;
            state.segment_length = 0;
          }
          else
          {
            jerry_module_path_push_front (&state, jerry_module_path_get_separator (style));
          }
          state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_SEPARATOR;
        }
        else
        {
          jerry_module_path_push_front (&state, ch);
        }
        break;
      }
    }

    if (it.item_pos < it.root_length)
    {
      state.iterator_state = JERRY_PATH_JOIN_ITERATOR_STATE_ROOT;
      if (it.root_length == 1 && (state.buffer_index_init - state.buffer_index) == 1)
      {
        jerry_module_path_push_front (&state, jerry_module_path_get_separator (style));
      }
    }
    jerry_module_path_interator_prev (&state, &it);
  }
  if (state.iterator_state != JERRY_PATH_JOIN_ITERATOR_STATE_ROOT)
  {
    if (state.segment_eat_count > 0 && state.segment_length > 0)
    {
      state.segment_eat_count -= 1;
      state.buffer_index += state.segment_length;
      state.segment_length = 0;
      if (state.buffer_index_init - state.buffer_index > 1)
      {
        /* Strip the starting path separator `/` or `\` */
        state.buffer_index += 1;
      }
    }

    if (state.buffer_index_init - state.buffer_index == 1)
    {
      if (state.segment_eat_count == 0)
      {
        if (state.path_total_size > 0)
        {
          jerry_module_path_push_front (&state, '.');
        }
      }
      else
      {
        for (jerry_size_t i = 0; i < state.segment_eat_count; i += 1)
        {
          jerry_module_path_push_front (&state, '.');
          jerry_module_path_push_front (&state, '.');
          if (i < state.segment_eat_count - 1)
          {
            jerry_module_path_push_front (&state, '/');
          }
        }
      }
    }
  }
  jerry_size_t path_size = state.buffer_index_init - state.buffer_index;
  if (state.buffer_p)
  {
    if (state.buffer_index > 0)
    {
      memmove (buffer_p, buffer_p + state.buffer_index, path_size);
    }
    else
    {
      if (buffer_size > 0 && buffer_size < path_size)
      {
        buffer_p[buffer_size - 1] = '\0';
      }
    }
  }
  return path_size - 1;
} /* jerry_module_path_join_and_normalize */

/**
 * Get the end of the directory part of the input path.
 *
 * @param path_p: input zero-terminated path string
 *
 * @return offset of the directory end in the input path string
 */
static jerry_size_t
jerry_module_path_base (jerry_path_style_t style, const jerry_char_t *path_p, jerry_size_t path_size)
{
  const jerry_char_t *end_p = path_p + path_size;

  while (end_p > path_p)
  {
    if (jerry_module_path_is_separator (style, *(end_p - 1)))
    {
      return (jerry_size_t) (end_p - path_p);
    }

    end_p--;
  }

  return 0;
} /* jerry_module_path_base */

static jerry_size_t
jerry_module_get_cwd (jerry_char_t **path_p)
{
  jerry_char_t *buffer_p = NULL;
  jerry_size_t path_size = jerry_port_get_cwd (NULL, 0);
  if (path_size > 0)
  {
    jerry_size_t buffer_size = path_size + 1;
    buffer_p = jerry_heap_alloc (buffer_size);
    if (buffer_p)
    {
      if (jerry_port_get_cwd (buffer_p, buffer_size) != path_size)
      {
        jerry_heap_free (buffer_p, buffer_size);
        buffer_p = NULL;
      }
    }
  }
  *path_p = buffer_p;
  return path_size;
} /* jerry_module_get_cwd */

static jerry_size_t
jerry_module_path_join_allocated (jerry_path_style_t style,
                                  const jerry_string_t *path_list_p,
                                  jerry_size_t path_list_count,
                                  jerry_char_t **path_out_p)
{
  jerry_char_t *buffer_p = NULL;
  jerry_size_t path_size =
    jerry_module_path_join_and_normalize (style, false, path_list_p, path_list_count, NULL, 0, 0);
  if (path_size > 0)
  {
    jerry_size_t buffer_size = path_size + 1;
    buffer_p = jerry_heap_alloc (buffer_size);
    if (buffer_p)
    {
      if (jerry_module_path_join_and_normalize (style,
                                                false,
                                                path_list_p,
                                                path_list_count,
                                                buffer_p,
                                                buffer_size,
                                                buffer_size)
          != path_size)
      {
        jerry_heap_free (buffer_p, buffer_size);
        buffer_p = NULL;
      }
    }
  }
  *path_out_p = buffer_p;
  return path_size;
} /* jerry_module_path_join_allocated */

/**
 * Release known modules.
 */
static void
jerry_module_free (jerry_module_manager_t *manager_p, /**< module manager */
                   const jerry_value_t realm) /**< if this argument is object, release only those modules,
                                               *   which realm value is equal to this argument. */
{
  jerry_module_t *module_p = manager_p->module_head_p;

  bool release_all = !jerry_value_is_object (realm);

  jerry_module_t *prev_p = NULL;

  while (module_p != NULL)
  {
    jerry_module_t *next_p = module_p->next_p;

    if (release_all || module_p->realm == realm)
    {
      jerry_heap_free (module_p->path_p, module_p->path_size + 1);
      jerry_value_free (module_p->realm);
      jerry_value_free (module_p->module);

      jerry_heap_free (module_p, sizeof (jerry_module_t));

      if (prev_p == NULL)
      {
        manager_p->module_head_p = next_p;
      }
      else
      {
        prev_p->next_p = next_p;
      }
    }
    else
    {
      prev_p = module_p;
    }

    module_p = next_p;
  }
} /* jerry_module_free */

/**
 * Initialize the default module manager.
 */
static void
jerry_module_manager_init (void *user_data_p)
{
  ((jerry_module_manager_t *) user_data_p)->module_head_p = NULL;
} /* jerry_module_manager_init */

/**
 * Deinitialize the default module manager.
 */
static void
jerry_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jerry_value_t undef = jerry_undefined ();
  jerry_module_free ((jerry_module_manager_t *) user_data_p, undef);
  jerry_value_free (undef);
} /* jerry_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jerry_context_data_manager_t jerry_module_manager JERRY_ATTR_CONST_DATA = {
  .init_cb = jerry_module_manager_init,
  .deinit_cb = jerry_module_manager_deinit,
  .bytes_needed = sizeof (jerry_module_manager_t)
};

#endif /* JERRY_MODULE_SYSTEM */

/**
 * Default module resolver.
 *
 * @return a module object if resolving is successful, an error otherwise
 */
jerry_value_t
jerry_module_resolve (const jerry_value_t specifier, /**< module specifier string */
                      const jerry_value_t referrer, /**< parent module */
                      void *user_p) /**< user data */
{
#if JERRY_MODULE_SYSTEM
  JERRY_UNUSED (user_p);

  const jerry_char_t *directory_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);
  jerry_size_t directory_size = lit_get_magic_string_size (LIT_MAGIC_STRING__EMPTY);

  jerry_module_t *module_p = jerry_object_get_native_ptr (referrer, &jerry_module_native_info);

  if (module_p != NULL)
  {
    directory_p = module_p->path_p;
    directory_size = module_p->basename_offset;
  }

  jerry_size_t specifier_size = jerry_string_size (specifier, JERRY_ENCODING_UTF8);
  jerry_size_t reference_size = directory_size + specifier_size;
  jerry_char_t *reference_path_p = jerry_heap_alloc (reference_size + 1);

  memcpy (reference_path_p, directory_p, directory_size);
  jerry_string_to_buffer (specifier, JERRY_ENCODING_UTF8, reference_path_p + directory_size, specifier_size);
  reference_path_p[reference_size] = '\0';

  jerry_char_t *path_p = NULL;
  jerry_size_t path_size = 0;
  jerry_path_style_t path_style = jerry_port_path_style ();
  jerry_char_t *cwd_path_p = NULL;
  jerry_size_t cwd_path_size = jerry_module_get_cwd (&cwd_path_p);
  if (cwd_path_p)
  {
    jerry_string_t path_list[] = { { cwd_path_p, cwd_path_size }, { reference_path_p, reference_size } };
    path_size =
      jerry_module_path_join_allocated (path_style, path_list, sizeof (path_list) / sizeof (path_list[0]), &path_p);
    jerry_heap_free (cwd_path_p, cwd_path_size + 1);
  }

  jerry_heap_free (reference_path_p, reference_size + 1);

  if (path_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Failed to resolve module");
  }

  jerry_value_t realm = jerry_current_realm ();

  jerry_module_manager_t *manager_p;
  manager_p = (jerry_module_manager_t *) jerry_context_data (&jerry_module_manager);

  module_p = manager_p->module_head_p;

  while (module_p != NULL)
  {
    if (module_p->realm == realm && module_p->path_size == path_size
        && memcmp (module_p->path_p, path_p, path_size) == 0)
    {
      jerry_value_free (realm);
      jerry_heap_free (path_p, path_size + 1);
      return jerry_value_copy (module_p->module);
    }

    module_p = module_p->next_p;
  }

  jerry_size_t source_size;
  jerry_char_t *source_p = jerry_port_source_read ((const char *) path_p, &source_size);

  if (source_p == NULL)
  {
    jerry_value_free (realm);
    jerry_heap_free (path_p, path_size + 1);

    return jerry_throw_sz (JERRY_ERROR_SYNTAX, "Module file not found");
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_value_copy (specifier);

  jerry_value_t ret_value = jerry_parse (source_p, source_size, &parse_options);
  jerry_value_free (parse_options.source_name);

  jerry_port_source_free (source_p);

  if (jerry_value_is_exception (ret_value))
  {
    jerry_heap_free (path_p, path_size + 1);
    jerry_value_free (realm);
    return ret_value;
  }

  module_p = (jerry_module_t *) jerry_heap_alloc (sizeof (jerry_module_t));

  module_p->next_p = manager_p->module_head_p;
  module_p->path_p = path_p;
  module_p->path_size = path_size;
  module_p->basename_offset = jerry_module_path_base (path_style, module_p->path_p, module_p->path_size);
  module_p->realm = realm;
  module_p->module = jerry_value_copy (ret_value);

  jerry_object_set_native_ptr (ret_value, &jerry_module_native_info, module_p);
  manager_p->module_head_p = module_p;

  return ret_value;
#else /* JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (specifier);
  JERRY_UNUSED (referrer);
  JERRY_UNUSED (user_p);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_resolve */

/**
 * Release known modules.
 */
void
jerry_module_cleanup (const jerry_value_t realm) /**< if this argument is object, release only those modules,
                                                  *   which realm value is equal to this argument. */
{
#if JERRY_MODULE_SYSTEM
  jerry_module_free ((jerry_module_manager_t *) jerry_context_data (&jerry_module_manager), realm);
#else /* JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (realm);
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_cleanup */
