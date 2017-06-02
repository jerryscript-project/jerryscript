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

/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

/*
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

#include "debugger.h"

#ifdef JERRY_DEBUGGER

/**
 * SHA-1 context structure.
 */
typedef struct
{
  uint32_t total[2]; /**< number of bytes processed */
  uint32_t state[5]; /**< intermediate digest state */
  uint8_t buffer[64]; /**< data block being processed */
} jerry_sha1_context;

/* 32-bit integer manipulation macros (big endian). */

#define JERRY_SHA1_GET_UINT32_BE(n, b, i) \
{ \
  (n) = (((uint32_t) (b)[(i) + 0]) << 24) \
        | (((uint32_t) (b)[(i) + 1]) << 16) \
        | (((uint32_t) (b)[(i) + 2]) << 8) \
        | ((uint32_t) (b)[(i) + 3]); \
}

#define JERRY_SHA1_PUT_UINT32_BE(n, b, i) \
{ \
  (b)[(i) + 0] = (uint8_t) ((n) >> 24); \
  (b)[(i) + 1] = (uint8_t) ((n) >> 16); \
  (b)[(i) + 2] = (uint8_t) ((n) >> 8); \
  (b)[(i) + 3] = (uint8_t) ((n)); \
}

/**
 * Initialize SHA-1 context.
 */
static void
jerry_sha1_init (jerry_sha1_context *sha1_context_p) /**< SHA-1 context */
{
  memset (sha1_context_p, 0, sizeof (jerry_sha1_context));

  sha1_context_p->total[0] = 0;
  sha1_context_p->total[1] = 0;

  sha1_context_p->state[0] = 0x67452301;
  sha1_context_p->state[1] = 0xEFCDAB89;
  sha1_context_p->state[2] = 0x98BADCFE;
  sha1_context_p->state[3] = 0x10325476;
  sha1_context_p->state[4] = 0xC3D2E1F0;
} /* jerry_sha1_init */

#define JERRY_SHA1_P(a, b, c, d, e, x) \
do { \
  e += JERRY_SHA1_SHIFT (a, 5) + JERRY_SHA1_F (b, c, d) + K + x; \
  b = JERRY_SHA1_SHIFT (b, 30); \
} while (0)

/**
 * Update SHA-1 internal buffer status.
 */
static void
jerry_sha1_process (jerry_sha1_context *sha1_context_p, /**< SHA-1 context */
                    const uint8_t data[64]) /**< data buffer */
{
  uint32_t temp, W[16], A, B, C, D, E;

  JERRY_SHA1_GET_UINT32_BE (W[0], data, 0);
  JERRY_SHA1_GET_UINT32_BE (W[1], data, 4);
  JERRY_SHA1_GET_UINT32_BE (W[2], data, 8);
  JERRY_SHA1_GET_UINT32_BE (W[3], data, 12);
  JERRY_SHA1_GET_UINT32_BE (W[4], data, 16);
  JERRY_SHA1_GET_UINT32_BE (W[5], data, 20);
  JERRY_SHA1_GET_UINT32_BE (W[6], data, 24);
  JERRY_SHA1_GET_UINT32_BE (W[7], data, 28);
  JERRY_SHA1_GET_UINT32_BE (W[8], data, 32);
  JERRY_SHA1_GET_UINT32_BE (W[9], data, 36);
  JERRY_SHA1_GET_UINT32_BE (W[10], data, 40);
  JERRY_SHA1_GET_UINT32_BE (W[11], data, 44);
  JERRY_SHA1_GET_UINT32_BE (W[12], data, 48);
  JERRY_SHA1_GET_UINT32_BE (W[13], data, 52);
  JERRY_SHA1_GET_UINT32_BE (W[14], data, 56);
  JERRY_SHA1_GET_UINT32_BE (W[15], data, 60);

#define JERRY_SHA1_SHIFT(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define JERRY_SHA1_R(t) \
( \
  temp = W[(t - 3) & 0x0F] ^ W[(t - 8) & 0x0F] ^ W[(t - 14) & 0x0F] ^ W[t & 0x0F], \
  W[t & 0x0F] = JERRY_SHA1_SHIFT (temp, 1) \
)

  A = sha1_context_p->state[0];
  B = sha1_context_p->state[1];
  C = sha1_context_p->state[2];
  D = sha1_context_p->state[3];
  E = sha1_context_p->state[4];

  uint32_t K = 0x5A827999;

#define JERRY_SHA1_F(x, y, z) (z ^ (x & (y ^ z)))

  JERRY_SHA1_P (A, B, C, D, E, W[0]);
  JERRY_SHA1_P (E, A, B, C, D, W[1]);
  JERRY_SHA1_P (D, E, A, B, C, W[2]);
  JERRY_SHA1_P (C, D, E, A, B, W[3]);
  JERRY_SHA1_P (B, C, D, E, A, W[4]);
  JERRY_SHA1_P (A, B, C, D, E, W[5]);
  JERRY_SHA1_P (E, A, B, C, D, W[6]);
  JERRY_SHA1_P (D, E, A, B, C, W[7]);
  JERRY_SHA1_P (C, D, E, A, B, W[8]);
  JERRY_SHA1_P (B, C, D, E, A, W[9]);
  JERRY_SHA1_P (A, B, C, D, E, W[10]);
  JERRY_SHA1_P (E, A, B, C, D, W[11]);
  JERRY_SHA1_P (D, E, A, B, C, W[12]);
  JERRY_SHA1_P (C, D, E, A, B, W[13]);
  JERRY_SHA1_P (B, C, D, E, A, W[14]);
  JERRY_SHA1_P (A, B, C, D, E, W[15]);
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (16));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (17));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (18));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (19));

#undef JERRY_SHA1_F

  K = 0x6ED9EBA1;

#define JERRY_SHA1_F(x, y, z) (x ^ y ^ z)

  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (20));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (21));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (22));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (23));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (24));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (25));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (26));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (27));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (28));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (29));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (30));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (31));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (32));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (33));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (34));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (35));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (36));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (37));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (38));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (39));

#undef JERRY_SHA1_F

  K = 0x8F1BBCDC;

#define JERRY_SHA1_F(x, y, z) ((x & y) | (z & (x | y)))

  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (40));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (41));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (42));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (43));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (44));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (45));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (46));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (47));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (48));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (49));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (50));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (51));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (52));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (53));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (54));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (55));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (56));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (57));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (58));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (59));

#undef JERRY_SHA1_F

  K = 0xCA62C1D6;

#define JERRY_SHA1_F(x, y, z) (x ^ y ^ z)

  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (60));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (61));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (62));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (63));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (64));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (65));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (66));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (67));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (68));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (69));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (70));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (71));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (72));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (73));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (74));
  JERRY_SHA1_P (A, B, C, D, E, JERRY_SHA1_R (75));
  JERRY_SHA1_P (E, A, B, C, D, JERRY_SHA1_R (76));
  JERRY_SHA1_P (D, E, A, B, C, JERRY_SHA1_R (77));
  JERRY_SHA1_P (C, D, E, A, B, JERRY_SHA1_R (78));
  JERRY_SHA1_P (B, C, D, E, A, JERRY_SHA1_R (79));

#undef JERRY_SHA1_F

  sha1_context_p->state[0] += A;
  sha1_context_p->state[1] += B;
  sha1_context_p->state[2] += C;
  sha1_context_p->state[3] += D;
  sha1_context_p->state[4] += E;

#undef JERRY_SHA1_SHIFT
#undef JERRY_SHA1_R
} /* jerry_sha1_process */

#undef JERRY_SHA1_P

/**
 * SHA-1 update buffer.
 */
static void
jerry_sha1_update (jerry_sha1_context *sha1_context_p, /**< SHA-1 context */
                   const uint8_t *source_p, /**< source buffer */
                   size_t source_length) /**< length of source buffer */
{
  size_t fill;
  uint32_t left;

  if (source_length == 0)
  {
    return;
  }

  left = sha1_context_p->total[0] & 0x3F;
  fill = 64 - left;

  sha1_context_p->total[0] += (uint32_t) source_length;

  /* Check overflow. */
  if (sha1_context_p->total[0] < (uint32_t) source_length)
  {
    sha1_context_p->total[1]++;
  }

  if (left && source_length >= fill)
  {
    memcpy ((void *) (sha1_context_p->buffer + left), source_p, fill);
    jerry_sha1_process (sha1_context_p, sha1_context_p->buffer);
    source_p += fill;
    source_length -= fill;
    left = 0;
  }

  while (source_length >= 64)
  {
    jerry_sha1_process (sha1_context_p, source_p);
    source_p += 64;
    source_length -= 64;
  }

  if (source_length > 0)
  {
    memcpy ((void *) (sha1_context_p->buffer + left), source_p, source_length);
  }
} /* jerry_sha1_update */

/**
 * SHA-1 final digest.
 */
static void
jerry_sha1_finish (jerry_sha1_context *sha1_context_p, /**< SHA-1 context */
                   uint8_t destination_p[20]) /**< result */
{
  uint8_t buffer[16];

  uint32_t high = (sha1_context_p->total[0] >> 29) | (sha1_context_p->total[1] << 3);
  uint32_t low = (sha1_context_p->total[0] << 3);

  uint32_t last = sha1_context_p->total[0] & 0x3F;
  uint32_t padn = (last < 56) ? (56 - last) : (120 - last);

  memset (buffer, 0, sizeof (buffer));
  buffer[0] = 0x80;

  while (padn > sizeof (buffer))
  {
    jerry_sha1_update (sha1_context_p, buffer, sizeof (buffer));
    buffer[0] = 0;
    padn -= (uint32_t) sizeof (buffer);
  }

  jerry_sha1_update (sha1_context_p, buffer, padn);

  JERRY_SHA1_PUT_UINT32_BE (high, buffer, 0);
  JERRY_SHA1_PUT_UINT32_BE (low, buffer, 4);

  jerry_sha1_update (sha1_context_p, buffer, 8);

  JERRY_SHA1_PUT_UINT32_BE (sha1_context_p->state[0], destination_p, 0);
  JERRY_SHA1_PUT_UINT32_BE (sha1_context_p->state[1], destination_p, 4);
  JERRY_SHA1_PUT_UINT32_BE (sha1_context_p->state[2], destination_p, 8);
  JERRY_SHA1_PUT_UINT32_BE (sha1_context_p->state[3], destination_p, 12);
  JERRY_SHA1_PUT_UINT32_BE (sha1_context_p->state[4], destination_p, 16);
} /* jerry_sha1_finish */

#undef JERRY_SHA1_GET_UINT32_BE
#undef JERRY_SHA1_PUT_UINT32_BE

/**
 * Computes the SHA-1 value of the combination of the two input buffers.
 */
void
jerry_debugger_compute_sha1 (const uint8_t *source1_p, /**< first part of the input */
                             size_t source1_length, /**< length of the first part */
                             const uint8_t *source2_p, /**< second part of the input */
                             size_t source2_length, /**< length of the second part */
                             uint8_t destination_p[20]) /**< result */
{
  JMEM_DEFINE_LOCAL_ARRAY (sha1_context_p, 1, jerry_sha1_context);

  jerry_sha1_init (sha1_context_p);
  jerry_sha1_update (sha1_context_p, source1_p, source1_length);
  jerry_sha1_update (sha1_context_p, source2_p, source2_length);
  jerry_sha1_finish (sha1_context_p, destination_p);

  JMEM_FINALIZE_LOCAL_ARRAY (sha1_context_p);
} /* jerry_debugger_compute_sha1 */

#endif /* JERRY_DEBUGGER */
