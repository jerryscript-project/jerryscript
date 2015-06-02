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

/**
 * Jerry libc platform-specific functions stm32f4 implementation
 */

#include <stdio.h>
#include <stdlib.h>

#include "jerry-libc-defs.h"

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
putchar (int c __attr_unused___)
{
  return 1;
} /* putchar */

/** exit - cause normal process termination  */
void __attr_noreturn___ __attr_used___
exit (int status __attr_unused___)
{
  while (true)
  {
  }
} /* exit */

/** abort - cause abnormal process termination  */
void __attr_noreturn___ __attr_used___
abort (void)
{
  while (true)
  {
  }
} /* abort */

/**
 * fwrite
 *
 * @return number of bytes written
 */
size_t
fwrite (const void *ptr __attr_unused___, /**< data to write */
        size_t size, /**< size of elements to write */
        size_t nmemb, /**< number of elements */
        FILE *stream __attr_unused___) /**< stream pointer */
{
  return size * nmemb;
} /* fwrite */

