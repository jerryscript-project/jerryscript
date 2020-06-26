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

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

/**
 * Maximum size of source code
 */
#define JERRY_BUFFER_SIZE (1048576)
static uint8_t buffer[JERRY_BUFFER_SIZE];

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define REPRL_CRFD 100
#define REPRL_CWFD 101
#define REPRL_DRFD 102
#define REPRL_DWFD 103

#define SHM_SIZE 0x100000
#define MAX_EDGES ((SHM_SIZE - 4) * 8)

#ifndef DCHECK
#define DCHECK(condition) \
  {                       \
    assert(condition);    \
    abort();              \
  }
#endif

#ifndef CHECK
#define CHECK DCHECK
#endif

FILE *fdopen(int fd, const char *mode);
void __sanitizer_cov_reset_edgeguards(void);
void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop);

struct shmem_data
{
  uint32_t num_edges;
  unsigned char edges[];
};

struct shmem_data *__shmem;
uint32_t *__edges_start, *__edges_stop;

void __sanitizer_cov_reset_edgeguards()
{
  uint32_t N = 0;
  for (uint32_t *x = __edges_start; x < __edges_stop && N < MAX_EDGES; x++)
    *x = ++N;
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
  // Avoid duplicate initialization
  if (start == stop || *start)
    return;

  if (__edges_start != NULL || __edges_stop != NULL)
  {
    fprintf(stderr, "Coverage instrumentation is only supported for a single module\n");
    _exit(-1);
  }

  __edges_start = start;
  __edges_stop = stop;

  // Map the shared memory region
  const char *shm_key = getenv("SHM_ID");
  if (!shm_key)
  {
    puts("[COV] no shared memory bitmap available, skipping");
    __shmem = (struct shmem_data *)malloc(SHM_SIZE);
  }
  else
  {
    int fd = shm_open(shm_key, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd <= -1)
    {
      fprintf(stderr, "Failed to open shared memory region: %s\n", strerror(errno));
      _exit(-1);
    }

    __shmem = (struct shmem_data *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (__shmem == MAP_FAILED)
    {
      fprintf(stderr, "Failed to mmap shared memory region\n");
      _exit(-1);
    }
  }

  __sanitizer_cov_reset_edgeguards();

  __shmem->num_edges = (uint32_t)(stop - start);
  printf("[COV] edge counters initialized. Shared memory: %s with %u edges\n", shm_key, __shmem->num_edges);
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
  // There's a small race condition here: if this function executes in two threads for the same
  // edge at the same time, the first thread might disable the edge (by setting the guard to zero)
  // before the second thread fetches the guard value (and thus the index). However, our
  // instrumentation ignores the first edge (see libcoverage.c) and so the race is unproblematic.
  uint32_t index = *guard;
  __shmem->edges[index / 8] |= 1 << (index % 8);
  *guard = 0;
}

// We have to assume that the fuzzer will be able to call this function e.g. by
// enumerating the properties of the global object and eval'ing them. As such
// this function is implemented in a way that requires passing some magic value
// as first argument (with the idea being that the fuzzer won't be able to
// generate this value) which then also acts as a selector for the operation
// to perform.
jerry_value_t
jerryx_handler_fuzzilli(const jerry_value_t func_obj_val, /**< function object */
                        const jerry_value_t this_p,       /**< this arg */
                        const jerry_value_t args_p[],     /**< function arguments */
                        const jerry_length_t args_cnt)    /**< number of function arguments */
{
  (void)func_obj_val; /* unused */
  (void)this_p;       /* unused */

  jerry_char_t operation[256] = {0};
  jerry_value_t ret_val = jerry_create_undefined();

  if (args_cnt > 0 && jerry_value_is_string (args_p[0]))
  {
    jerry_value_t str_val;
    str_val = jerry_value_to_string (args_p[0]);

    if (!jerry_value_is_error (str_val))
    {
      jerry_length_t length = jerry_get_utf8_string_length (str_val);

      if (length > 0 && length < 256)
      {
        jerry_string_to_utf8_char_buffer (str_val, operation, length);
      }
    }

    jerry_release_value (str_val);
  }

  if (strcmp((char *)operation, "FUZZILLI_CRASH") == 0)
  {
    if (args_cnt == 2 && jerry_value_is_number(args_p[1]))
    {
      int arg = (int)jerry_get_number_value(args_p[1]);
      switch (arg)
      {
      case 0:
        *((int *)0x41414141) = 0x1337;
        break;
      default:
        DCHECK(false);
        break;
      }
    }
  }
  else if (strcmp((char *)operation, "FUZZILLI_PRINT") == 0)
  {
    static FILE *fzliout;
    fzliout = fdopen(REPRL_DWFD, "w");
    if (!fzliout)
    {
      fprintf(stderr, "Fuzzer output channel not available, printing to stdout instead\n");
      fzliout = stdout;
    }

    /* Based on the jerryx_handler_print handler */
    const char *const null_str = "\\u0000";

    for (jerry_length_t arg_index = 1; arg_index < args_cnt; arg_index++)
    {
      jerry_value_t str_val;

      if (jerry_value_is_symbol (args_p[arg_index]))
      {
        str_val = jerry_get_symbol_descriptive_string (args_p[arg_index]);
      }
      else
      {
        str_val = jerry_value_to_string (args_p[arg_index]);
      }

      if (jerry_value_is_error (str_val))
      {
        /* There is no need to free the undefined value. */
        ret_val = str_val;
        break;
      }

      jerry_length_t length = jerry_get_utf8_string_length (str_val);
      jerry_length_t substr_pos = 0;
      jerry_char_t substr_buf[256];

      do
      {
        jerry_size_t substr_size = jerry_substring_to_utf8_char_buffer(str_val,
                                                                       substr_pos,
                                                                       length,
                                                                       substr_buf,
                                                                       256 - 1);

        jerry_char_t *buf_end_p = substr_buf + substr_size;

        /* Update start position by the number of utf-8 characters. */
        for (jerry_char_t *buf_p = substr_buf; buf_p < buf_end_p; buf_p++)
        {
          /* Skip intermediate utf-8 octets. */
          if ((*buf_p & 0xc0) != 0x80)
          {
            substr_pos++;
          }
        }

        if (substr_pos == length)
        {
          *buf_end_p++ = (arg_index < args_cnt - 1) ? ' ' : '\n';
        }

        for (jerry_char_t *buf_p = substr_buf; buf_p < buf_end_p; buf_p++)
        {
          char chr = (char)*buf_p;

          if (chr != '\0')
          {
            putc(chr, fzliout);
            continue;
          }

          for (jerry_size_t null_index = 0; null_str[null_index] != '\0'; null_index++)
          {
            putc(null_str[null_index], fzliout);
          }
        }
      } while (substr_pos < length);

      jerry_release_value(str_val);
    }

    if (args_cnt == 0 || jerry_value_is_error(ret_val))
    {
      putc('\n', fzliout);
    }
    fflush(fzliout);
  }
  return ret_val;
}

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function(const char *name_p,                 /**< name of the function */
                     jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t result_val = jerryx_handler_register_global((const jerry_char_t *)name_p, handler_p);

  if (jerry_value_is_error(result_val))
  {
    jerry_port_log(JERRY_LOG_LEVEL_WARNING, "Warning: failed to register '%s' method.", name_p);
    result_val = jerry_get_value_from_error(result_val, true);
  }

  jerry_release_value(result_val);
} /* register_js_function */

int main ()
{
  // Let parent know we are ready
  char helo[] = "HELO";

  if (write(REPRL_CWFD, helo, 4) != 4 ||
      read(REPRL_CRFD, helo, 4) != 4 ||
      memcmp(helo, "HELO", 4) != 0)
  {
    fprintf(stderr, "Invalid response from parent\n");
    _exit(-1);
  }

  while (true) {
    jerry_init(JERRY_INIT_EMPTY);
    // Register a JavaScript function 'fuzzilli' in the global object.
    register_js_function("fuzzilli", jerryx_handler_fuzzilli);

    unsigned action = 0;
    ssize_t nread = read(REPRL_CRFD, &action, 4);
    fflush(0);
    if (nread != 4 || action != 0x63657865) // exec
    {
      fprintf(stderr, "Unknown action: %x\n", action);
      _exit(-1);
    }

    size_t script_size = 0;
    uint8_t *source_buffer_tail = buffer;
    size_t len = 0;

    read(REPRL_CRFD, &script_size, 8);
    source_buffer_tail = buffer;
    ssize_t remaining = (ssize_t)script_size;
    while (remaining > 0)
    {
      ssize_t rv = read(REPRL_DRFD, source_buffer_tail, (size_t)remaining);
      if (rv <= 0)
      {
        fprintf(stderr, "Failed to load script\n");
        _exit(-1);
      }
      remaining -= rv;
      source_buffer_tail += rv;
    }
    buffer[script_size] = 0;
    len = script_size;

    if (len > 0 && jerry_is_valid_utf8_string (buffer, (jerry_size_t)len))
    {
      int status_rc = 0;

      jerry_value_t parse_value = jerry_parse(NULL, 0, (jerry_char_t *)buffer, len, JERRY_PARSE_NO_OPTS);
      jerry_value_t run_value;

      if (!jerry_value_is_error(parse_value))
      {
        run_value = jerry_run(parse_value);

        if (!jerry_value_is_error (run_value))
        {
          jerry_value_t run_queue_value = jerry_run_all_enqueued_jobs ();
          jerry_release_value(run_queue_value);
        }
        else
        {
          status_rc = 1;
        }
        jerry_release_value(run_value);
      }

      jerry_release_value(parse_value);
      jerry_cleanup();

      status_rc <<= 8;
      if (write(REPRL_CWFD, &status_rc, 4) != 4)
        _exit (1);

      __sanitizer_cov_reset_edgeguards ();
    }
  }

  return 0;
}
