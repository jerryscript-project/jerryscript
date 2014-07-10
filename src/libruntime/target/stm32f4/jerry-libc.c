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

extern void __noreturn exit(int status);

/**
 * printf
 * 
 * @return number of characters printed
 */
int
__printf(const char *format, /**< format string */
         ...)                /**< parameters' values */
{
    va_list args;
    
    va_start( args, format);
    
    int ret = vprintf( format, args);
            
    va_end( args);
    
    return ret;
} /* __printf */

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
__putchar (int c)
{
  return __printf ("%c", c);
} /* __putchar */

/** exit - cause normal process termination  */
void __noreturn
__exit (int status)
{
  /**
   * TODO: Blink LEDs? status -> binary -> LEDs?
   */

  while(true);
} /* __exit */
