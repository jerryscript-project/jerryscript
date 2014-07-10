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
 * Jerry libc platform-specific functions linux implementation
 */

#include "globals.h"
#include "jerry-libc.h"

#include <stdarg.h>


extern void __noreturn exit(int status);
extern FILE* fopen(const char *path, const char *mode);
extern int fclose(FILE *fp);
extern size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
extern size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
extern int vprintf (__const char *__restrict __format, __builtin_va_list __arg);
extern int vfprintf (FILE *stream, __const char *__restrict __format, __builtin_va_list __arg);

/**
 * printf
 * 
 * @return number of characters printed
 */
int
__printf(const char *format, /**< format string */
         ...)                /**< parameters' values */
{
    va_list args;
    
    va_start( args, format);
    
    int ret = vprintf( format, args);
            
    va_end( args);
    
    return ret;
} /* __printf */

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
__putchar (int c)
{
  return __printf ("%c", c);
} /* __putchar */

/** exit - cause normal process termination  */
void __noreturn
__exit (int status)
{
  exit( status);
} /* __exit */

/**
 * fopen
 *
 * @return FILE pointer - upon successful completion,
 *         NULL - otherwise
 */
FILE*
__fopen(const char *path, /**< file path */
        const char *mode) /**< file open mode */
{
  return fopen( path, mode);
} /* __fopen */

/**
 * fclose
 *
 * @return 0 - upon successful completion,
 *         non-zero value - otherwise.
 */
int
__fclose(FILE *fp) /**< stream pointer */
{
  return fclose( fp);
} /* __fclose */

/**
 * fread
 *
 * @return number of bytes read
 */
size_t
__fread(void *ptr, /**< address of buffer to read to */
        size_t size, /**< size of elements to read */
        size_t nmemb, /**< number of elements to read */
        FILE *stream) /**< stream pointer */
{
  return fread(ptr, size, nmemb, stream);
} /* __fread */

/**
 * fwrite
 *
 * @return number of bytes written
 */
size_t
__fwrite(const void *ptr, /**< data to write */
         size_t size, /**< size of elements to write */
         size_t nmemb, /**< number of elements */
         FILE *stream) /**< stream pointer */
{
  return fwrite(ptr, size, nmemb, stream);
} /* __fwrite */

/**
 * fprintf
 * 
 * @return number of characters printed
 */
int
__fprintf(FILE *stream, /**< stream pointer */
          const char *format, /**< format string */
         ...)                /**< parameters' values */
{
    va_list args;
    
    va_start( args, format);
    
    int ret = vfprintf( stream, format, args);
            
    va_end( args);
    
    return ret;
} /* __fprintf */

