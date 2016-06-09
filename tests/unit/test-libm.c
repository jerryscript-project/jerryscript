/* Copyright 2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
 * Unit test for jerry-libm
 */

#include "test-common.h"

static bool passed = true;

static void
check_int (const char *expr, int computed, int expected)
{
  printf ("%s = %d [expected=%d] ", expr, computed, expected);

  bool result = computed == expected;
  printf ("%s\n", result ? "PASS" : "FAIL");

  passed &= result;
} /* check_int */

static void
check_double (const char *expr, double computed, double expected)
{
  uint64_t *computed_bits_p = (uint64_t *) &computed;
  uint64_t *expected_bits_p = (uint64_t *) &expected;

  printf ("%s = 0x%lx [expected=0x%lx] ", expr, *computed_bits_p, *expected_bits_p);

  bool result;
  if (isnan (computed) && isnan (expected))
  {
    result = true;
  }
  else
  {
    int64_t diff = (int64_t) (*computed_bits_p - *expected_bits_p);
    if (diff < 0)
    {
      diff = -diff;
    }

    if (diff <= 1) /* tolerate 1 bit of differene in the last place */
    {
      result = true;
      if (diff != 0)
      {
        printf ("APPROX ");
      }
    }
    else
    {
      result = false;
    }
  }
  printf ("%s\n", result ? "PASS" : "FAIL");

  passed &= result;
} /* check_double */

int
main ()
{
#define INF INFINITY
#include "test-libm.inc.h"

  return passed ? 0 : 1;
} /* main */
