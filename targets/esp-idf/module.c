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

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jerryscript-port.h"

/**
 * Opens file with the given path and reads its source.
 * @return the source of the file
 */
uint8_t *
jerry_port_read_source (const char *file_name_p, /**< file name */
                        size_t *out_size_p) /**< [out] read bytes */
{
  FILE *file_p = fopen (file_name_p, "rb");

  if (file_p == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Failed to open file: %s\n", file_name_p);
    return NULL;
  }

  struct stat info = { };
  fstat(fileno(file_p), &info);	
  uint8_t *buffer_p = (uint8_t *) malloc (info.st_size);

  if (buffer_p == NULL)
  {
    fclose (file_p);

    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Failed to allocate memory for file: %s\n", file_name_p);
    return NULL;
  }

  size_t bytes_read = fread (buffer_p, 1u, info.st_size, file_p);
  if (bytes_read != info.st_size)
  {
    fclose (file_p);
    free (buffer_p);

    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Failed to read file: %s\n", file_name_p);
    return NULL;
  }

  fclose (file_p);
  *out_size_p = bytes_read;

  return buffer_p;
} /* jerry_port_read_source */

/**
 * Release the previously opened file's content.
 */
void
jerry_port_release_source (uint8_t *buffer_p) /**< buffer to free */
{
  free (buffer_p);
} /* jerry_port_release_source */

/**
 * Normalize a file path
 *
 * @return length of the path written to the output buffer
 */
size_t
jerry_port_normalize_path (const char *in_path_p,   /**< input file path */
                           char *out_buf_p,         /**< output buffer */
                           size_t out_buf_size,     /**< size of output buffer */
                           char *base_file_p)       /**< base file path */
{
  size_t ret = strlen(base_file_p) + strlen(in_path_p) + 1;

  if (ret < out_buf_size) {
    strcpy (out_buf_p, base_file_p);
    strcat (out_buf_p, "/");
    strcat (out_buf_p, in_path_p);
    return ret;
  }

  return 0; 
} /* jerry_port_normalize_path */

/**
 * Get the module object of a native module.
 *
 * @return Undefined, if 'name' is not a native module
 *         jerry_value_t containing the module object, otherwise
 */
jerry_value_t
jerry_port_get_native_module (jerry_value_t name) /**< module specifier */
{
  (void) name;
  return jerry_create_undefined ();
} /* jerry_port_get_native_module */
