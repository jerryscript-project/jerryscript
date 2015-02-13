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

#ifndef JRT_LIBC_INCLUDES_H
#define JRT_LIBC_INCLUDES_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Ensuring no macro implementation of functions declared in the headers are used */
#undef isspace
#undef isalpha
#undef islower
#undef isupper
#undef isdigit
#undef isxdigit
#undef memset
#undef memcmp
#undef memcpy
#undef memmove
#undef strcmp
#undef strncmp
#undef strncpy
#undef strlen
#undef putchar
#undef puts
#undef exit
#undef fopen
#undef rewind
#undef fclose
#undef fseek
#undef ftell
#undef fread
#undef fwrite
#undef vfprintf
#undef fprintf
#undef printf

#endif /* !JRT_LIBC_INCLUDES_H */
