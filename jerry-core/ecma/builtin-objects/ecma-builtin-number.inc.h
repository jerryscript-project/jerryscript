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
 * Number built-in description
 */

#ifndef OBJECT_ID
# define OBJECT_ID(builtin_object_id)
#endif /* !OBJECT_ID */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_writable, prop_enumerable, prop_configurable)
#endif /* !NUMBER_VALUE */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable)
#endif /* !OBJECT_VALUE */

/* Object identifier */
OBJECT_ID (ECMA_BUILTIN_ID_NUMBER)

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

// 15.7.3
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              1,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// 15.7.3.4
NUMBER_VALUE (LIT_MAGIC_STRING_NAN,
              ecma_number_make_nan (),
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// 15.7.3.2
NUMBER_VALUE (LIT_MAGIC_STRING_MAX_VALUE_U,
              ECMA_NUMBER_MAX_VALUE,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// 15.7.3.3
NUMBER_VALUE (LIT_MAGIC_STRING_MIN_VALUE_U,
              ECMA_NUMBER_MIN_VALUE,
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// 15.7.3.5
NUMBER_VALUE (LIT_MAGIC_STRING_POSITIVE_INFINITY_U,
              ecma_number_make_infinity (false),
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

// 15.7.3.6
NUMBER_VALUE (LIT_MAGIC_STRING_NEGATIVE_INFINITY_U,
              ecma_number_make_infinity (true),
              ECMA_PROPERTY_NOT_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_NOT_CONFIGURABLE)

/* Object properties:
 *  (property name, object pointer getter) */

// 15.7.3.1
OBJECT_VALUE (LIT_MAGIC_STRING_PROTOTYPE,
              ecma_builtin_get (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE),
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
