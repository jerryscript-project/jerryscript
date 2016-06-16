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
 * TypeError.prototype built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef STRING_VALUE
# define STRING_VALUE(name, magic_string_id, prop_attributes)
#endif /* !STRING_VALUE */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE)

/* Object properties:
 *  (property name, object pointer getter) */

// 15.11.7.8
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_TYPE_ERROR,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// 15.11.7.9
STRING_VALUE (LIT_MAGIC_STRING_NAME,
              LIT_MAGIC_STRING_TYPE_ERROR_UL,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

// 15.11.7.10
STRING_VALUE (LIT_MAGIC_STRING_MESSAGE,
              LIT_MAGIC_STRING__EMPTY,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

#undef OBJECT_ID
#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
