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

#ifdef LIBC_RAW

#ifdef __TARGET_HOST_x64
# include "asm_x64.h"
#elif defined(__TARGET_HOST_x86)
# include "asm_x86.h"
#endif /* !__TARGET_HOST_x64 && TARGET_HOST_x86 */

FIXME( Rename __unused )
#undef __unused

#include <unistd.h>
#include <syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

/**
 * Exit program with ERR_SYSCALL if syscall_ret_val is negative
 */
#define LIBC_EXIT_ON_ERROR( syscall_ret_val) \
  if ( unlikely( ( syscall_ret_val ) < 0 ) ) \
    { \
      __exit( -ERR_SYSCALL); \
    }

static long int syscall_1( long int syscall_no, long int arg1);
static long int syscall_2( long int syscall_no, long int arg1, long int arg2);
static long int syscall_3( long int syscall_no, long int arg1, long int arg2, long int arg3);

/**
 * System call with one argument.
 *
 * @return syscall's return value
 */
static long int
syscall_1( long int syscall_no, /**< syscall number */
           long int arg1) /**< argument */
{
  long int ret;

  SYSCALL_1( syscall_no, arg1, ret);

  LIBC_EXIT_ON_ERROR( ret );
  
  return ret;
} /* syscall_1 */

/**
 * System call with two arguments.
 *
 * @return syscall's return value
 */
static long int
syscall_2( long int syscall_no, /**< syscall number */
           long int arg1, /**< first argument */
           long int arg2) /**< second argument */
{
  long int ret;

  SYSCALL_2( syscall_no, arg1, arg2, ret);

  LIBC_EXIT_ON_ERROR( ret );
  
  return ret;
} /* syscall_2 */

/**
 * System call with three arguments.
 *
 * @return syscall's return value
 */
static long int
syscall_3( long int syscall_no, /**< syscall number */
           long int arg1, /**< first argument */
           long int arg2, /**< second argument */
           long int arg3) /**< third argument */
{
  long int ret;

  SYSCALL_3( syscall_no, arg1, arg2, arg3, ret);

  LIBC_EXIT_ON_ERROR( ret );
  
  return ret;
} /* syscall_3 */

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
__putchar (int c)
{
  __fwrite( &c, 1, sizeof(char), LIBC_STDOUT);

  return c;
} /* __putchar */

/**
 * Exit - cause normal process termination with specified status code
 */
void __noreturn
__exit (int status) /**< status code */
{
  syscall_1( __NR_close, (long int)LIBC_STDIN);
  syscall_1( __NR_close, (long int)LIBC_STDOUT);
  syscall_1( __NR_close, (long int)LIBC_STDERR);

  syscall_1( __NR_exit_group, status);

  while ( true )
    {
      /* unreachable */
    }
} /* __exit */

/**
 * fopen
 *
 * @return _FILE pointer - upon successful completion,
 *         NULL - otherwise
 */
_FILE*
__fopen(const char *path, /**< file path */
        const char *mode) /**< file open mode */
{
  bool may_read = false,
       may_write = false,
       truncate = false,
       create_if_not_exist = false,
       position_at_end = false;

  JERRY_ASSERT( path != NULL && mode != NULL );
  JERRY_ASSERT( mode[1] == '+' || mode[1] == '\0' );

  switch( mode[0] )
    {
    case 'r':
      may_read = true;
      may_write = (mode[1] == '+');
      break;
    case 'w':
      may_write = true;
      truncate = true;
      create_if_not_exist = true;
      may_read = (mode[1] == '+');
      break;
    case 'a':
      may_write = true;
      position_at_end = true;
      create_if_not_exist = true;
      if ( mode[1] == '+' )
        {
          JERRY_UNIMPLEMENTED();
        }
      break;
    default:
      JERRY_UNREACHABLE();
    }

  int flags = 0;
  int access = S_IRUSR | S_IWUSR;
  if ( may_read && !may_write )
    {
      flags = O_RDONLY;
    }
  else if ( !may_read && may_write )
    {
      flags = O_WRONLY;
    }
  else
    {
      JERRY_ASSERT( may_read && may_write );

      flags = O_RDWR;
    }

  if ( truncate )
    {
      flags |= O_TRUNC;
    }

  if ( create_if_not_exist )
    {
      flags |= O_CREAT;
    }

  if ( position_at_end )
    {
      flags |= O_APPEND;
    }

  long int ret = syscall_3( __NR_open, (long int)path, flags, access);

  return (void*)(uintptr_t)(ret);
} /* __fopen */

/**
 * The rewind() function sets the file position indicator
 * for the stream pointed to by STREAM to the beginning of the file.
 */
void
__rewind (_FILE *stream) /**< stream pointer */
{
  syscall_3( __NR_lseek, (long int)stream, 0, SEEK_SET);
} /* __rewind */

/**
 * fclose
 *
 * @return 0 - upon successful completion,
 *         non-zero value - otherwise.
 */
int
__fclose(_FILE *fp) /**< stream pointer */
{
  syscall_2( __NR_close, (long int)fp, 0);

  return 0;
} /* __fclose */

/**
 * fseek
 */
int
__fseek(_FILE * fp, /**< stream pointer */
        long offset, /**< offset */
        _whence_t whence) /**< specifies position type
                               to add offset to */
{
  int whence_real = SEEK_CUR;
  switch ( whence )
  {
    case __SEEK_SET:
      whence_real = SEEK_SET;
      break;
    case __SEEK_CUR:
      whence_real = SEEK_CUR;
      break;
    case __SEEK_END:
      whence_real = SEEK_END;
      break;
  }

  syscall_3( __NR_lseek, (long int)fp, offset, whence_real);

  return 0;
} /* __fseek */

/**
 * ftell
 */
long
__ftell(_FILE * fp) /**< stream pointer */
{
  long int ret = syscall_3( __NR_lseek, (long int)fp, 0, SEEK_CUR);

  return ret;
} /* __ftell */

/**
 * fread
 *
 * @return number of bytes read
 */
size_t
__fread(void *ptr, /**< address of buffer to read to */
        size_t size, /**< size of elements to read */
        size_t nmemb, /**< number of elements to read */
        _FILE *stream) /**< stream pointer */
{
  long int ret;
  size_t bytes_read = 0;

  do
    {
      ret = syscall_3( __NR_read, (long int)stream, (long int) ((uint8_t*)ptr + bytes_read), (long int) (size * nmemb - bytes_read));

      bytes_read += (size_t)ret;
    } while (bytes_read != size * nmemb && ret != 0);

  return bytes_read;
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
         _FILE *stream) /**< stream pointer */
{
  size_t bytes_written = 0;

  do
    {
      long int ret = syscall_3( __NR_write, (long int)stream, (long int) ((uint8_t*)ptr + bytes_written), (long int) (size * nmemb - bytes_written));

      bytes_written += (size_t)ret;
    } while (bytes_written != size * nmemb);

  return bytes_written;
} /* __fwrite */

#elif defined(LIBC_MUSL)

#include <stdio.h>
#include <stdlib.h>

const _FILE **libc_stdin = (void*)&stdin;
const _FILE **libc_stdout = (void*)&stdout;
const _FILE **libc_stderr = (void*)&stderr;

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
__putchar (int c)
{
  return putchar( c);
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
 * @return _FILE pointer - upon successful completion,
 *         NULL - otherwise
 */
_FILE*
__fopen(const char *path, /**< file path */
        const char *mode) /**< file open mode */
{
  return fopen( path, mode);
} /* __fopen */

/** The rewind() function sets the file position 
  indicator for the stream pointed to by STREAM to the beginning of the file.  */
void
__rewind (_FILE *stream)
{
  rewind (stream);
}

/**
 * fclose
 *
 * @return 0 - upon successful completion,
 *         non-zero value - otherwise.
 */
int
__fclose(_FILE *fp) /**< stream pointer */
{
  return fclose( fp);
} /* __fclose */

/**
 * fseek
 */
int
__fseek(_FILE * fp, /**< stream pointer */
        long offset, /**< offset */
        _whence_t whence) /**< specifies position type
                               to add offset to */
{
  int whence_real = SEEK_CUR;
  switch ( whence )
  {
    case __SEEK_SET:
      whence_real = SEEK_SET;
      break;
    case __SEEK_CUR:
      whence_real = SEEK_CUR;
      break;
    case __SEEK_END:
      whence_real = SEEK_END;
      break;
  }

  return fseek( fp, offset, whence_real);
} /* __fseek */

/**
 * ftell
 */
long
__ftell(_FILE * fp) /**< stream pointer */
{
  return ftell( fp);
} /* __ftell */

/**
 * fread
 *
 * @return number of bytes read
 */
size_t
__fread(void *ptr, /**< address of buffer to read to */
        size_t size, /**< size of elements to read */
        size_t nmemb, /**< number of elements to read */
        _FILE *stream) /**< stream pointer */
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
         _FILE *stream) /**< stream pointer */
{
  return fwrite(ptr, size, nmemb, stream);
} /* __fwrite */

#else /* !LIBC_RAW && !LIBC_MUSL */
# error "!LIBC_RAW && !LIBC_MUSL"
#endif /* !LIBC_RAW && !LIBC_MUSL */
