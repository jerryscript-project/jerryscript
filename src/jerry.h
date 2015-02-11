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

#ifndef JERRY_H
#define JERRY_H

#include "jrt_types.h"

/** \addtogroup jerry Jerry engine interface
 * @{
 */

extern bool
jerry_run (const char *script_source,
           size_t script_source_size,
           bool is_parse_only,
           bool is_show_opcodes,
           bool is_show_mem_stats);

/**
 * @}
 */

#endif /* !JERRY_H */
