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

/*
 * Number.prototype built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE)

/* Object properties:
 *  (property name, object pointer getter) */

// 15.7.4.1
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_NUMBER,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_number_prototype_object_to_string, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_VALUE_OF_UL, ecma_builtin_number_prototype_object_value_of, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_STRING_UL, ecma_builtin_number_prototype_object_to_locale_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_FIXED_UL, ecma_builtin_number_prototype_object_to_fixed, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TO_EXPONENTIAL_UL, ecma_builtin_number_prototype_object_to_exponential, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TO_PRECISION_UL, ecma_builtin_number_prototype_object_to_precision, 1, 1)

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
