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
 * %TypedArrayPrototype% description
 */
#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

#ifndef STRING_VALUE
# define STRING_VALUE(name, magic_string_id, prop_attributes)
#endif /* !STRING_VALUE */

#ifndef OBJECT_VALUE
# define OBJECT_VALUE(name, obj_builtin_id, prop_attributes)
#endif /* !OBJECT_VALUE */

#ifndef ROUTINE
# define ROUTINE(name, c_function_name, args_number, length_prop_value)
#endif /* !ROUTINE */

/* ES2015 22.2.3.4 */
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_TYPEDARRAY,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Readonly accessor properties */
/* ES2015 22.2.3.1 */
ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_BUFFER,
                    ecma_builtin_typedarray_prototype_buffer_getter,
                    ECMA_PROPERTY_FIXED)
/* ES2015 22.2.3.2 */
ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_BYTE_LENGTH_UL,
                    ecma_builtin_typedarray_prototype_bytelength_getter,
                    ECMA_PROPERTY_FIXED)
/* ES2015 22.2.3.3 */
ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_BYTE_OFFSET_UL,
                    ecma_builtin_typedarray_prototype_byteoffset_getter,
                    ECMA_PROPERTY_FIXED)

/* ES2015 22.2.3.17 */
ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_LENGTH,
                    ecma_builtin_typedarray_prototype_length_getter,
                    ECMA_PROPERTY_FIXED)

ROUTINE (LIT_MAGIC_STRING_EVERY, ecma_builtin_typedarray_prototype_every, 2, 1)
ROUTINE (LIT_MAGIC_STRING_SOME, ecma_builtin_typedarray_prototype_some, 2, 1)
ROUTINE (LIT_MAGIC_STRING_FOR_EACH_UL, ecma_builtin_typedarray_prototype_for_each, 2, 1)
ROUTINE (LIT_MAGIC_STRING_MAP, ecma_builtin_typedarray_prototype_map, 2, 1)
ROUTINE (LIT_MAGIC_STRING_REDUCE, ecma_builtin_typedarray_prototype_reduce, 2, 1)
ROUTINE (LIT_MAGIC_STRING_REDUCE_RIGHT_UL, ecma_builtin_typedarray_prototype_reduce_right, 2, 1)
ROUTINE (LIT_MAGIC_STRING_FILTER, ecma_builtin_typedarray_prototype_filter, 2, 1)
ROUTINE (LIT_MAGIC_STRING_REVERSE, ecma_builtin_typedarray_prototype_reverse, 0, 0)

#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
