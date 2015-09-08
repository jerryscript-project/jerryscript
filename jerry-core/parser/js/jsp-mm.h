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

#ifndef JSP_MM_H
#define JSP_MM_H

#include "jrt-libc-includes.h"

/** \addtogroup jsparser ECMAScript parser
 * @{
 *
 * \addtogroup managedmem Managed memory allocation
 * @{
 */

extern void jsp_mm_init (void);
extern void jsp_mm_finalize (void);
extern size_t jsp_mm_recommend_size (size_t);
extern void *jsp_mm_alloc (size_t);
extern void jsp_mm_free (void *);
extern void jsp_mm_free_all (void);

/**
 * @}
 * @}
 */

#endif /* !JSP_MM_H */
