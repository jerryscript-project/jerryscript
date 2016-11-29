/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerry-libc-defs.h"

/**
 * State of pseudo-random number generator
 */
static uint32_t libc_random_gen_state[4] = { 1455997910, 1999515274, 1234451287, 1949149569 };

/**
 * Standard file descriptors
 */
FILE *stdin  = (FILE *) 0;
FILE *stdout = (FILE *) 1;
FILE *stderr = (FILE *) 2;

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
 * @return @a s
 */
void * __attr_used___
memset (void *s,  /**< area to set values in */
        int c,    /**< value to set */
        size_t n) /**< area size */
{
  uint8_t *area_p = (uint8_t *) s;
  while (n--)
  {
    *area_p++ = (uint8_t) c;
  }

  return s;
} /* memset */

/**
 * memcmp
 *
 * @return 0, if areas are equal;
 *         <0, if first area's content is lexicographically less, than second area's content;
 *         >0, otherwise
 */
int
memcmp (const void *s1, /**< first area */
        const void *s2, /**< second area */
        size_t n) /**< area size */
{
  const uint8_t *area1_p = (uint8_t *) s1, *area2_p = (uint8_t *) s2;
  while (n--)
  {
    int diff = ((int) *area1_p++) - ((int) *area2_p++);
    if (diff)
    {
      return diff;
    }
  }

  return 0;
} /* memcmp */

/**
 * memcpy
 */
void *  __attr_used___
memcpy (void *s1, /**< destination */
        const void *s2, /**< source */
        size_t n) /**< bytes number */
{
  uint8_t *dst_p = (uint8_t *) s1;
  const uint8_t *src_p = (const uint8_t *) s2;

  /* Aligned fast case. */
  if (n >= 4 && !(((uintptr_t) s1) & 0x3) && !(((uintptr_t) s2) & 0x3))
  {
    size_t chunks = (n >> 2);
    uint32_t *u32_dst_p = (uint32_t *) dst_p;
    const uint32_t *u32_src_p = (const uint32_t *) src_p;

    do
    {
      *u32_dst_p++ = *u32_src_p++;
    }
    while (--chunks);

    n &= 0x3;
    dst_p = (uint8_t *) u32_dst_p;
    src_p = (const uint8_t *) u32_src_p;
  }

  while (n--)
  {
    *dst_p++ = *src_p++;
  }

  return s1;
} /* memcpy */

/**
 * memmove
 *
 * @return the dest pointer's value
 */
void * __attr_used___
memmove (void *s1, /**< destination */
         const void *s2, /**< source */
         size_t n) /**< bytes number */
{
  uint8_t *dest_p;
  const uint8_t *src_p;

  if (s1 < s2)
  { /* from begin to end */
    dest_p = (uint8_t *) s1;
    src_p = (const uint8_t *) s2;

    while (n--)
    {
      *dest_p++ = *src_p++;
    }
  }
  else if (s1 > s2)
  { /* from end to begin */
    dest_p = ((uint8_t *) s1) + n - 1;
    src_p = ((const uint8_t *) s2) + n - 1;

    while (n--)
    {
      *dest_p-- = *src_p--;
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
  while (1)
  {
    int c1 = (unsigned char) *s1++;
    int c2 = (unsigned char) *s2++;
    int diff = c1 - c2;

    if (!c1 || diff)
    {
      return diff;
    }
  }
} /* strcmp */

/** Compare two strings. return an integer less than, equal to, or greater than zero
     if the first n character of s1 is found, respectively, to be less than, to match,
     or be greater than the first n character of s2.  */
int
strncmp (const char *s1, const char *s2, size_t n)
{
  while (n--)
  {
    int c1 = (unsigned char) *s1++;
    int c2 = (unsigned char) *s2++;
    int diff = c1 - c2;

    if (!c1 || diff)
    {
      return diff;
    }
  }

  return 0;
} /* strncmp */

/** Copy a string. At most n bytes of src are copied.  Warning: If there is no
     null byte among the first n bytes of src, the string placed in dest will not be null-terminated.
     @return a pointer to the destination string dest.  */
char * __attr_used___
strncpy (char *dest, const char *src, size_t n)
{
  while (n--)
  {
    char c = *src++;
    *dest++ = c;

    if (!c)
    {
      break;
    }
  }

  return dest;
} /* strncpy */

/** Calculate the length of a string.  */
size_t
strlen (const char *s)
{
  size_t i = 0;
  while (s[i])
  {
    i++;
  }

  return i;
} /* strlen */

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

  return (int) (libc_random_gen_state[3] % (RAND_MAX + 1));
} /* rand */

/**
 * Initialize pseudo-random number generator with the specified seed value
 */
void
srand (unsigned int seed) /**< new seed */
{
  libc_random_gen_state[0] =
  libc_random_gen_state[1] =
  libc_random_gen_state[2] =
  libc_random_gen_state[3] = seed;
} /* srand */
