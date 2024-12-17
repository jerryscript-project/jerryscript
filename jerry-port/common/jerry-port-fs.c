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

jerry_char_t *JERRY_ATTR_WEAK
jerry_port_source_read (const char *file_name_p, jerry_size_t *out_size_p)
{
  /* TODO(dbatyai): Temporary workaround for nuttx target
   * The nuttx target builds and copies the jerryscript libraries as a separate build step, which causes linking issues
   * later due to different libc libraries. It should incorporate the amalgam sources into the main nuttx build so that
   * the correct libraries are used, then this guard should be removed from here and also from the includes. */
#if defined(__GLIBC__) || defined(_WIN32)
  struct stat stat_buffer;
  if (stat (file_name_p, &stat_buffer) == -1 || S_ISDIR (stat_buffer.st_mode))
  {
    return NULL;
  }
#endif /* __GLIBC__ */

  FILE *file_p = fopen (file_name_p, "rb");

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

void JERRY_ATTR_WEAK
jerry_port_source_free (uint8_t *buffer_p)
{
  free (buffer_p);
} /* jerry_port_source_free */

/**
 * These functions provide generic implementation for paths and are only enabled when the compiler support weak symbols,
 * and we are not building for a platform that has platform specific versions.
 */
#if !(defined(__unix__) || defined(__APPLE__) || defined(_WIN32))

jerry_char_t *JERRY_ATTR_WEAK
jerry_port_path_normalize (const jerry_char_t *path_p, jerry_size_t path_size)
{
  jerry_char_t *buffer_p = (jerry_char_t *) malloc (path_size + 1);

  if (buffer_p == NULL)
  {
    return NULL;
  }

  /* Also copy terminating zero byte. */
  memcpy (buffer_p, path_p, path_size + 1);

  return buffer_p;
} /* jerry_port_path_normalize */

void JERRY_ATTR_WEAK
jerry_port_path_free (jerry_char_t *path_p)
{
  free (path_p);
} /* jerry_port_path_free */

jerry_size_t JERRY_ATTR_WEAK
jerry_port_path_base (const jerry_char_t *path_p)
{
  const jerry_char_t *basename_p = (jerry_char_t *) strrchr ((char *) path_p, '/') + 1;

  if (basename_p == NULL)
  {
    return 0;
  }

  return (jerry_size_t) (basename_p - path_p);
} /* jerry_port_path_base */

#endif /* !(defined(__unix__) || defined(__APPLE__) || defined(_WIN32)) */
