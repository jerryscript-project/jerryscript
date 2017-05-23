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
 * This macro needs to be placed before any occurrence of JERRYX_SECTION_ITERATE, for a given section.
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

/**
 * Produces the for loop header for iterating over items in an array stored in a section
 *
 * @param name is the name of the section without quotes.
 * @param index is the name of a variable declared in the scope to be used as the index into the array.
 * @param item_p is a variable to be used as a pointer to an item in the array.
 *
 * The macro assumes that JERRYX_SECTION_DECLARE was used at the top of the file to make the section available.
 *
 * The macro is to be succeeded by the body of a for-loop. Inside the body of the loop, the macro sets @p index and
 * @p item_p as follows:
 *
 * @p index will be set to the index of the current item.
 * @p item_p will point to the current item.
 *
 * Usage example:
 * @code
 * int index;
 * widget_t *current_widget_p;
 * for JERRYX_SECTION_ITERATE (widget_array, index, current_widget_p)
 * {
 *    printf("widget %d in the array has name %s\n", index, current_widget_p->name);
 * }
 * @endcode
 */
#define JERRYX_SECTION_ITERATE(name, index, item_p) \
  (index = 0, item_p = &__start_ ## name[index];    \
  &__start_ ## name[index] < __stop_ ## name;       \
  index++, item_p = &__start_ ## name[index])

#endif /* !JERRYX_SECTION_IMPL_H */
