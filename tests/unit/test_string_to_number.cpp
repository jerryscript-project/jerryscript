/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "jrt.h"
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
    (const ecma_char_t*) "1e-45",
    (const ecma_char_t*) "-2.5e+38",
    (const ecma_char_t*) "-2.5e38",
    (const ecma_char_t*) "- 2.5e+38",
    (const ecma_char_t*) "-2 .5e+38",
    (const ecma_char_t*) "-2. 5e+38",
    (const ecma_char_t*) "-2.5e+ 38",
    (const ecma_char_t*) "-2.5 e+38",
    (const ecma_char_t*) "-2.5e +38",
    (const ecma_char_t*) "NaN",
    (const ecma_char_t*) "abc",
    (const ecma_char_t*) "   Infinity  ",
    (const ecma_char_t*) "-Infinity",
    (const ecma_char_t*) "0",
    (const ecma_char_t*) "0",
  };

  const ecma_number_t nums[] =
  {
    1.0,
    0.5,
    12345.0,
    1.0e-45,
    -2.5e+38,
    -2.5e+38,
    NAN,
    NAN,
    NAN,
    NAN,
    NAN,
    NAN,
    NAN,
    NAN,
    INFINITY,
    -INFINITY,
    +0.0,
    -0.0
  };

  for (uint32_t i = 0;
       i < sizeof (nums) / sizeof (nums[0]);
       i++)
  {
    ecma_number_t num = ecma_zt_string_to_number (zt_strings[i]);

    if (num != nums[i]
        && (!ecma_number_is_nan (num)
            || !ecma_number_is_nan (nums[i])))
    {
      return 1;
    }
  }

  return 0;
} /* main */
