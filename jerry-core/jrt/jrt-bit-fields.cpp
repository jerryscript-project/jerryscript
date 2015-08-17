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

#include "jrt.h"
#include "jrt-bit-fields.h"

/**
 * Extract a bit-field from the integer.
 *
 * @return bit-field's value
 */
uint64_t __attr_const___
jrt_extract_bit_field (uint64_t container, /**< container to extract bit-field from */
                       size_t lsb, /**< least significant bit of the value
                                    *   to be extracted */
                       size_t width) /**< width of the bit-field to be extracted */
{
  JERRY_ASSERT (lsb < JERRY_BITSINBYTE * sizeof (uint64_t));
  JERRY_ASSERT (width < JERRY_BITSINBYTE * sizeof (uint64_t));
  JERRY_ASSERT ((lsb + width) <= JERRY_BITSINBYTE * sizeof (uint64_t));

  uint64_t shifted_value = container >> lsb;
  uint64_t bit_field_mask = (1ull << width) - 1;

  return (shifted_value & bit_field_mask);
} /* jrt_extract_bit_field */

/**
 * Extract a bit-field from the integer.
 *
 * @return bit-field's value
 */
uint64_t __attr_const___
jrt_set_bit_field_value (uint64_t container, /**< container to insert bit-field to */
                         uint64_t new_bit_field_value, /**< value of bit-field to insert */
                         size_t lsb, /**< least significant bit of the value
                                      *   to be extracted */
                         size_t width) /**< width of the bit-field to be extracted */
{
  JERRY_ASSERT (lsb < JERRY_BITSINBYTE * sizeof (uint64_t));
  JERRY_ASSERT (width < JERRY_BITSINBYTE * sizeof (uint64_t));
  JERRY_ASSERT ((lsb + width) <= JERRY_BITSINBYTE * sizeof (uint64_t));
  JERRY_ASSERT (new_bit_field_value < (1ull << width));

  uint64_t bit_field_mask = (1ull << width) - 1;
  uint64_t shifted_bit_field_mask = bit_field_mask << lsb;
  uint64_t shifted_new_bit_field_value = new_bit_field_value << lsb;

  return (container & ~shifted_bit_field_mask) | shifted_new_bit_field_value;
} /* jrt_set_bit_field_value */
