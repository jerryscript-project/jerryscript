/* Copyright JS Foundation and other contributors, http://js.foundation
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

#ifndef ECMA_BIG_INT_H
#define ECMA_BIG_INT_H

#include "ecma-globals.h"

#if ENABLED (JERRY_BUILTIN_BIGINT)

/**
 * Sign bit of a BigInt value. The number is negative, if this bit is set.
 */
#define ECMA_BIGINT_SIGN 0x1

ecma_value_t ecma_bigint_parse_string (const lit_utf8_byte_t *string_p, lit_utf8_size_t size,
                                       bool throw_syntax_error);
ecma_value_t ecma_bigint_parse_string_value (ecma_value_t string, bool throw_syntax_error);
ecma_string_t *ecma_bigint_to_string (ecma_value_t value, ecma_bigint_digit_t radix);
ecma_value_t ecma_bigint_number_to_bigint (ecma_number_t number);
ecma_value_t ecma_bigint_to_bigint (ecma_value_t value);

bool ecma_bigint_is_equal_to_bigint (ecma_value_t left_value, ecma_value_t right_value);
bool ecma_bigint_is_equal_to_number (ecma_value_t left_value, ecma_number_t right_value);
int ecma_bigint_compare_to_bigint (ecma_value_t left_value, ecma_value_t right_value);
int ecma_bigint_compare_to_number (ecma_value_t left_value, ecma_number_t right_value);

ecma_value_t ecma_bigint_negate (ecma_extended_primitive_t *value_p);
ecma_value_t ecma_bigint_add_sub (ecma_value_t left_value, ecma_value_t right_value, bool is_add);
ecma_value_t ecma_bigint_mul (ecma_value_t left_value, ecma_value_t right_value);
ecma_value_t ecma_bigint_div_mod (ecma_value_t left_value, ecma_value_t right_value, bool is_mod);
ecma_value_t ecma_bigint_shift (ecma_value_t left_value, ecma_value_t right_value, bool is_left);

#endif /* ENABLED (JERRY_BUILTIN_BIGINT) */

#endif /* ECMA_BIG_INT_H */
