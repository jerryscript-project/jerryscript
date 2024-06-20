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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript-port.h"

#if defined(__GLIBC__) || defined(_WIN32)
#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) &S_IFMT) == S_IFDIR)
#endif /* !defined(S_ISDIR) */
#endif /* __GLIBC__ */

/**
 * Determines the size of the given file.
 * @return size of the file
 */
static jerry_size_t
jerry_port_get_file_size (FILE *file_p) /**< opened file */
{
  fseek (file_p, 0, SEEK_END);
  long size = ftell (file_p);
  fseek (file_p, 0, SEEK_SET);

  return (jerry_size_t) size;
} /* jerry_port_get_file_size */

/**
 * Opens file with the given path and reads its source.
 * @return the source of the file
 */
jerry_char_t *JERRY_ATTR_WEAK
jerry_port_source_read (const jerry_char_t *file_name_p, jerry_size_t file_name_size, jerry_size_t *out_size_p)
{
  (void) file_name_size;
  /* TODO(dbatyai): Temporary workaround for nuttx target
   * The nuttx target builds and copies the jerryscript libraries as a separate build step, which causes linking issues
   * later due to different libc libraries. It should incorporate the amalgam sources into the main nuttx build so that
   * the correct libraries are used, then this guard should be removed from here and also from the includes. */
#if defined(__GLIBC__) || defined(_WIN32)
  struct stat stat_buffer;
  if (stat ((const char *) file_name_p, &stat_buffer) == -1 || S_ISDIR (stat_buffer.st_mode))
  {
    return NULL;
  }
#endif /* __GLIBC__ */

  FILE *file_p = fopen ((const char *) file_name_p, "rb");

  if (file_p == NULL)
  {
    return NULL;
  }

  jerry_size_t file_size = jerry_port_get_file_size (file_p);
  jerry_char_t *buffer_p = (jerry_char_t *) malloc (file_size);

  if (buffer_p == NULL)
  {
    fclose (file_p);
    return NULL;
  }

  size_t bytes_read = fread (buffer_p, 1u, file_size, file_p);

  if (bytes_read != file_size)
  {
    fclose (file_p);
    free (buffer_p);
    return NULL;
  }

  fclose (file_p);
  *out_size_p = (jerry_size_t) bytes_read;

  return buffer_p;
} /* jerry_port_source_read */

/**
 * Release the previously opened file's content.
 */
void JERRY_ATTR_WEAK
jerry_port_source_free (uint8_t *buffer_p) /**< buffer to free */
{
  free (buffer_p);
} /* jerry_port_source_free */

/**
 * Evaluate path root or check if char is path separator is only different on win32
 */
#if !defined(_WIN32)

bool JERRY_ATTR_WEAK
jerry_port_path_is_separator (jerry_char_t path_c)
{
  if (path_c == '/')
  {
    return true;
  }
  return false;
} /* jerry_port_path_is_separator */

jerry_size_t JERRY_ATTR_WEAK
jerry_port_path_root (const jerry_char_t *path_p, jerry_size_t path_size)
{
  if (path_p == NULL)
  {
    return 0;
  }
  if (path_size == 0)
  {
    return 0;
  }
  if (jerry_port_path_is_separator (path_p[0]))
  {
    return 1;
  }
  return 0;
} /* jerry_port_path_root */

#endif /* !defined(_WIN32) */

/**
 * These functions provide generic implementation for paths and are only enabled when the compiler support weak symbols,
 * and we are not building for a platform that has platform specific versions.
 */
#if defined(JERRY_WEAK_SYMBOL_SUPPORT) && !(defined(__unix__) || defined(__APPLE__) || defined(_WIN32))

/**
 * Normalize a file path.
 *
 * @return a newly allocated buffer with the normalized path if the operation is successful,
 *         NULL otherwise
 */
jerry_char_t *JERRY_ATTR_WEAK
jerry_port_path_normalize (const jerry_char_t *path_p, jerry_size_t path_size, jerry_size_t *out_size_p)
{
  jerry_char_t *buffer_p = (jerry_char_t *) malloc (path_size + 1);

  if (buffer_p == NULL)
  {
    return NULL;
  }

  /* Also copy terminating zero byte. */
  memcpy (buffer_p, path_p, path_size + 1);
  *out_size_p = path_size;
  return buffer_p;
} /* jerry_port_normalize_path */

/**
 * Free a path buffer returned by jerry_port_path_normalize.
 *
 * @param path_p: the path to free
 */
void JERRY_ATTR_WEAK
jerry_port_path_free (jerry_char_t *path_p)
{
  free (path_p);
} /* jerry_port_normalize_path */

#endif /* defined(JERRY_WEAK_SYMBOL_SUPPORT) && !(defined(__unix__) || defined(__APPLE__) || defined(_WIN32)) */
