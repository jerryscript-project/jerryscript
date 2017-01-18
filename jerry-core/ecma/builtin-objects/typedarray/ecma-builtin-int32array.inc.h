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
 * Int32Array description
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

/* ES2015 22.2.5 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              3,
              ECMA_PROPERTY_FIXED)

/* ES2015 22.2.5.1 */
NUMBER_VALUE (LIT_MAGIC_STRING_BYTES_PER_ELEMENT_U,
              4,
              ECMA_PROPERTY_FIXED)

/* ES2015 22.2.5 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              3,
              ECMA_PROPERTY_FIXED)

/* ES2015 22.2.5 */
STRING_VALUE (LIT_MAGIC_STRING_NAME,
              LIT_MAGIC_STRING_INT32_ARRAY_UL,
              ECMA_PROPERTY_FIXED)

/* ES2015 22.2.5.2 */
OBJECT_VALUE (LIT_MAGIC_STRING_PROTOTYPE,
              ECMA_BUILTIN_ID_INT32ARRAY_PROTOTYPE,
              ECMA_PROPERTY_FIXED)

#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
