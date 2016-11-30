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

#ifndef JERRY_PORT_H
#define JERRY_PORT_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \addtogroup jerry_port Jerry engine port
 * @{
 */

/*
 * Termination Port API
 *
 * Note:
 *      It is questionable whether a library should be able to terminate an
 *      application. However, as of now, we only have the concept of completion
 *      code around jerry_parse and jerry_run. Most of the other API functions
 *      have no way of signaling an error. So, we keep the termination approach
 *      with this port function.
 */

/**
 * Error codes
 */
typedef enum
{
  ERR_OUT_OF_MEMORY = 10,
  ERR_SYSCALL = 11,
  ERR_REF_COUNT_LIMIT = 12,
  ERR_FAILED_INTERNAL_ASSERTION = 120
} jerry_fatal_code_t;

/**
 * Signal the port that jerry experienced a fatal failure from which it cannot
 * recover.
 *
 * @param code gives the cause of the error.
 *
 * Note:
 *      Jerry expects the function not to return.
 *
 * Example: a libc-based port may implement this with exit() or abort(), or both.
 */
void jerry_port_fatal (jerry_fatal_code_t code);

/*
 *  I/O Port API
 */

/**
 * Print a string to the console. The function should implement a printf-like
 * interface, where the first argument specifies a format string on how to
 * stringify the rest of the parameter list.
 *
 * This function is only called with strings coming from the executed ECMAScript
 * wanting to print something as the result of its normal operation.
 *
 * It should be the port that decides what a "console" is.
 *
 * Example: a libc-based port may implement this with vprintf().
 */
void jerry_port_console (const char *format, ...) __attribute__ ((format (printf, 1, 2)));

/**
 * Jerry log levels. The levels are in severity order
 * where the most serious levels come first.
 */
typedef enum
{
  JERRY_LOG_LEVEL_ERROR,    /**< the engine will terminate after the message is printed */
  JERRY_LOG_LEVEL_WARNING,  /**< a request is aborted, but the engine continues its operation */
  JERRY_LOG_LEVEL_DEBUG,    /**< debug messages from the engine, low volume */
  JERRY_LOG_LEVEL_TRACE     /**< detailed info about engine internals, potentially high volume */
} jerry_log_level_t;

/**
 * Display or log a debug/error message. The function should implement a printf-like
 * interface, where the first argument specifies the log level
 * and the second argument specifies a format string on how to stringify the rest
 * of the parameter list.
 *
 * This function is only called with messages coming from the jerry engine as
 * the result of some abnormal operation or describing its internal operations
 * (e.g., data structure dumps or tracing info).
 *
 * It should be the port that decides whether error and debug messages are logged to
 * the console, or saved to a database or to a file.
 *
 * Example: a libc-based port may implement this with vfprintf(stderr) or
 * vfprintf(logfile), or both, depending on log level.
 */
void jerry_port_log (jerry_log_level_t level, const char *format, ...) __attribute__ ((format (printf, 2, 3)));

/*
 * Date Port API
 */

/**
 * Jerry time zone structure
 */
typedef struct
{
  int offset;                /**< minutes from west */
  int daylight_saving_time;  /**< daylight saving time (1 - DST applies, 0 - not on DST) */
} jerry_time_zone_t;

/**
 * Get timezone and daylight saving data
 *
 * @return true  - if success
 *         false - otherwise
 */
bool jerry_port_get_time_zone (jerry_time_zone_t *tz_p);

/**
 * Get system time
 *
 * @return milliseconds since Unix epoch
 */
double jerry_port_get_current_time (void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_PORT_H */
