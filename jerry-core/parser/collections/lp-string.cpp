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

#include "lp-string.h"
#include "jrt-libc-includes.h"

bool
lp_string_equal (lp_string s1, lp_string s2)
{
  if (s1.length != s2.length)
  {
    return false;
  }

  for (ecma_length_t i = 0; i < s1.length; i++)
  {
    JERRY_ASSERT (s1.str[i] != '\0' && s1.str[i] != '\0');
    if (s1.str[i] != s2.str[i])
    {
      return false;
    }
  }

  return true;
}

bool
lp_string_equal_s (lp_string lp, const char *s)
{
  return lp_string_equal_zt (lp, (const ecma_char_t *) s);
}

bool
lp_string_equal_zt (lp_string lp, const ecma_char_t *s)
{
  for (ecma_length_t i = 0; i < lp.length; i++)
  {
    JERRY_ASSERT (lp.str[i] != '\0');
    if (lp.str[i] != s[i])
    {
      return false;
    }
  }

  if (s[lp.length] != '\0')
  {
    return false;
  }

  return true;
}
