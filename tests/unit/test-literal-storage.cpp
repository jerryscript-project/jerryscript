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
#include "lit-literal-storage.h"
#include "rcs-records.h"
#include "test-common.h"

// Iterations count
#define test_iters 64

// Subiterations count
#define test_sub_iters 64

// Max characters in a string
#define max_characters_in_string 256

static void
generate_string (lit_utf8_byte_t *str, lit_utf8_size_t len)
{
  static const lit_utf8_byte_t bytes[] = "!@#$%^&*()_+abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
  static const lit_utf8_size_t length = (lit_utf8_size_t) (sizeof (bytes) - 1);
  for (lit_utf8_size_t i = 0; i < len; ++i)
  {
    str[i] = bytes[(unsigned long) rand () % length];
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
  TEST_INIT ();

  const lit_utf8_byte_t *ptrs[test_sub_iters];
  ecma_number_t numbers[test_sub_iters];
  lit_utf8_byte_t strings[test_sub_iters][max_characters_in_string + 1];
  lit_utf8_size_t lengths[test_sub_iters];

  mem_init ();
  lit_init ();

  for (uint32_t i = 0; i < test_iters; i++)
  {
    memset (numbers, 0, sizeof (ecma_number_t) * test_sub_iters);
    memset (lengths, 0, sizeof (lit_utf8_size_t) * test_sub_iters);
    memset (ptrs, 0, sizeof (lit_utf8_byte_t *) * test_sub_iters);

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      int type = rand () % 3;
      if (type == 0)
      {
        lengths[j] = (lit_utf8_size_t) (rand () % max_characters_in_string + 1);
        generate_string (strings[j], lengths[j]);
        lit_create_literal_from_utf8_string (strings[j], lengths[j]);
        strings[j][lengths[j]] = '\0';
        ptrs[j] = strings[j];
        JERRY_ASSERT (ptrs[j]);
      }
      else if (type == 1)
      {
        lit_magic_string_id_t msi = (lit_magic_string_id_t) (rand () % LIT_MAGIC_STRING__COUNT);
        ptrs[j] = lit_get_magic_string_utf8 (msi);
        JERRY_ASSERT (ptrs[j]);
        lengths[j] = (lit_utf8_size_t) lit_zt_utf8_string_size (ptrs[j]);
        lit_create_literal_from_utf8_string (ptrs[j], lengths[j]);
      }
      else
      {
        ecma_number_t num = generate_number ();
        lengths[j] = ecma_number_to_utf8_string (num, strings[j], max_characters_in_string);
        lit_create_literal_from_num (num);
      }
    }

    // Add empty string
    lit_create_literal_from_utf8_string (NULL, 0);

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      lit_literal_t lit1;
      lit_literal_t lit2;
      if (ptrs[j])
      {
        lit1 = lit_find_or_create_literal_from_utf8_string (ptrs[j], lengths[j]);
        lit2 = lit_find_literal_by_utf8_string (ptrs[j], lengths[j]);
        JERRY_ASSERT (lit_literal_equal_utf8 (lit1, ptrs[j], lengths[j]));
        JERRY_ASSERT (lit_literal_equal_type_utf8 (lit2, ptrs[j], lengths[j]));
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
    JERRY_ASSERT (lit_find_literal_by_utf8_string (NULL, 0));

    rcs_lit_storage.cleanup ();
    JERRY_ASSERT (rcs_record_get_first (&rcs_lit_storage) == NULL);
  }

  lit_finalize ();
  mem_finalize (true);
  return 0;
}
