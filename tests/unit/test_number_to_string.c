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

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"
#include "jerry-libc.h"

#include <math.h>

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  const ecma_char_t* zt_strings[] =
  {
    (const ecma_char_t*) "1",
    (const ecma_char_t*) "0.5",
    (const ecma_char_t*) "12345",
    (const ecma_char_t*) "12345.12209",
    (const ecma_char_t*) "1.401298403e-45",
    (const ecma_char_t*) "-2.5e+38",
    (const ecma_char_t*) "NaN",
    (const ecma_char_t*) "Infinity",
    (const ecma_char_t*) "-Infinity",
    (const ecma_char_t*) "0",
    (const ecma_char_t*) "0",
  };

  const ecma_number_t nums[] =
  {
    1.0f,
    0.5f,
    12345.0f,
    12345.123f,
    1.0e-45f,
    -2.5e+38f,
    NAN,
    INFINITY,
    -INFINITY,
    +0.0f,
    -0.0f
  };

  for (uint32_t i = 0;
       i < sizeof (nums) / sizeof (nums[0]);
       i++)
  {
    ecma_char_t zt_str[64];

    ecma_number_to_zt_string (nums[i], zt_str, sizeof (zt_str));

    if (__strcmp ((char*)zt_str, (char*)zt_strings [i]) != 0)
    {
      return 1;
    }
  }

  return 0;
} /* main */
