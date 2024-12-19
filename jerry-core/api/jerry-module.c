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

typedef struct
{
  const jerry_string_t *path_list_p;
  jerry_size_t path_list_count;
  jerry_size_t root_length;
  bool root_is_absolute;

  bool end_with_separator;
  jerry_size_t list_pos;
  jerry_size_t pos;
  jerry_size_t length;
  jerry_size_t segment_eat_count;
  jerry_size_t segment_count;
} jerry_segment_iterator_t;

static bool
jerry_path_is_separator (jerry_path_style_t style, const jerry_char_t ch)
{
  if (style == JERRY_STYLE_WINDOWS)
  {
    return ch == '/' || ch == '\\';
  }
  else
  {
    return ch == '/';
  }
} /* jerry_path_is_separator */

static jerry_size_t
jerry_path_get_root_windows (const jerry_char_t *path)
{
  const jerry_char_t *c;
  jerry_size_t length = 0;
  // We can not determine the root if this is an empty string. So we set the
  // root to NULL and the length to zero and cancel the whole thing.
  c = path;
  if (!*c)
  {
    return length;
  }

  // Now we have to verify whether this is a windows network path (UNC), which
  // we will consider our root.
  if (jerry_path_is_separator (JERRY_STYLE_WINDOWS, *c))
  {
    bool is_device_path;
    ++c;

    // Check whether the path starts with a single backslash, which means this
    // is not a network path - just a normal path starting with a backslash.
    if (!jerry_path_is_separator (JERRY_STYLE_WINDOWS, *c))
    {
      // Okay, this is not a network path but we still use the backslash as a
      // root.
      ++length;
      return length;
    }

    // A device path is a path which starts with "\\." or "\\?". A device path
    // can be a UNC path as well, in which case it will take up one more
    // segment. So, this is a network or device path. Skip the previous
    // separator. Now we need to determine whether this is a device path. We
    // might advance one character here if the server name starts with a '?' or
    // a '.', but that's fine since we will search for a separator afterwards
    // anyway.
    ++c;
    is_device_path = (*c == '?' || *c == '.') && jerry_path_is_separator (JERRY_STYLE_WINDOWS, *(++c));
    if (is_device_path)
    {
      // That's a device path, and the root must be either "\\.\" or "\\?\"
      // which is 4 characters long. (at least that's how Windows
      // GetFullPathName behaves.)
      length = 4;
      return length;
    }

    // We will grab anything up to the next stop. The next stop might be a '\0'
    // or another separator. That will be the server name.
    while (*c != '\0' && !jerry_path_is_separator (JERRY_STYLE_WINDOWS, *c))
    {
      ++c;
    }

    // If this is a separator and not the end of a string we wil have to include
    // it. However, if this is a '\0' we must not skip it.
    while (jerry_path_is_separator (JERRY_STYLE_WINDOWS, *c))
    {
      ++c;
    }

    // We are now skipping the shared folder name, which will end after the
    // next stop.
    while (*c != '\0' && !jerry_path_is_separator (JERRY_STYLE_WINDOWS, *c))
    {
      ++c;
    }
    // Then there might be a separator at the end. We will include that as well,
    // it will mark the path as absolute.
    if (jerry_path_is_separator (JERRY_STYLE_WINDOWS, *c))
    {
      ++c;
    }

    // Finally, calculate the size of the root.
    length = (jerry_size_t) (c - path);
    return length;
  }

  // Move to the next and check whether this is a colon.
  if (*++c == ':')
  {
    length = 2;

    // Now check whether this is a backslash (or slash). If it is not, we could
    // assume that the next character is a '\0' if it is a valid path. However,
    // we will not assume that - since ':' is not valid in a path it must be a
    // mistake by the caller than. We will try to understand it anyway.
    if (jerry_path_is_separator (JERRY_STYLE_WINDOWS, *(++c)))
    {
      length = 3;
    }
  }
  return length;
} /* jerry_path_get_root_windows */

static jerry_size_t
jerry_path_get_root_unix (const jerry_char_t *path)
{
  // The slash of the unix path represents the root. There is no root if there
  // is no slash.
  return jerry_path_is_separator (JERRY_STYLE_UNIX, *path) ? 1 : 0;
} /* jerry_path_get_root_unix */

static jerry_size_t
jerry_path_get_root (jerry_path_style_t path_style, const jerry_char_t *path)
{
  if (!path)
  {
    return 0;
  }
  // We use a different implementation here based on the configuration of the
  // library.
  return path_style == JERRY_STYLE_WINDOWS ? jerry_path_get_root_windows (path) : jerry_path_get_root_unix (path);
} /* jerry_path_get_root */

static bool
jerry_path_iterator_before_root (jerry_segment_iterator_t *it)
{
  return it->list_pos == 0 && ((it->pos + 1) <= it->root_length);
} /* jerry_path_iterator_before_root */

static jerry_char_t
jerry_path_char_get (jerry_segment_iterator_t *it)
{
  if (jerry_path_iterator_before_root (it))
  {
    return 0;
  }
  if (it->pos == JERRY_SIZE_MAX)
  {
    /**
     * The tail '\0' treat as '/' that is a path separator for both
     * POSIX/WINDOWS
     */
    return '/';
  }
  return it->path_list_p[it->list_pos].ptr[it->pos];
} /* jerry_path_char_get */

static void
jerry_path_char_iter_prev (jerry_segment_iterator_t *it)
{
  if (jerry_path_iterator_before_root (it))
  {
    return;
  }
  if (it->pos == JERRY_SIZE_MAX)
  {
    it->list_pos -= 1;
    it->pos = it->path_list_p[it->list_pos].size - 1;
  }
  else
  {
    --it->pos;
  }
} /* jerry_path_char_iter_prev */

static void
jerry_path_get_prev_segment_detail (jerry_path_style_t path_style, jerry_segment_iterator_t *it)
{
  if (it->segment_eat_count > 0)
  {
    --it->segment_eat_count;
    if (it->segment_eat_count > 0)
    {
      return;
    }
  }
  if (it->list_pos == 0 && it->pos == JERRY_SIZE_MAX && it->length == it->root_length)
  {
    /* To the head */
    it->length = 0;
    return;
  }
  for (;;)
  {
    jerry_char_t ch;
    jerry_size_t segment_length = 0;
    for (;;)
    {
      ch = jerry_path_char_get (it);
      if (jerry_path_is_separator (path_style, ch))
      {
        jerry_path_char_iter_prev (it);
        continue;
      }
      if (ch != 0)
      {
        segment_length += 1;
      }
      break;
    }
    for (;;)
    {
      jerry_path_char_iter_prev (it);
      ch = jerry_path_char_get (it);
      if (ch == 0 || jerry_path_is_separator (path_style, ch))
      {
        break;
      }
      segment_length += 1;
    }
    const jerry_string_t *path_current = it->path_list_p + it->list_pos;
    if (segment_length == 1)
    {
      if (path_current->ptr[it->pos + 1] == '.')
      {
        continue;
      }
    }
    else if (segment_length == 2)
    {
      if (path_current->ptr[it->pos + 1] == '.' || path_current->ptr[it->pos + 2] == '.')
      {
        it->segment_eat_count += 1;
        continue;
      }
    }
    if (it->segment_eat_count > 0 && segment_length > 0)
    {
      it->segment_eat_count -= 1;
      continue;
    }
    if (segment_length > 0)
    {
      /* segment_eat_count must be zero, this is a normal segment */
      it->length = segment_length;
      return;
    }
    it->length = 0;
    if (ch == 0 && it->root_is_absolute)
    {
      /* Dropping segment eat when the root segment is absolute */
      it->segment_eat_count = 0;
    }
    if (it->segment_eat_count > 0)
    {
      /* Genearting .. segment for relative path */
      return;
    }

    if (ch == 0)
    {
      if (it->segment_count == 0 && !it->root_is_absolute)
      {
        /* Path like `C:` `C:abc\..` `` `abc\..`  `.` should place a . as the
         * path component */
        return;
      }

      /* Return the root segment or head depends on `root_length` */
      it->pos = JERRY_SIZE_MAX;
      it->length = it->root_length;
    }
    return;
  } /* for (;;) */
} /* jerry_path_get_prev_segment_detail */

static bool
jerry_path_get_prev_segment (jerry_path_style_t path_style, jerry_segment_iterator_t *it)
{
  jerry_path_get_prev_segment_detail (path_style, it);
  if (it->segment_eat_count == 0 && it->list_pos == 0 && it->pos == JERRY_SIZE_MAX && it->length == 0
      && it->segment_count > 0)
  {
    /* no more segments */
    return false;
  }
  if (it->list_pos == 0 && it->pos == JERRY_SIZE_MAX && it->root_length > 0)
  {
    /* Root have no trailing separator */
    it->end_with_separator = false;
  }
  else if (it->segment_count > 0)
  {
    it->end_with_separator = true;
  }
  else
  {
    /* The last segment preserve the original separator. */
  }

  it->segment_count += 1;
  return true;
} /* jerry_path_get_prev_segment */

static jerry_string_t
jerry_path_get_segment (jerry_segment_iterator_t *it)
{
  jerry_string_t segment;
  const jerry_string_t *path_current = it->path_list_p + it->list_pos;
  jerry_size_t pos = it->pos + 1;
  if (it->length > 0)
  {
    segment.size = it->length;
    segment.ptr = path_current->ptr + pos;
    return segment;
  }
  if (it->segment_eat_count > 0)
  {
    segment.size = 2;
  }
  else
  {
    segment.size = 1;
  }
  if ((it->list_pos + 1 == it->path_list_count) && pos + segment.size == path_current->size)
  {
    /* The latest segment with '.' or '..' */
    segment.ptr = path_current->ptr + pos;
  }
  else if (segment.size == 2)
  {
    segment.ptr = JERRY_ZSTR_LITERAL ("..");
  }
  else
  {
    segment.ptr = JERRY_ZSTR_LITERAL (".");
  }
  return segment;
} /* jerry_path_get_segment */

static void
jerry_path_push_front_char (jerry_path_style_t path_style,
                            jerry_char_t *buffer_p,
                            jerry_size_t buffer_size,
                            jerry_size_t *buffer_index,
                            jerry_char_t ch)
{
  *buffer_index -= 1;
  jerry_size_t buffer_index_current = *buffer_index;
  if (buffer_index_current < buffer_size)
  {
    if (buffer_index_current == (buffer_size - 1))
    {
      buffer_p[buffer_index_current] = '\0';
    }
    else if (jerry_path_is_separator (path_style, ch))
    {
      buffer_p[buffer_index_current] = path_style == JERRY_STYLE_UNIX ? '/' : '\\';
    }
    else
    {
      buffer_p[buffer_index_current] = ch;
    }
  }
} /* jerry_path_push_front_char */

static void
jerry_path_push_front_string (jerry_path_style_t path_style,
                              jerry_char_t *buffer_p,
                              jerry_size_t buffer_size,
                              jerry_size_t *buffer_index,
                              const jerry_string_t *str)
{
  jerry_size_t k = str->size;
  for (; k > 0;)
  {
    jerry_path_push_front_char (path_style, buffer_p, buffer_size, buffer_index, str->ptr[--k]);
  }
} /* jerry_path_push_front_string */

static const jerry_string_t path_list_empty = { JERRY_ZSTR_ARG ("") };

/**
 * Init the path segment interator
 */
static jerry_segment_iterator_t
jerry_path_interator_init (jerry_path_style_t path_style, /**< The style of the path list */
                           bool is_resolve, /**< If do path resolve */
                           bool remove_trailing_slash, /**< If remove the trailing slash symbol */
                           const jerry_string_t *path_list_p, /**< Path list */
                           jerry_size_t path_list_count)
{
  jerry_segment_iterator_t it = { 0 };
  jerry_size_t path_list_i;
  jerry_size_t end_with_separator = JERRY_SIZE_MAX;
  if (path_list_count == 0)
  {
    path_list_count = 1;
    path_list_p = &path_list_empty;
  }
  path_list_i = path_list_count;
  it.path_list_p = path_list_p;
  it.path_list_count = path_list_count;
  for (; path_list_i > 0;)
  {
    const jerry_string_t *path_list_current = path_list_p + (--path_list_i);
    if (end_with_separator == JERRY_SIZE_MAX && path_list_current->size > 0)
    {
      end_with_separator =
        jerry_path_is_separator (path_style, path_list_current->ptr[path_list_current->size - 1]) ? 1 : 0;
    }
    if (it.root_length == 0 && (is_resolve || path_list_i == 0))
    {
      /* Find the first root path from right to left when `is_resolve` are
       * `true` */
      it.root_length = jerry_path_get_root (path_style, path_list_current->ptr);
      if (it.root_length > 0)
      {
        it.path_list_p += path_list_i;
        it.path_list_count -= path_list_i;
      }
    }
  }
  it.root_is_absolute =
    it.root_length > 0 && jerry_path_is_separator (path_style, it.path_list_p[0].ptr[it.root_length - 1]);
  it.list_pos = it.path_list_count;
  it.pos = JERRY_SIZE_MAX;
  if (remove_trailing_slash)
  {
    it.end_with_separator = false;
  }
  else
  {
    it.end_with_separator = end_with_separator == 1;
  }
  return it;
} /* jerry_path_interator_init */

/**
 * @brief Joins multiple paths together.
 *
 * This function generates a new path by joining multiple paths together. It
 * will remove double separators, and unlike cpj_path_get_absolute_test it
 * permits the use of multiple relative paths to combine. The last path of the
 * submitted string array must be set to NULL. The result will be written to a
 * buffer, which might be truncated if the buffer is not large enough to hold
 * the full path. However, the truncated result will always be
 * null-terminated. The returned value is the amount of characters which the
 * resulting path would take if it was not truncated (excluding the
 * null-terminating character).
 *
 * @param path_style Style depending on the operating system. So this should
 * detect whether we should use windows or unix paths.
 * @param is_resolve If true, then the given sequence of paths is processed from
 * right to left, with each subsequent path prepended until an absolute path is
 * constructed. For instance, given the sequence of path segments: /foo, /bar,
 *   baz, calling path.resolve('/foo', '/bar', 'baz') would return /bar/baz
 * because 'baz' is not an absolute path but
 *   '/bar' + '/' + 'baz' is.
 * Otherwise all paths are joined.
 * @param remove_trailing_slash If true then remove the trailing slash of a
 * directory path otherwise preserved.
 * @param path_list_p An array of paths which will be joined.
 * @param path_list_count The count of array of paths.
 * @param buffer_p The buffer where the result will be written to.
 * @param buffer_size The size of the result buffer.
 * @return Returns the size of the joined path, excluding the '\0' teminiator
 */
static jerry_size_t
jerry_path_join_multiple (jerry_path_style_t path_style,
                          bool is_resolve,
                          bool remove_trailing_slash,
                          const jerry_string_t *path_list_p,
                          jerry_size_t path_list_count,
                          jerry_char_t *buffer_p,
                          jerry_size_t buffer_size)
{
  jerry_size_t buffer_size_calculated = 0;
  for (;;)
  {
    jerry_segment_iterator_t it =
      jerry_path_interator_init (path_style, is_resolve, remove_trailing_slash, path_list_p, path_list_count);
    jerry_size_t buffer_index = JERRY_SIZE_MAX;
    jerry_char_t *buffer_p_used;

    if (buffer_size_calculated == 0)
    {
      buffer_p_used = NULL;
      /* For calculating the path size */
      buffer_index = JERRY_SIZE_MAX;
    }
    else
    {
      buffer_p_used = buffer_p;
      if (buffer_size_calculated > buffer_size)
      {
        /**
         * When `buffer_p` exist and `buffer_size_calculated > buffer_size`,
         * that means there is not enough buffer to storage the final generated
         * path, so that set buffer_index_init to `buffer_size_calculated` to
         * ensure only the head part of the generated path are stored into
         * `buffer_p`
         */
        buffer_index = buffer_size_calculated;
      }
      else
      {
        if (buffer_p == it.path_list_p[0].ptr && it.path_list_count == 1)
        {
          /**
           * The input path and output buffer are the same, storing the
           * generated path at the tail of `buffer_p`; so that when normalizing
           * the path inplace, the path won't be corrupted.
           */
          buffer_index = buffer_size;
        }
        else
        {
          buffer_index = buffer_size_calculated;
        }
      }
    }
    jerry_path_push_front_char (path_style, buffer_p_used, buffer_size, &buffer_index, '\0');
    for (; jerry_path_get_prev_segment (path_style, &it);)
    {
      if (it.end_with_separator)
      {
        jerry_path_push_front_char (path_style, buffer_p_used, buffer_size, &buffer_index, '/');
      }
      jerry_string_t segment = jerry_path_get_segment (&it);
      jerry_path_push_front_string (path_style, buffer_p_used, buffer_size, &buffer_index, &segment);
    }
    if (buffer_size_calculated == 0)
    {
      buffer_size_calculated = JERRY_SIZE_MAX - buffer_index;
      if (!buffer_p)
      {
        return buffer_size_calculated - 1;
      }
      continue;
    }
    if (buffer_p)
    {
      if (buffer_index > 0 && buffer_index < buffer_size)
      {
        memmove (buffer_p, buffer_p + buffer_index, buffer_size_calculated);
      }
    }
    return buffer_size_calculated - 1;
  }
} /* jerry_path_join_multiple */

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
    if (jerry_path_is_separator (style, *(end_p - 1)))
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
  jerry_size_t path_size = jerry_path_join_multiple (style, true, true, path_list_p, path_list_count, NULL, 0);
  if (path_size > 0)
  {
    jerry_size_t buffer_size = path_size + 1;
    buffer_p = jerry_heap_alloc (buffer_size);
    if (buffer_p)
    {
      if (jerry_path_join_multiple (style, true, true, path_list_p, path_list_count, buffer_p, buffer_size)
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
                                                  *   whose realm value is equal to this argument. */
{
#if JERRY_MODULE_SYSTEM
  jerry_module_free ((jerry_module_manager_t *) jerry_context_data (&jerry_module_manager), realm);
#else /* JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (realm);
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_cleanup */
