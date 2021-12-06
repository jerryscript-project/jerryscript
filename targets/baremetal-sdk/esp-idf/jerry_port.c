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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char TAG[] = "JS";

/**
 * Default implementation of jerry_port_fatal. Calls 'abort' if exit code is
 * non-zero, 'exit' otherwise.
 */
void
jerry_port_fatal (jerry_fatal_code_t code) /**< cause of error */
{
  ESP_LOGE (TAG, "Fatal error %d", code);
  vTaskSuspend (NULL);
  abort ();
} /* jerry_port_fatal */

/**
 * Default implementation of jerry_port_sleep. Uses 'nanosleep' or 'usleep' if
 * available on the system, does nothing otherwise.
 */
void
jerry_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  vTaskDelay (sleep_time / portTICK_PERIOD_MS);
} /* jerry_port_sleep */

/**
 * Opens file with the given path and reads its source.
 * @return the source of the file
 */
jerry_char_t *
jerry_port_source_read (const char *file_name_p, /**< file name */
                        jerry_size_t *out_size_p) /**< [out] read bytes */
{
  FILE *file_p = fopen (file_name_p, "rb");

  if (file_p == NULL)
  {
    return NULL;
  }

  struct stat info = {};
  fstat (fileno (file_p), &info);
  jerry_char_t *buffer_p = (jerry_char_t *) malloc (info.st_size);

  if (buffer_p == NULL)
  {
    fclose (file_p);

    return NULL;
  }

  size_t bytes_read = fread (buffer_p, 1u, info.st_size, file_p);
  if (bytes_read != info.st_size)
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
void
jerry_port_source_free (uint8_t *buffer_p) /**< buffer to free */
{
  free (buffer_p);
} /* jerry_port_source_free */

/**
 * Default implementation of jerry_port_local_tza. Uses the 'tm_gmtoff' field
 * of 'struct tm' (a GNU extension) filled by 'localtime_r' if available on the
 * system, does nothing otherwise.
 *
 * @return offset between UTC and local time at the given unix timestamp, if
 *         available. Otherwise, returns 0, assuming UTC time.
 */
int32_t
jerry_port_local_tza (double unix_ms)
{
  struct tm tm;
  char buf[8];
  time_t now = (time_t) unix_ms / 1000;

  localtime_r (&now, &tm);
  strftime (buf, 8, "%z", &tm);
  return -atoi (buf) * 3600 * 1000 / 100;
} /* jerry_port_local_tza */

/**
 * Default implementation of jerry_port_current_time. Uses 'gettimeofday' if
 * available on the system, does nothing otherwise.
 *
 * @return milliseconds since Unix epoch - if 'gettimeofday' is available and
 *                                         executed successfully,
 *         0 - otherwise.
 */
double
jerry_port_current_time (void)
{
  struct timeval tv;

  if (gettimeofday (&tv, NULL) == 0)
  {
    return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
  }
  return 0.0;
} /* jerry_port_current_time */
