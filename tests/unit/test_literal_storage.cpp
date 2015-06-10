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

#include "ecma-helpers.h"
#include "lit-literal.h"

extern "C"
{
  extern void srand (unsigned int __seed);
  extern int rand (void);
  extern long int time (long int *__timer);
  extern int printf (__const char *__restrict __format, ...);
  extern void *memset (void *__s, int __c, size_t __n);
}

// Heap size is 32K
#define test_heap_size (32 * 1024)

// Iterations count
#define test_iters 64

// Subiterations count
#define test_sub_iters 64

// Max characters in a string
#define max_characters_in_string 256

static void
generate_string (ecma_char_t *str, ecma_length_t len)
{
  static const ecma_char_t characters[] = "!@#$%^&*()_+abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
  static const ecma_length_t length = (ecma_length_t) (sizeof (characters) / sizeof (ecma_char_t) - 1);
  for (ecma_length_t i = 0; i < len; ++i)
  {
    str[i] = characters[(unsigned long) rand () % length];
  }
}

static ecma_number_t
generate_number ()
{
  ecma_number_t num = ((ecma_number_t) rand () / 32767.0);
  if (rand () % 2)
  {
    num = -num;
  }
  int power = rand () % 30;
  while (power-- > 0)
  {
    num *= 10;
  }
  return num;
}

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  const ecma_char_t *ptrs[test_sub_iters];
  ecma_number_t numbers[test_sub_iters];
  ecma_char_t strings[test_sub_iters][max_characters_in_string + 1];
  ecma_length_t lengths[test_sub_iters];

  mem_init ();
  lit_init ();

  srand ((unsigned int) time (NULL));
  int k = rand ();
  printf ("seed=%d\n", k);
  srand ((unsigned int) k);

  for (uint32_t i = 0; i < test_iters; i++)
  {
    memset (numbers, 0, sizeof (ecma_number_t) * test_sub_iters);
    memset (lengths, 0, sizeof (ecma_length_t) * test_sub_iters);
    memset (ptrs, 0, sizeof (ecma_char_t *) * test_sub_iters);

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      int type = rand () % 3;
      if (type == 0)
      {
        lengths[j] = (ecma_length_t) (rand () % max_characters_in_string + 1);
        generate_string (strings[j], lengths[j]);
        lit_create_literal_from_charset (strings[j], lengths[j]);
        strings[j][lengths[j]] = '\0';
        ptrs[j] = strings[j];
        JERRY_ASSERT (ptrs[j]);
      }
      else if (type == 1)
      {
        ecma_magic_string_id_t msi = (ecma_magic_string_id_t) (rand () % ECMA_MAGIC_STRING__COUNT);
        ptrs[j] = ecma_get_magic_string_zt (msi);
        JERRY_ASSERT (ptrs[j]);
        lengths[j] = (ecma_length_t) ecma_zt_string_length (ptrs[j]);
        lit_create_literal_from_charset (ptrs[j], lengths[j]);
      }
      else
      {
        ecma_number_t num = generate_number ();
        lengths[j] = ecma_number_to_zt_string (num, strings[j], max_characters_in_string);
        lit_create_literal_from_num (num);
      }
    }

    // Add empty string
    lit_create_literal_from_charset (NULL, 0);

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      literal_t lit1;
      literal_t lit2;
      if (ptrs[j])
      {
        lit1 = lit_find_or_create_literal_from_charset (ptrs[j], lengths[j]);
        lit2 = lit_find_literal_by_charset (ptrs[j], lengths[j]);
        JERRY_ASSERT (lit_literal_equal_zt (lit1, ptrs[j]));
        JERRY_ASSERT (lit_literal_equal_type_zt (lit2, ptrs[j]));
      }
      else
      {
        lit1 = lit_find_or_create_literal_from_num (numbers[j]);
        lit2 = lit_find_literal_by_num (numbers[j]);
        JERRY_ASSERT (lit_literal_equal_num (lit1, numbers[j]));
        JERRY_ASSERT (lit_literal_equal_type_num (lit2, numbers[j]));
      }
      JERRY_ASSERT (lit1);
      JERRY_ASSERT (lit2);
      JERRY_ASSERT (lit1 == lit2);
      JERRY_ASSERT (lit_literal_equal (lit1, lit2));
    }

    // Check empty string exists
    JERRY_ASSERT (lit_find_literal_by_charset (NULL, 0));

    lit_storage.cleanup ();
    JERRY_ASSERT (lit_storage.get_first () == NULL);
  }

  lit_finalize ();
  mem_finalize (true);
  return 0;
}
