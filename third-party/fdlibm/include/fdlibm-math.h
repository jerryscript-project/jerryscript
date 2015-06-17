/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#ifndef JERRY_FDLIBM_MATH_H
#define JERRY_FDLIBM_MATH_H

#ifdef __cplusplus
# define EXTERN_C "C"
#else /* !__cplusplus */
# define EXTERN_C
#endif /* !__cplusplus */

// General Constants

#define INFINITY    (1.0/0.0)
#define NAN         (0.0/0.0)
#define HUGE_VAL    INFINITY

#define isnan(x)    ((x) != (x))
#define isinf(x)    (((x) == INFINITY) || ((x) == -INFINITY))
#define isfinite(x) (!(isinf(x)) && (x != NAN))

// Exponential and Logarithmic constants

#define M_E        2.7182818284590452353602874713526625
#define M_SQRT2    1.4142135623730950488016887242096981
#define M_SQRT1_2  0.7071067811865475244008443621048490
#define M_LOG2E    1.4426950408889634073599246810018921
#define M_LOG10E   0.4342944819032518276511289189166051
#define M_LN2      0.6931471805599453094172321214581765
#define M_LN10     2.3025850929940456840179914546843642

// Trigonometric Constants

#define M_PI       3.1415926535897932384626433832795029
#define M_PI_2     1.5707963267948966192313216916397514
#define M_PI_4     0.7853981633974483096156608458198757
#define M_1_PI     0.3183098861837906715377675267450287
#define M_2_PI     0.6366197723675813430755350534900574
#define M_2_SQRTPI 1.1283791670955125738961589031215452

// Trigonometric functions
extern EXTERN_C double cos(double);
extern EXTERN_C double sin(double);
extern EXTERN_C double tan(double);
extern EXTERN_C double acos(double);
extern EXTERN_C double asin(double);
extern EXTERN_C double atan(double);
extern EXTERN_C double atan2(double, double);

// Exponential and logarithmic functions
extern EXTERN_C double exp(double);
extern EXTERN_C double log(double);

// Power functions
extern EXTERN_C double pow(double, double);
extern EXTERN_C double sqrt(double);

// Rounding and remainder functions
extern EXTERN_C double ceil(double);
extern EXTERN_C double floor(double);

// Other functions
extern EXTERN_C double fabs(double);
extern EXTERN_C double fmod(double, double);

#endif /* !JERRY_FDLIBM_MATH_H */
