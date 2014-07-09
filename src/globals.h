/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#ifndef JERRY_GLOBALS_H
#define JERRY_GLOBALS_H

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Types
 */
typedef unsigned long mword_t;
typedef mword_t uintptr_t;
//typedef mword_t size_t;
typedef signed long ssize_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed int int32_t;

/**
 * Attributes
 */
#define __unused __attribute__((unused))
#define __packed __attribute__((packed))

/**
 * Constants
 */

#define JERRY_BITSINBYTE 8

/**
 * Asserts
 *
 * Warning:
 *         Don't use JERRY_STATIC_ASSERT in headers, because
 *         __LINE__ may be the same for asserts in a header
 *         and in an implementation file.
 */
#define JERRY_STATIC_ASSERT_GLUE_( a, b ) a ## b
#define JERRY_STATIC_ASSERT_GLUE( a, b ) JERRY_STATIC_ASSERT_GLUE_( a, b )
#define JERRY_STATIC_ASSERT( x ) typedef char JERRY_STATIC_ASSERT_GLUE( static_assertion_failed_, __LINE__) [ ( x ) ? 1 : -1 ] __unused

#define CALL_PRAGMA(x) _Pragma (#x)
#define TODO(x) CALL_PRAGMA(message ("TODO - " #x))
#define FIXME(x) CALL_PRAGMA(message ("FIXME - " #x))

/**
 * Variable that must not be referenced.
 *
 * May be used for static assertion checks.
 */
extern uint32_t jerry_UnreferencedExpression;

#ifndef JERRY_NDEBUG
#define JERRY_ASSERT( x ) assert( x )
#else /* !JERRY_NDEBUG */
#define JERRY_ASSERT( x ) (void) (x)
#endif /* !JERRY_NDEBUG */

/**
 * Mark for unreachable points and unimplemented cases
 */
#define JERRY_UNREACHABLE() do { JERRY_ASSERT( false); __builtin_trap(); } while (0)
#define JERRY_UNIMPLEMENTED() JERRY_UNREACHABLE()

/**
 * sizeof, offsetof, ...
 */
#define JERRY_SIZE_OF_STRUCT_MEMBER( struct_name, member_name) sizeof(((struct_name*)NULL)->member_name)

/**
 * Alignment
 */

/**
 * Aligns @value to @alignment.
 *
 * Returns maximum positive value, that divides @alignment and is less than or equal to @value
 */
#define JERRY_ALIGNDOWN( value, alignment) ( (alignment) * ( (value) / (alignment) ) )

/**
 * Aligns @value to @alignment.
 *
 * Returns minimum positive value, that divides @alignment and is more than or equal to @value
 */
#define JERRY_ALIGNUP( value, alignment)   ( (alignment) * ( ((value) + (alignment) - 1) / (alignment) ) )

/**
 * min, max
 */
#define JERRY_MIN( v1, v2) ( ( v1 < v2 ) ? v1 : v2 )
#define JERRY_MAX( v1, v2) ( ( v1 < v2 ) ? v2 : v1 )

/**
 * Bit-fields
 */
inline uint32_t jerry_ExtractBitField(uint32_t value, uint32_t lsb,
                                      uint32_t width);
inline uint32_t jerry_SetBitFieldValue(uint32_t value, uint32_t bitFieldValue,
                                       uint32_t lsb, uint32_t width);

/**
 * Extract a bit-field from the integer.
 *
 * @return bit-field's value
 */
inline uint32_t
jerry_ExtractBitField(uint32_t
                      container, /**< container to extract bit-field from */
                      uint32_t lsb, /**< least significant bit of the value
                                     *   to be extracted */
                      uint32_t width) /**< width of the bit-field to be extracted */
{
    JERRY_ASSERT(lsb < JERRY_BITSINBYTE * sizeof (uint32_t));
    JERRY_ASSERT((lsb + width) <= JERRY_BITSINBYTE * sizeof (uint32_t));

    uint32_t shiftedValue = container >> lsb;
    uint32_t bitFieldMask = (1u << width) - 1;

    return ( shiftedValue & bitFieldMask);
} /* jerry_ExtractBitField */

/**
 * Extract a bit-field from the integer.
 *
 * @return bit-field's value
 */
inline uint32_t
jerry_SetBitFieldValue(uint32_t
                       container, /**< container to insert bit-field to */
                       uint32_t newBitFieldValue, /**< value of bit-field to insert */
                       uint32_t lsb, /**< least significant bit of the value
                                      *   to be extracted */
                       uint32_t width) /**< width of the bit-field to be extracted */
{
    JERRY_ASSERT(lsb < JERRY_BITSINBYTE * sizeof (uint32_t));
    JERRY_ASSERT((lsb + width) <= JERRY_BITSINBYTE * sizeof (uint32_t));
    JERRY_ASSERT(newBitFieldValue <= (1u << width));

    uint32_t bitFieldMask = (1u << width) - 1;
    uint32_t shiftedBitFieldMask = bitFieldMask << lsb;
    uint32_t shiftedNewBitFieldValue = newBitFieldValue << lsb;

    return ( container & ~shiftedBitFieldMask) | shiftedNewBitFieldValue;
} /* jerry_SetBitFieldValue */

#endif /* !JERRY_GLOBALS_H */
