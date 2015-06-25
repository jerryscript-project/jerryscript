/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
# define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE)

OBJECT_VALUE (ECMA_MAGIC_STRING_CONSTRUCTOR,
              ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP),
              ECMA_PROPERTY_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_CONFIGURABLE)

ROUTINE (ECMA_MAGIC_STRING_EXEC, ecma_builtin_regexp_prototype_exec, 1, 1)
ROUTINE (ECMA_MAGIC_STRING_TEST, ecma_builtin_regexp_prototype_test, 1, 1)
ROUTINE (ECMA_MAGIC_STRING_TO_STRING_UL, ecma_builtin_regexp_prototype_to_string, 0, 0)

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef CP_UNIMPLEMENTED_VALUE
#undef ROUTINE
