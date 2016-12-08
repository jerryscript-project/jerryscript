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

/*
 * Math built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef SIMPLE_VALUE
# define SIMPLE_VALUE(name, simple_value, prop_attributes)
#endif /* !SIMPLE_VALUE */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_MATH)

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

/* ECMA-262 v5, 15.8.1.1 */
NUMBER_VALUE (LIT_MAGIC_STRING_E_U,
              ECMA_BUILTIN_NUMBER_E,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.2 */
NUMBER_VALUE (LIT_MAGIC_STRING_LN10_U,
              ECMA_BUILTIN_NUMBER_LN10,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.3 */
NUMBER_VALUE (LIT_MAGIC_STRING_LN2_U,
              ECMA_BUILTIN_NUMBER_LN2,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.4 */
NUMBER_VALUE (LIT_MAGIC_STRING_LOG2E_U,
              ECMA_BUILTIN_NUMBER_LOG2E,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.5 */
NUMBER_VALUE (LIT_MAGIC_STRING_LOG10E_U,
              ECMA_BUILTIN_NUMBER_LOG10E,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.6 */
NUMBER_VALUE (LIT_MAGIC_STRING_PI_U,
              ECMA_BUILTIN_NUMBER_PI,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.7 */
NUMBER_VALUE (LIT_MAGIC_STRING_SQRT1_2_U,
              ECMA_BUILTIN_NUMBER_SQRT_1_2,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.8.1.8 */
NUMBER_VALUE (LIT_MAGIC_STRING_SQRT2_U,
              ECMA_BUILTIN_NUMBER_SQRT2,
              ECMA_PROPERTY_FIXED)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_ABS, ecma_builtin_math_object_abs, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ACOS, ecma_builtin_math_object_acos, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ASIN, ecma_builtin_math_object_asin, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ATAN, ecma_builtin_math_object_atan, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ATAN2, ecma_builtin_math_object_atan2, 2, 2)
ROUTINE (LIT_MAGIC_STRING_CEIL, ecma_builtin_math_object_ceil, 1, 1)
ROUTINE (LIT_MAGIC_STRING_COS, ecma_builtin_math_object_cos, 1, 1)
ROUTINE (LIT_MAGIC_STRING_EXP, ecma_builtin_math_object_exp, 1, 1)
ROUTINE (LIT_MAGIC_STRING_FLOOR, ecma_builtin_math_object_floor, 1, 1)
ROUTINE (LIT_MAGIC_STRING_LOG, ecma_builtin_math_object_log, 1, 1)
ROUTINE (LIT_MAGIC_STRING_MAX, ecma_builtin_math_object_max, NON_FIXED, 2)
ROUTINE (LIT_MAGIC_STRING_MIN, ecma_builtin_math_object_min, NON_FIXED, 2)
ROUTINE (LIT_MAGIC_STRING_POW, ecma_builtin_math_object_pow, 2, 2)
ROUTINE (LIT_MAGIC_STRING_RANDOM, ecma_builtin_math_object_random, 0, 0)
ROUTINE (LIT_MAGIC_STRING_ROUND, ecma_builtin_math_object_round, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SIN, ecma_builtin_math_object_sin, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SQRT, ecma_builtin_math_object_sqrt, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TAN, ecma_builtin_math_object_tan, 1, 1)

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
