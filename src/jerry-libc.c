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

/**
 * Jerry libc implementation
 */

#include "jerry-libc.h"

#include <stdarg.h>

extern int vprintf (__const char *__restrict __format, __builtin_va_list __arg);

/**
 * memset
 * 
 * @return @s
 */
void*
libc_memset(void *s,  /**< area to set values in */
            int c,    /**< value to set */
            size_t n) /**< area size */
{
    uint8_t *pArea = s;
    for ( size_t index = 0; index < n; index++ )
    {
        pArea[ index ] = (uint8_t)c;
    }
    
    return s;
} /* libc_memset */

/**
 * memcmp
 * 
 * @return 0, if areas are equal;
 *         -1, if first area's content is lexicographically less, than second area's content;
 *         1, otherwise
 */
int
libc_memcmp(const void *s1, /**< first area */
            const void *s2, /**< second area */
            size_t n) /**< area size */
{
    const uint8_t *pArea1 = s1, *pArea2 = s2;
    for ( size_t index = 0; index < n; index++ )
    {
        if ( pArea1[ index ] < pArea2[ index ] )
        {
            return -1;
        } else if ( pArea1[ index ] > pArea2[ index ] )
        {
            return 1;
        }
    }
    
    return 0;
} /* libc_memcmp */

/**
 * memcpy
 */
void
libc_memcpy(void *s1, /**< destination */
            const void *s2, /**< source */
            size_t n) /**< bytes number */
{
    uint8_t *pArea1 = s1;
    const uint8_t *pArea2 = s2;
    
    for ( size_t index = 0; index < n; index++ )
    {
        pArea1[ index ] = pArea2[ index ];
    }
} /* libc_memcpy */

/**
 * printf
 * 
 * @return number of characters printed
 */
int
libc_printf(const char *format, /**< format string */
            ...)                /**< parameters' values */
{
    va_list args;
    
    va_start( args, format);
    
    int ret = vprintf( format, args);
            
    va_end( args);
    
    return ret;
} /* libc_printf */
