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

#ifndef JERRY_BIT_FIELDS_H
#define JERRY_BIT_FIELDS_H

extern uint64_t __attr_const___ jrt_extract_bit_field (uint64_t value, size_t lsb, size_t width);
extern uint64_t __attr_const___ jrt_set_bit_field_value (uint64_t value, uint64_t bit_field_value,
                                                         size_t lsb, size_t width);

#endif /* !JERRY_BIT_FIELDS_H */
