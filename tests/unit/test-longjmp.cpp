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

#include "test-common.h"

#define TEST_MAX_DEPTH 10
#define TEST_ITERATIONS_NUM 256

jmp_buf buffers[TEST_MAX_DEPTH];

static void
test_setjmp_longjmp (volatile int depth)
{
  if (depth != TEST_MAX_DEPTH)
  {
    int a = 1, b = 2, c = 3;

    int array[256];
    for (int i = 0; i < 256; i++)
    {
      array[i] = i;
    }

    (void) a;
    (void) b;
    (void) c;
    (void) array;

    int k = setjmp (buffers[depth]);

    if (k == 0)
    {
      test_setjmp_longjmp (depth + 1);
    }
    else
    {
      JERRY_ASSERT (k == depth + 1);

      JERRY_ASSERT (a == 1);
      JERRY_ASSERT (b == 2);
      JERRY_ASSERT (c == 3);

      for (int i = 0; i < 256; i++)
      {
        JERRY_ASSERT (array[i] == i);
      }
    }
  }
  else
  {
    int t = rand () % depth;
    JERRY_ASSERT (t >= 0 && t < depth);

    longjmp (buffers[t], t + 1);
  }
}

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  for (int i = 0; i < TEST_ITERATIONS_NUM; i++)
  {
    test_setjmp_longjmp (0);
  }

  return 0;
} /* main */
