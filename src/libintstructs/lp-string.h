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

#ifndef LP_STRING
#define LP_STRING

#include "ecma-globals.h"

/* Length-prefixed or "pascal" string.  */
typedef struct
{
  ecma_char_t *str;
  ecma_length_t length;
}
lp_string;

bool lp_string_equal (lp_string, lp_string);
bool lp_string_equal_s (lp_string, const char *);
bool lp_string_equal_zt (lp_string, const ecma_char_t *);

#endif /* LP_STRING */
