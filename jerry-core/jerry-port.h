/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include <stdio.h>

#ifdef __cplusplus
# define EXTERN_C "C"
#else /* !__cplusplus */
# define EXTERN_C
#endif /* !__cplusplus */

/** \addtogroup jerry Jerry engine port
 * @{
 */

/**
 * Target port functions for console output
 */
extern EXTERN_C
int jerry_port_logmsg (FILE* stream, const char* format, ...);

extern EXTERN_C
int jerry_port_errormsg (const char* format, ...);

extern EXTERN_C
int jerry_port_putchar (int c);

/**
 * @}
 */

#endif /* !JERRY_API_H */
