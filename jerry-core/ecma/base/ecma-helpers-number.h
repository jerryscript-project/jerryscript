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

#ifndef ECMA_HELPERS_NUMBER_H
#define ECMA_HELPERS_NUMBER_H

#include "ecma-globals.h"

#include "config.h"

/**
 * Binary representation of an ecma-number
 */
#if JERRY_NUMBER_TYPE_FLOAT64
typedef uint64_t ecma_binary_num_t;
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
typedef uint32_t ecma_binary_num_t;
#endif /* !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Makes it possible to read/write the binary representation of an ecma_number_t
 * without strict aliasing rule violation.
 */
typedef union
{
  ecma_number_t as_number; /**< ecma-number */
  ecma_binary_num_t as_binary; /**< binary representation */
} ecma_number_accessor_t;

ecma_binary_num_t ecma_number_to_binary (ecma_number_t number);
ecma_number_t ecma_number_from_binary (ecma_binary_num_t binary);

bool ecma_number_sign (ecma_binary_num_t binary);
uint32_t ecma_number_biased_exp (ecma_binary_num_t binary);
uint64_t ecma_number_fraction (ecma_binary_num_t binary);
ecma_number_t ecma_number_create (bool sign, uint32_t biased_exp, uint64_t fraction);

/**
 * Maximum number of significant decimal digits that an ecma-number can store
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_MAX_DIGITS (19)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_MAX_DIGITS (9)
#endif /* !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Width of sign field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_SIGN_WIDTH (1)

/**
 * Width of biased exponent field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_BIASED_EXP_WIDTH (11)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_BIASED_EXP_WIDTH (8)
#endif /* !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Exponent bias
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_EXPONENT_BIAS (1023)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_EXPONENT_BIAS (127)
#endif /* !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Width of fraction field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_FRACTION_WIDTH (52)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_FRACTION_WIDTH (23)
#endif /* !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Sign bit in ecma-numbers
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_SIGN_BIT 0x8000000000000000ull
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_SIGN_BIT 0x7f800000u;
#endif /* !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Binary representation of an IEEE-754 QNaN value.
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_BINARY_QNAN 0x7ff8000000000000ull
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_BINARY_QNAN 0x7fc00000u
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Binary representation of an IEEE-754 Infinity value.
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_BINARY_INF 0x7ff0000000000000ull
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_BINARY_INF 0x7f800000u
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Binary representation of an IEEE-754 zero value.
 */
#define ECMA_NUMBER_BINARY_ZERO 0x0ull

/**
 * Number.MIN_VALUE (i.e., the smallest positive value of ecma-number)
 *
 * See also: ECMA_262 v5, 15.7.3.3
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_MIN_VALUE ((ecma_number_t) 5e-324)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_MIN_VALUE (FLT_MIN)
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Number.MAX_VALUE (i.e., the maximum value of ecma-number)
 *
 * See also: ECMA_262 v5, 15.7.3.2
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_MAX_VALUE ((ecma_number_t) 1.7976931348623157e+308)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_MAX_VALUE (FLT_MAX)
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Number.EPSILON
 *
 * See also: ECMA_262 v6, 20.1.2.1
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_EPSILON ((ecma_number_t) 2.2204460492503130808472633361816e-16)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_EPSILON ((ecma_number_t) 1.1920928955078125e-7)
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Number.MAX_SAFE_INTEGER
 *
 * See also: ECMA_262 v6, 20.1.2.6
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_MAX_SAFE_INTEGER ((ecma_number_t) 0x1FFFFFFFFFFFFF)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_MAX_SAFE_INTEGER ((ecma_number_t) 0xFFFFFF)
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Number.MIN_SAFE_INTEGER
 *
 * See also: ECMA_262 v6, 20.1.2.8
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define ECMA_NUMBER_MIN_SAFE_INTEGER ((ecma_number_t) -0x1FFFFFFFFFFFFF)
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define ECMA_NUMBER_MIN_SAFE_INTEGER ((ecma_number_t) -0xFFFFFF)
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Number.MAX_VALUE exponent part
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define NUMBER_MAX_DECIMAL_EXPONENT 308
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define NUMBER_MAX_DECIMAL_EXPONENT 38
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Number.MIN_VALUE exponent part
 */
#if JERRY_NUMBER_TYPE_FLOAT64
#define NUMBER_MIN_DECIMAL_EXPONENT -324
#else /* !JERRY_NUMBER_TYPE_FLOAT64 */
#define NUMBER_MIN_DECIMAL_EXPONENT -45
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Euler number
 */
#define ECMA_NUMBER_E ((ecma_number_t) 2.7182818284590452354)

/**
 * Natural logarithm of 10
 */
#define ECMA_NUMBER_LN10 ((ecma_number_t) 2.302585092994046)

/**
 * Natural logarithm of 2
 */
#define ECMA_NUMBER_LN2 ((ecma_number_t) 0.6931471805599453)

/**
 * Logarithm base 2 of the Euler number
 */
#define ECMA_NUMBER_LOG2E ((ecma_number_t) 1.4426950408889634)

/**
 * Logarithm base 10 of the Euler number
 */
#define ECMA_NUMBER_LOG10E ((ecma_number_t) 0.4342944819032518)

/**
 * Pi number
 */
#define ECMA_NUMBER_PI ((ecma_number_t) 3.1415926535897932)

/**
 * Square root of 0.5
 */
#define ECMA_NUMBER_SQRT_1_2 ((ecma_number_t) 0.7071067811865476)

/**
 * Square root of 2
 */
#define ECMA_NUMBER_SQRT2 ((ecma_number_t) 1.4142135623730951)

#endif /* !ECMA_HELPERS_NUMBER_H */
