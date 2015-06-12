/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
 * RegExp built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable)
#endif /* !OBJECT_VALUE */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_writable, prop_enumerable, prop_configurable)
#endif /* !NUMBER_VALUE */

#ifndef SIMPLE_VALUE
# define SIMPLE_VALUE(name, simple_value, prop_writable, prop_enumerable, prop_configurable)
#endif /* !SIMPLE_VALUE */

#ifndef STRING_VALUE
# define STRING_VALUE(name, magic_string_id, prop_writable, prop_enumerable, prop_configurable)
#endif /* !STRING_VALUE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_REGEXP)

// ECMA-262 v5, 15.10.5.1
OBJECT_VALUE (ECMA_MAGIC_STRING_PROTOTYPE,
              ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE),
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// ECMA-262 v5, 15.10.7.1
STRING_VALUE (ECMA_MAGIC_STRING_SOURCE,
              ECMA_MAGIC_STRING_REGEXP_SOURCE_UL,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// ECMA-262 v5, 15.10.7.2
SIMPLE_VALUE (ECMA_MAGIC_STRING_GLOBAL,
              ECMA_SIMPLE_VALUE_FALSE,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// ECMA-262 v5, 15.10.7.3
SIMPLE_VALUE (ECMA_MAGIC_STRING_IGNORECASE_UL,
              ECMA_SIMPLE_VALUE_FALSE,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)
// ECMA-262 v5, 15.10.7.4
SIMPLE_VALUE (ECMA_MAGIC_STRING_MULTILINE,
              ECMA_SIMPLE_VALUE_FALSE,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// ECMA-262 v5, 15.10.7.5
NUMBER_VALUE (ECMA_MAGIC_STRING_LASTINDEX_UL,
              0,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

NUMBER_VALUE (ECMA_MAGIC_STRING_LENGTH,
              2,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef CP_UNIMPLEMENTED_VALUE
#undef ROUTINE
