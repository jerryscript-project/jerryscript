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
 *     @(#)s_isnan.c 1.3 95/01/18
 */

#include "jerry-libm-internal.h"

/* isnan(x) returns 1 is x is nan, else 0;
 * no branching!
 */

int
isnan (double x)
{
  int hx, lx;

  hx = (__HI (x) & 0x7fffffff);
  lx = __LO (x);
  hx |= (unsigned) (lx | (-lx)) >> 31;
  hx = 0x7ff00000 - hx;
  return ((unsigned) (hx)) >> 31;
} /* isnan */
