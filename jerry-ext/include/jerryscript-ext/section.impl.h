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

#ifndef JERRYX_SECTION_IMPL_H
#define JERRYX_SECTION_IMPL_H

/**
 * Define the name of a section
 *
 * @param name the name of the section without quotes
 */
#ifdef __MACH__
#define JERRYX_SECTION_NAME(name) "__DATA," #name
#else /* !__MACH__ */
#define JERRYX_SECTION_NAME(name) #name
#endif /* __MACH__ */

/**
 * Expands to the proper __attribute__(()) qualifier for appending a variable to a section.
 */
#define JERRYX_SECTION_ATTRIBUTE(name) \
  __attribute__ ((used, section (JERRYX_SECTION_NAME (name)), aligned (sizeof (void *))))

/**
 * Declare references to a section that contains an array of items.
 *
 * @param name the name of the section (without quotes)
 * @param type the type of the elements stored in the array
 *
 * This macro declares two extern const variables such that their type is an array of @p type and their names are
 * constructed by prefixing @p name with "__start_" and "__stop_". They evaluate to the starting and ending address
 * of the section @p name.
 */
#ifdef __MACH__
#define JERRYX_SECTION_DECLARE(name, type)                                   \
  extern const type __start_ ## name[] __asm("section$start$__DATA$" #name); \
  extern const type __stop_ ## name[] __asm("section$end$__DATA$" #name);
#else /* !__MACH__ */
#define JERRYX_SECTION_DECLARE(name, type)                    \
  extern const type __start_ ## name[] __attribute__((weak)); \
  extern const type __stop_ ## name[] __attribute__((weak));
#endif /* __MACH__ */

#endif /* !JERRYX_SECTION_IMPL_H */
