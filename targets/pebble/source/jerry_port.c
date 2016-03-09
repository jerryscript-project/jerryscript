#include "jerry-port.h"
#include <stdarg.h>
#include <stdint.h>

#define MEM_HEAP_INTERNAL

#include "mem-heap-internal.h"

void pbl_log(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, ...);

#define LOG_LEVEL_DEBUG 200
#define LOG_LEVEL_ERROR 1

/**
 * Provide log message to filestream implementation for the engine.
 */
int jerry_port_logmsg (FILE* stream, const char* format, ...)
{
  va_list args;
  va_start(args, format);

  pbl_log(LOG_LEVEL_DEBUG, "JERRY-LOG", 0, format, args);

  va_end(args);

  return 0;
}

/**
 * Provide error message to console implementation for the engine.
 */
int jerry_port_errormsg (const char* format, ...)
{
  va_list args;
  va_start(args, format);

  pbl_log(LOG_LEVEL_ERROR, "JERRY-ERROR", 0, format, args);

  va_end(args);

  return 0;
}

void dbgserial_putchar(uint8_t c);

/**
 * Provide output character to console implementation for the engine.
 */
int jerry_port_putchar (int c)
{
  dbgserial_putchar((uint8_t)c);
  return c;
}

void wtf(void);

void jerry_port_abort () {
  wtf();
}

void *task_zalloc(size_t bytes);
void task_free(void *p);
void app_state_set_js_state(void *data);
void *app_state_get_js_state(void);

mem_heap_t *jerry_port_init_heap(void) {
  mem_heap_t *mem_heap = task_zalloc(sizeof(mem_heap_t));
  app_state_set_js_state(mem_heap);
  return mem_heap;
}

void jerry_port_finalize_heap(mem_heap_t *mem_heap) {
  task_free(mem_heap);
}

mem_heap_t *jerry_port_get_heap(void) {
  mem_heap_t *mem_heap = app_state_get_js_state();
  return mem_heap;
}

