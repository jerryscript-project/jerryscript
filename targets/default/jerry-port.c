/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#include "jerry-port.h"
#include <stdarg.h>

#define MEM_HEAP_INTERNAL

#include "mem-heap-internal.h"

/**
 * Provide log message to filestream implementation for the engine.
 */
int jerry_port_logmsg (FILE *stream, /**< stream pointer */
                       const char *format, /**< format string */
                       ...) /**< parameters */
{
  va_list args;
  int count;
  va_start (args, format);
  count = vfprintf (stream, format, args);
  va_end (args);
  return count;
} /* jerry_port_logmsg */

/**
 * Provide error message to console implementation for the engine.
 */
int jerry_port_errormsg (const char *format, /**< format string */
                         ...) /**< parameters */
{
  va_list args;
  int count;
  va_start (args, format);
  count = vfprintf (stderr, format, args);
  va_end (args);
  return count;
} /* jerry_port_errormsg */

/**
 * Provide output character to console implementation for the engine.
 */
int jerry_port_putchar (int c) /**< character to put */
{
  return putchar (c);
} /* jerry_port_putchar */

/**
 * Provide abort implementation for the engine
 */
void jerry_port_abort() {
  abort();
} /* jerry_port_abort */

#ifndef JERRY_HEAP_SECTION_ATTR
static mem_heap_t mem_heap;
#else
static mem_heap_t mem_heap __attribute__ ((section (JERRY_HEAP_SECTION_ATTR)));
#endif

mem_heap_t *jerry_port_init_heap(void) {
  return &mem_heap;
}

void jerry_port_finalize_heap(mem_heap_t *mem_heap) {
  return;
}

mem_heap_t *jerry_port_get_heap(void) {
  return &mem_heap;
}

