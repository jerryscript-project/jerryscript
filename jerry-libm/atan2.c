/* Copyright 2016 Samsung Electronics Co., Ltd.
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
 *
 * This file is based on work under the following copyright and permission
 * notice:
 *
 *     Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 *     Developed at SunSoft, a Sun Microsystems, Inc. business.
 *     Permission to use, copy, modify, and distribute this
 *     software is freely granted, provided that this notice
 *     is preserved.
 *
 *     @(#)e_atan2.c 1.3 95/01/18
 */

#include "jerry-libm-internal.h"

/* atan2(y,x)
 *
 * Method:
 *      1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *      2. Reduce x to positive by (if x and y are unexceptional):
 *              ARG (x+iy) = arctan(y/x)           ... if x > 0,
 *              ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *      ATAN2((anything), NaN ) is NaN;
 *      ATAN2(NAN , (anything) ) is NaN;
 *      ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *      ATAN2(+-0, -(anything but NaN)) is +-pi ;
 *      ATAN2(+-(anything but 0 and NaN), 0) is +-pi/2;
 *      ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *      ATAN2(+-(anything but INF and NaN), -INF) is +-pi;
 *      ATAN2(+-INF,+INF ) is +-pi/4 ;
 *      ATAN2(+-INF,-INF ) is +-3pi/4;
 *      ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-pi/2;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#define tiny   1.0e-300
#define zero   0.0
#define pi_o_4 7.8539816339744827900E-01 /* 0x3FE921FB, 0x54442D18 */
#define pi_o_2 1.5707963267948965580E+00 /* 0x3FF921FB, 0x54442D18 */
#define pi     3.1415926535897931160E+00 /* 0x400921FB, 0x54442D18 */
#define pi_lo  1.2246467991473531772E-16 /* 0x3CA1A626, 0x33145C07 */

double
atan2 (double y, double x)
{
  double z;
  int k, m, hx, hy, ix, iy;
  unsigned lx, ly;

  hx = __HI (x);
  ix = hx & 0x7fffffff;
  lx = __LO (x);
  hy = __HI (y);
  iy = hy & 0x7fffffff;
  ly = __LO (y);
  if (((ix | ((lx | -lx) >> 31)) > 0x7ff00000) || ((iy | ((ly | -ly) >> 31)) > 0x7ff00000)) /* x or y is NaN */
  {
    return x + y;
  }
  if (((hx - 0x3ff00000) | lx) == 0) /* x = 1.0 */
  {
    return atan (y);
  }
  m = ((hy >> 31) & 1) | ((hx >> 30) & 2); /* 2 * sign(x) + sign(y) */

  /* when y = 0 */
  if ((iy | ly) == 0)
  {
    switch (m)
    {
      case 0:
      case 1:
      {
        return y; /* atan(+-0,+anything) = +-0 */
      }
      case 2:
      {
        return pi + tiny; /* atan(+0,-anything) = pi */
      }
      case 3:
      {
        return -pi - tiny; /* atan(-0,-anything) = -pi */
      }
    }
  }
  /* when x = 0 */
  if ((ix | lx) == 0)
  {
    return (hy < 0) ? -pi_o_2 - tiny : pi_o_2 + tiny;
  }

  /* when x is INF */
  if (ix == 0x7ff00000)
  {
    if (iy == 0x7ff00000)
    {
      switch (m)
      {
        case 0: /* atan(+INF,+INF) */
        {
          return pi_o_4 + tiny;
        }
        case 1: /* atan(-INF,+INF) */
        {
          return -pi_o_4 - tiny;
        }
        case 2: /* atan(+INF,-INF) */
        {
          return 3.0 * pi_o_4 + tiny;
        }
        case 3: /* atan(-INF,-INF) */
        {
          return -3.0 * pi_o_4 - tiny;
        }
      }
    }
    else
    {
      switch (m)
      {
        case 0: /* atan(+...,+INF) */
        {
          return zero;
        }
        case 1: /* atan(-...,+INF) */
        {
          return -zero;
        }
        case 2: /* atan(+...,-INF) */
        {
          return pi + tiny;
        }
        case 3: /* atan(-...,-INF) */
        {
          return -pi - tiny;
        }
      }
    }
  }
  /* when y is INF */
  if (iy == 0x7ff00000)
  {
    return (hy < 0) ? -pi_o_2 - tiny : pi_o_2 + tiny;
  }

  /* compute y / x */
  k = (iy - ix) >> 20;
  if (k > 60) /* |y / x| > 2**60 */
  {
    z = pi_o_2 + 0.5 * pi_lo;
  }
  else if (hx < 0 && k < -60) /* |y| / x < -2**60 */
  {
    z = 0.0;
  }
  else /* safe to do y / x */
  {
    z = atan (fabs (y / x));
  }
  switch (m)
  {
    case 0: /* atan(+,+) */
    {
      return z;
    }
    case 1: /* atan(-,+) */
    {
      __HI (z) ^= 0x80000000;
      return z;
    }
    case 2: /* atan(+,-) */
    {
      return pi - (z - pi_lo);
    }
    /* case 3: */
    default: /* atan(-,-) */
    {
      return (z - pi_lo) - pi;
    }
  }
} /* atan2 */

#undef tiny
#undef zero
#undef pi_o_4
#undef pi_o_2
#undef pi
#undef pi_lo
