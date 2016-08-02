/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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
 * RegExp.prototype built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

#ifndef SIMPLE_VALUE
# define SIMPLE_VALUE(name, simple_value, prop_attributes)
#endif /* !SIMPLE_VALUE */

#ifndef STRING_VALUE
# define STRING_VALUE(name, magic_string_id, prop_attributes)
#endif /* !STRING_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE)

// ECMA-262 v5, 15.10.6.1
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_REGEXP,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// ECMA-262 v5, 15.10.7.1
STRING_VALUE (LIT_MAGIC_STRING_SOURCE,
              LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP,
              ECMA_PROPERTY_FIXED)

// ECMA-262 v5, 15.10.7.2
SIMPLE_VALUE (LIT_MAGIC_STRING_GLOBAL,
              ECMA_SIMPLE_VALUE_FALSE,
              ECMA_PROPERTY_FIXED)

// ECMA-262 v5, 15.10.7.3
SIMPLE_VALUE (LIT_MAGIC_STRING_IGNORECASE_UL,
              ECMA_SIMPLE_VALUE_FALSE,
              ECMA_PROPERTY_FIXED)

// ECMA-262 v5, 15.10.7.4
SIMPLE_VALUE (LIT_MAGIC_STRING_MULTILINE,
              ECMA_SIMPLE_VALUE_FALSE,
              ECMA_PROPERTY_FIXED)

// ECMA-262 v5, 15.10.7.5
NUMBER_VALUE (LIT_MAGIC_STRING_LASTINDEX_UL,
              0,
              ECMA_PROPERTY_FLAG_WRITABLE)

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
ROUTINE (LIT_MAGIC_STRING_COMPILE, ecma_builtin_regexp_prototype_compile, 2, 1)
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
ROUTINE (LIT_MAGIC_STRING_EXEC, ecma_builtin_regexp_prototype_exec, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TEST, ecma_builtin_regexp_prototype_test, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_regexp_prototype_to_string, 0, 0)

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
