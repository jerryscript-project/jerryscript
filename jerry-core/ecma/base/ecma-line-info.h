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

#ifndef ECMA_LINE_INFO_H
#define ECMA_LINE_INFO_H

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalineinfo Line info
 * @{
 */

#if JERRY_LINE_INFO

#include "ecma-globals.h"

/**
 * Increase the current value of line or column.
 */
#define ECMA_LINE_INFO_INCREASE 0x0

/**
 * Decrease the current value of line or column.
 */
#define ECMA_LINE_INFO_DECREASE 0x1

/**
 * Line update is present.
 */
#define ECMA_LINE_INFO_HAS_LINE 0x1

/**
 * A default value for columns after a line update.
 */
#define ECMA_LINE_INFO_COLUMN_DEFAULT 127

/**
 * Vlq encoding: flag which is set for all bytes except the last one.
 */
#define ECMA_LINE_INFO_VLQ_CONTINUE 0x80

/**
 * Vlq encoding: mask to decode the number fragment.
 */
#define ECMA_LINE_INFO_VLQ_MASK 0x7f

/**
 * Vlq encoding: number of bits stored in a byte.
 */
#define ECMA_LINE_INFO_VLQ_SHIFT 7

/**
 * Small encoding: a value which represents a two byte long number.
 */
#define ECMA_LINE_INFO_ENCODE_TWO_BYTE (UINT8_MAX - 1)

/**
 * Small encoding: minimum value of an encoded two byte long number.
 */
#define ECMA_LINE_INFO_ENCODE_TWO_BYTE_MIN (UINT8_MAX - 1)

/**
 * Small encoding: a value which represents a three byte long number.
 */
#define ECMA_LINE_INFO_ENCODE_VLQ UINT8_MAX

/**
 * Small encoding: minimum value of an encoded three byte long number.
 */
#define ECMA_LINE_INFO_ENCODE_VLQ_MIN (ECMA_LINE_INFO_ENCODE_TWO_BYTE_MIN + UINT8_MAX + 1)

/**
 * Maximum number of line/column entries stored in a stream.
 */
#define ECMA_LINE_INFO_STREAM_VALUE_COUNT_MAX 48

/**
 * Minimum size of a stream (except the last one).
 */
#define ECMA_LINE_INFO_STREAM_SIZE_MIN ((2 * ECMA_LINE_INFO_STREAM_VALUE_COUNT_MAX) - 1)

/* Helper functions for parser/js/js-parser-line-info-create.c. */
uint32_t ecma_line_info_decode_vlq (uint8_t **buffer_p);
uint32_t ecma_line_info_difference_update (uint32_t current_value, uint32_t difference_value);

/* General functions. */
void ecma_line_info_free (uint8_t *line_info_p);
void ecma_line_info_get (uint8_t *line_info_p, uint32_t offset, jerry_frame_location_t *location_p);

#if JERRY_PARSER_DUMP_BYTE_CODE
void ecma_line_info_dump (uint8_t *line_info_p);
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */

#endif /* JERRY_LINE_INFO */

/**
 * @}
 * @}
 */

#endif /* !ECMA_LINE_INFO_H */
