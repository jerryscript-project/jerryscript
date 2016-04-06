
/* @(#)fdlibm.h 1.5 04/04/22 */
/*
 * ====================================================
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* Sometimes it's necessary to define __LITTLE_ENDIAN explicitly
   but these catch some common cases. */

#if (defined (i386) || defined (__i386) || defined (__i386__) || \
     defined (i486) || defined (__i486) || defined (__i486__) || \
     defined (intel) || defined (x86) || defined (i86pc) || \
     defined (__alpha) || defined (__osf__) || \
     defined (__x86_64__) || defined (__arm__) || defined (__aarch64__))
#define __LITTLE_ENDIAN
#endif

#ifdef __LITTLE_ENDIAN
#define __HI(x) *(1 + (int *) &x)
#define __LO(x) *(int *) &x
#else
#define __HI(x) *(int *) &x
#define __LO(x) *(1 + (int *) &x)
#endif

/*
 * ANSI/POSIX
 */

#define MAXFLOAT        ((float) 3.40282346638528860e+38)

#define HUGE            MAXFLOAT

/*
 * set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
 * (one may replace the following line by "#include <values.h>")
 */

#define X_TLOSS         1.41484755040568800000e+16

#define DOMAIN          1
#define SING            2
#define OVERFLOW        3
#define UNDERFLOW       4
#define TLOSS           5
#define PLOSS           6

/*
 * ANSI/POSIX
 */
extern double acos (double);
extern double asin (double);
extern double atan (double);
extern double atan2 (double, double);
extern double cos (double);
extern double sin (double);
extern double tan (double);

extern double exp (double);
extern double log (double);

extern double pow (double, double);
extern double sqrt (double);

extern double ceil (double);
extern double fabs (double);
extern double floor (double);
extern double fmod (double, double);

extern int isnan (double);
extern int finite (double);

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
extern double copysign (double, double);
extern double scalbn (double, int);
