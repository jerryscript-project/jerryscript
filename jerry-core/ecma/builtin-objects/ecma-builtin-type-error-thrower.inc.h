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
 * [[ThrowTypeError]] description
 *
 * See also: ECMA-262 v5, 13.2.3
 */
#ifndef NUMBER_VALUE
# define NUMBER_VALUE(name, number_value, prop_attributes)
#endif /* !NUMBER_VALUE */

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              0,
              ECMA_PROPERTY_FIXED)

#undef SIMPLE_VALUE
#undef NUMBER_VALUE
#undef STRING_VALUE
#undef OBJECT_VALUE
#undef ROUTINE
#undef ACCESSOR_READ_WRITE
#undef ACCESSOR_READ_ONLY
