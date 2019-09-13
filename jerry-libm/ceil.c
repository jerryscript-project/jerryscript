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
 *     @(#)s_ceil.c 1.3 95/01/18
 */

#include "jerry-libm-internal.h"

/* ceil(x)
 * Return x rounded toward -inf to integral value
 *
 * Method:
 *      Bit twiddling.
 *
 * Exception:
 *      Inexact flag raised if x not equal to ceil(x).
 */

#define huge 1.0e300

double
ceil (double x)
{
  int i0, i1, j0;
  unsigned i, j;

  i0 = __HI (x);
  i1 = __LO (x);
  j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
  if (j0 < 20)
  {
    if (j0 < 0) /* raise inexact if x != 0 */
    {
      if (huge + x > 0.0) /* return 0 * sign(x) if |x| < 1 */
      {
        if (i0 < 0)
        {
          i0 = 0x80000000;
          i1 = 0;
        }
        else if ((i0 | i1) != 0)
        {
          i0 = 0x3ff00000;
          i1 = 0;
        }
      }
    }
    else
    {
      i = (0x000fffff) >> j0;
      if (((i0 & i) | i1) == 0) /* x is integral */
      {
        return x;
      }
      if (huge + x > 0.0) /* raise inexact flag */
      {
        if (i0 > 0)
        {
          i0 += (0x00100000) >> j0;
        }
        i0 &= (~i);
        i1 = 0;
      }
    }
  }
  else if (j0 > 51)
  {
    if (j0 == 0x400) /* inf or NaN */
    {
      return x + x;
    }
    else /* x is integral */
    {
      return x;
    }
  }
  else
  {
    i = ((unsigned) (0xffffffff)) >> (j0 - 20);
    if ((i1 & i) == 0) /* x is integral */
    {
      return x;
    }
    if (huge + x > 0.0) /* raise inexact flag */
    {
      if (i0 > 0)
      {
        if (j0 == 20)
        {
          i0 += 1;
        }
        else
        {
          j = i1 + (1 << (52 - j0));
          if (j < i1) /* got a carry */
          {
            i0 += 1;
          }
          i1 = j;
        }
      }
      i1 &= (~i);
    }
  }

  double_accessor ret;
  ret.as_int.hi = i0;
  ret.as_int.lo = i1;
  return ret.dbl;
} /* ceil */

#undef huge
