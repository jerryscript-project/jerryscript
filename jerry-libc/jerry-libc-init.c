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
 *
 * This file is based on work under the following copyright and permission
 * notice:
 *
 *    Copyright (C) 2004 CodeSourcery, LLC
 *
 *    Permission to use, copy, modify, and distribute this file
 *    for any purpose is hereby granted without fee, provided that
 *    the above copyright notice and this notice appears in all
 *    copies.
 *
 *    This file is distributed WITHOUT ANY WARRANTY; without even the implied
 *    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "jerry-libc-defs.h"

#ifdef ENABLE_INIT_FINI

/* These magic symbols are provided by the linker.  */
typedef void (*libc_init_fn_t) (void);

extern libc_init_fn_t __preinit_array_start[] __attr_weak___;
extern libc_init_fn_t __preinit_array_end[] __attr_weak___;
extern libc_init_fn_t __init_array_start[] __attr_weak___;
extern libc_init_fn_t __init_array_end[] __attr_weak___;
extern libc_init_fn_t __fini_array_start[] __attr_weak___;
extern libc_init_fn_t __fini_array_end[] __attr_weak___;
extern void _init (void);
extern void _fini (void);


/**
 * No-op default _init.
 */
void __attr_weak___
_init (void)
{
} /* _init */

/**
 * No-op default _fini.
 */
void __attr_weak___
_fini (void)
{
} /* _fini */

/**
 * Iterate over all the init routines.
 */
void
libc_init_array (void)
{
  size_t count = (size_t) (__preinit_array_end - __preinit_array_start);
  for (size_t i = 0; i < count; i++)
  {
    __preinit_array_start[i] ();
  }

  _init ();

  count = (size_t) (__init_array_end - __init_array_start);
  for (size_t i = 0; i < count; i++)
  {
    __init_array_start[i] ();
  }
} /* libc_init_array */

/**
 * Run all the cleanup routines.
 */
void
libc_fini_array (void)
{
  size_t count = (size_t) (__fini_array_end - __fini_array_start);
  for (size_t i = count; i > 0; i--)
  {
    __fini_array_start[i - 1] ();
  }

  _fini ();
} /* libc_fini_array */

#endif /* ENABLE_INIT_FINI */
