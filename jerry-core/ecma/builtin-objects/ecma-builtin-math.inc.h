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
ROUTINE (LIT_MAGIC_STRING_ABS, ECMA_MATH_OBJECT_ABS, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ACOS, ECMA_MATH_OBJECT_ACOS, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ASIN, ECMA_MATH_OBJECT_ASIN, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ATAN, ECMA_MATH_OBJECT_ATAN, 1, 1)
ROUTINE (LIT_MAGIC_STRING_ATAN2, ECMA_MATH_OBJECT_ATAN2, 2, 2)
ROUTINE (LIT_MAGIC_STRING_CEIL, ECMA_MATH_OBJECT_CEIL, 1, 1)
ROUTINE (LIT_MAGIC_STRING_COS, ECMA_MATH_OBJECT_COS, 1, 1)
ROUTINE (LIT_MAGIC_STRING_EXP, ECMA_MATH_OBJECT_EXP, 1, 1)
ROUTINE (LIT_MAGIC_STRING_FLOOR, ECMA_MATH_OBJECT_FLOOR, 1, 1)
ROUTINE (LIT_MAGIC_STRING_LOG, ECMA_MATH_OBJECT_LOG, 1, 1)
ROUTINE (LIT_MAGIC_STRING_MAX, ECMA_MATH_OBJECT_MAX, NON_FIXED, 2)
ROUTINE (LIT_MAGIC_STRING_MIN, ECMA_MATH_OBJECT_MIN, NON_FIXED, 2)
ROUTINE (LIT_MAGIC_STRING_POW, ECMA_MATH_OBJECT_POW, 2, 2)
ROUTINE (LIT_MAGIC_STRING_RANDOM, ECMA_MATH_OBJECT_RANDOM, 0, 0)
ROUTINE (LIT_MAGIC_STRING_ROUND, ECMA_MATH_OBJECT_ROUND, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SIN, ECMA_MATH_OBJECT_SIN, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SQRT, ECMA_MATH_OBJECT_SQRT, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TAN, ECMA_MATH_OBJECT_TAN, 1, 1)

#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
