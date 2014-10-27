/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/**
 * Jerry libc platform-specific functions stm32f4 implementation
 */

#include "jerry-libc.h"

#include <stdarg.h>

extern void __noreturn exit (int status);

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
__putchar (int c)
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS("putchar is not implemented for STM32F3.", c);
} /* __putchar */

/** exit - cause normal process termination  */
void __noreturn
__exit (int status __unused)
{
  /**
   * TODO: Blink LEDs? status -> binary -> LEDs?
   */

  while (true)
  {
  }
} /* __exit */

/**
 * fwrite
 *
 * @return number of bytes written
 */
size_t
__fwrite (const void *ptr, /**< data to write */
         size_t size, /**< size of elements to write */
         size_t nmemb, /**< number of elements */
         _FILE *stream) /**< stream pointer */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS("fwrite is not implemented for STM32F3.", ptr, size, nmemb, stream);
} /* __fwrite */

