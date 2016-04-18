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
 */

#ifndef JERRY_LIBC_TIME_H
#define JERRY_LIBC_TIME_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Time value structure
 */
struct timeval
{
  unsigned long tv_sec;   /**< seconds */
  unsigned long tv_usec;  /**< microseconds */
};

/**
 * Timezone structure
 */
struct timezone
{
  int tz_minuteswest;     /**< minutes west of Greenwich */
  int tz_dsttime;         /**< type of DST correction */
};

int gettimeofday (void *tp, void *tzp);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_LIBC_TIME_H */
