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

/**
 * Jerry libc's common functions implementation
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerry-libc-defs.h"

/**
 * State of pseudo-random number generator
 */
static unsigned int libc_random_gen_state[4] = { 1455997910, 1999515274, 1234451287, 1949149569 };

/**
 * Standard file descriptors
 */
FILE *stdin  = (FILE*) 0;
FILE *stdout = (FILE*) 1;
FILE *stderr = (FILE*) 2;

#ifdef __GNUC__
/*
 * Making GCC not to replace:
 *   - memcpy  -> call to memcpy;
 *   - memset  -> call to memset;
 *   - memmove -> call to memmove.
 */
#define CALL_PRAGMA(x) _Pragma (#x)

CALL_PRAGMA (GCC diagnostic push)
CALL_PRAGMA (GCC diagnostic ignored "-Wpragmas")
CALL_PRAGMA (GCC push_options)
CALL_PRAGMA (GCC optimize ("-fno-tree-loop-distribute-patterns"))
#endif /* __GNUC__ */

/**
 * memset
 *
 * @return @s
 */
void* __attr_used___ // FIXME
memset (void *s,  /**< area to set values in */
        int c,    /**< value to set */
        size_t n) /**< area size */
{
  uint8_t *area_p = (uint8_t *) s;
  for (size_t index = 0; index < n; index++)
  {
    area_p[ index ] = (uint8_t) c;
  }

  return s;
} /* memset */

/**
 * memcmp
 *
 * @return 0, if areas are equal;
 *         -1, if first area's content is lexicographically less, than second area's content;
 *         1, otherwise
 */
int
memcmp (const void *s1, /**< first area */
        const void *s2, /**< second area */
        size_t n) /**< area size */
{
  const uint8_t *area1_p = (uint8_t *) s1, *area2_p = (uint8_t *) s2;
  for (size_t index = 0; index < n; index++)
  {
    if (area1_p[ index ] < area2_p[ index ])
    {
      return -1;
    }
    else if (area1_p[ index ] > area2_p[ index ])
    {
      return 1;
    }
  }

  return 0;
} /* memcmp */

/**
 * memcpy
 */
void *  __attr_used___ // FIXME
memcpy (void *s1, /**< destination */
        const void *s2, /**< source */
        size_t n) /**< bytes number */
{
  uint8_t *area1_p = (uint8_t *) s1;
  const uint8_t *area2_p = (const uint8_t *) s2;

  for (size_t index = 0; index < n; index++)
  {
    area1_p[ index ] = area2_p[ index ];
  }

  return s1;
} /* memcpy */

/**
 * memmove
 *
 * @return the dest pointer's value
 */
void * __attr_used___ // FIXME
memmove (void *s1, /**< destination */
         const void *s2, /**< source */
         size_t n) /**< bytes number */
{
  uint8_t *dest_p = (uint8_t *) s1;
  const uint8_t *src_p = (const uint8_t *) s2;

  if (dest_p < src_p)
  { /* from begin to end */
    for (size_t index = 0; index < n; index++)
    {
      dest_p[ index ] = src_p[ index ];
    }
  }
  else if (dest_p > src_p)
  { /* from end to begin */
    for (size_t index = 1; index <= n; index++)
    {
      dest_p[ n - index ] = src_p[ n - index ];
    }
  }

  return s1;
} /* memmove */

#ifdef __GNUC__
CALL_PRAGMA (GCC pop_options)
CALL_PRAGMA (GCC diagnostic pop)
#endif /* __GNUC__ */

/** Compare two strings. return an integer less than, equal to, or greater than zero
     if s1 is found, respectively, to be less than, to match, or be greater than s2.  */
int
strcmp (const char *s1, const char *s2)
{
  size_t i;
  if (s1 == NULL)
  {
    if (s2 != NULL)
    {
      return -1;
    }
    else
    {
      return 0;
    }
  }
  if (s2 == NULL)
  {
    return 1;
  }

  for (i = 0; s1[i]; i++)
  {
    if (s1[i] > s2[i])
    {
      return 1;
    }
    else if (s1[i] < s2[i])
    {
      return -1;
    }
  }

  if (s2[i])
  {
    return -1;
  }

  return 0;
}

/** Compare two strings. return an integer less than, equal to, or greater than zero
     if the first n character of s1 is found, respectively, to be less than, to match,
     or be greater than the first n character of s2.  */
int
strncmp (const char *s1, const char *s2, size_t n)
{
  size_t i;

  if (n == 0)
  {
    return 0;
  }

  if (s1 == NULL)
  {
    if (s2 != NULL)
    {
      return -1;
    }
    else
    {
      return 0;
    }
  }
  if (s2 == NULL)
  {
    return 1;
  }

  for (i = 0; i < n; i++)
  {
    if (s1[i] > s2[i])
    {
      return 1;
    }
    else if (s1[i] < s2[i])
    {
      return -1;
    }
  }

  return 0;
}

/** Copy a string. At most n bytes of src are copied.  Warning: If there is no
     null byte among the first n bytes of src, the string placed in dest will not be null-terminated.
     @return a pointer to the destination string dest.  */
char * __attr_used___ // FIXME
strncpy (char *dest, const char *src, size_t n)
{
  size_t i;

  for (i = 0; i < n; i++)
  {
    dest[i] = src[i];
    if (src[i] == '\0')
    {
      break;
    }
  }

  return dest;
}

/** Calculate the length of a string.  */
size_t
strlen (const char *s)
{
  size_t i;
  for (i = 0; s[i]; i++)
  {
    ;
  }

  return i;
}

/**
 * Generate pseudo-random integer
 *
 * Note:
 *      The function implements George Marsaglia's XorShift random number generator
 *
 * @return integer in range [0; RAND_MAX]
 */
int
rand (void)
{
  uint32_t intermediate = libc_random_gen_state[0] ^ (libc_random_gen_state[0] << 11);
  intermediate ^= intermediate >> 8;

  libc_random_gen_state[0] = libc_random_gen_state[1];
  libc_random_gen_state[1] = libc_random_gen_state[2];
  libc_random_gen_state[2] = libc_random_gen_state[3];

  libc_random_gen_state[3] ^= libc_random_gen_state[3] >> 19;
  libc_random_gen_state[3] ^= intermediate;

  uint32_t ret = libc_random_gen_state[3] % (RAND_MAX + 1u);

  return (int32_t) ret;
} /* rand */

/**
 * Initialize pseudo-random number generator with the specified seed value
 */
void
srand (unsigned int seed) /**< new seed */
{
  libc_random_gen_state[3] = seed;
} /* srand */
