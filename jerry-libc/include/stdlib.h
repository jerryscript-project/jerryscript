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

#ifndef JERRY_LIBC_STDLIB_H
#define JERRY_LIBC_STDLIB_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Maximum integer that could be returned by random number generator
 *
 * See also:
 *          rand
 */
#define RAND_MAX (0x7fffffffu)

void __attribute__ ((noreturn)) exit (int);
void __attribute__ ((noreturn)) abort (void);
int rand (void);
void srand (unsigned int);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_LIBC_STDLIB_H */
