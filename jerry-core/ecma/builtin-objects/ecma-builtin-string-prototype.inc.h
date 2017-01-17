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
 * String.prototype built-in description
 */
#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.5.4.1 */
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_STRING,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Number properties:
 *  (property name, number value) */

/* ECMA-262 v5, 15.5.4 (String.prototype is itself a String object whose value is an empty String), 15.5.5.1 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              0,
              ECMA_PROPERTY_FIXED)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_string_prototype_object_to_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_VALUE_OF_UL, ecma_builtin_string_prototype_object_value_of, 0, 0)
ROUTINE (LIT_MAGIC_STRING_CONCAT, ecma_builtin_string_prototype_object_concat, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_SLICE, ecma_builtin_string_prototype_object_slice, 2, 2)
ROUTINE (LIT_MAGIC_STRING_INDEX_OF_UL, ecma_builtin_string_prototype_object_index_of, 2, 1)
ROUTINE (LIT_MAGIC_STRING_LAST_INDEX_OF_UL, ecma_builtin_string_prototype_object_last_index_of, 2, 1)
ROUTINE (LIT_MAGIC_STRING_CHAR_AT_UL, ecma_builtin_string_prototype_object_char_at, 1, 1)
ROUTINE (LIT_MAGIC_STRING_CHAR_CODE_AT_UL, ecma_builtin_string_prototype_object_char_code_at, 1, 1)
ROUTINE (LIT_MAGIC_STRING_LOCALE_COMPARE_UL, ecma_builtin_string_prototype_object_locale_compare, 1, 1)

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
ROUTINE (LIT_MAGIC_STRING_MATCH, ecma_builtin_string_prototype_object_match, 1, 1)
ROUTINE (LIT_MAGIC_STRING_REPLACE, ecma_builtin_string_prototype_object_replace, 2, 2)
ROUTINE (LIT_MAGIC_STRING_SEARCH, ecma_builtin_string_prototype_object_search, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SPLIT, ecma_builtin_string_prototype_object_split, 2, 2)
#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

ROUTINE (LIT_MAGIC_STRING_SUBSTRING, ecma_builtin_string_prototype_object_substring, 2, 2)
ROUTINE (LIT_MAGIC_STRING_TO_LOWER_CASE_UL, ecma_builtin_string_prototype_object_to_lower_case, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_LOWER_CASE_UL, ecma_builtin_string_prototype_object_to_locale_lower_case, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_UPPER_CASE_UL, ecma_builtin_string_prototype_object_to_upper_case, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_UPPER_CASE_UL, ecma_builtin_string_prototype_object_to_locale_upper_case, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TRIM, ecma_builtin_string_prototype_object_trim, 0, 0)

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
ROUTINE (LIT_MAGIC_STRING_SUBSTR, ecma_builtin_string_prototype_object_substr, 2, 2)
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
