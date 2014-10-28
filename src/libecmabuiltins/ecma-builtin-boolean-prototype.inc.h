/* Copyright 2014 Samsung Electronics Co., Ltd.
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
 * Boolean.prototype description
 */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* Object properties:
 *  (property name, object pointer getter) */
OBJECT_VALUE (ECMA_MAGIC_STRING_CONSTRUCTOR,
              ecma_builtin_get (ECMA_BUILTIN_ID_BOOLEAN),
              ECMA_PROPERTY_WRITABLE,
              ECMA_PROPERTY_NOT_ENUMERABLE,
              ECMA_PROPERTY_CONFIGURABLE)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (ECMA_MAGIC_STRING_TO_STRING_UL, ecma_builtin_boolean_prototype_object_to_string, 0, 0)
ROUTINE (ECMA_MAGIC_STRING_VALUE_OF_UL,  ecma_builtin_boolean_prototype_object_value_of,  0, 0)

#undef OBJECT_VALUE
#undef ROUTINE
