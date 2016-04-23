/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

/**
 * Target port functions for console output
 */
int jerry_port_logmsg (FILE *stream, const char *format, ...);
int jerry_port_errormsg (const char *format, ...);

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
  ERR_UNIMPLEMENTED_CASE = 118,
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
