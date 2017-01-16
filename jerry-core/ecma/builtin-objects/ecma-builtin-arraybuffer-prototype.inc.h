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
#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

#ifndef ACCESSOR_READ_WRITE
# define ACCESSOR_READ_WRITE(name, c_getter_func_name, c_setter_func_name, prop_attributes)
#endif /* !ROUTINE */

#ifndef ACCESSOR_READ_ONLY
# define ACCESSOR_READ_ONLY(name, c_getter_func_name, prop_attributes)
#endif /* !ACCESSOR_READ_ONLY */

/* Object properties:
 *  (property name, object pointer getter) */

OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_ARRAYBUFFER,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Readonly accessor properties */
ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_BYTE_LENGTH_UL,
                    ecma_builtin_arraybuffer_prototype_bytelength_getter,
                    ECMA_PROPERTY_FIXED)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_SLICE, ecma_builtin_arraybuffer_prototype_object_slice, 2, 2)

#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
