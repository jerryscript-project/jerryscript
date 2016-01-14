#!/bin/bash

# Copyright 2014-2016 Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ITERS="$1"
ENGINE="$2"
BENCHMARK="$3"
PRINT_MIN="$4"
OS=`uname -s | tr [:upper:] [:lower:]`

if [ "$OS" == "darwin" ]
then
  time_regexp='s/user[ 	]*\([0-9]*\)m\([0-9.]*\)s/\1 \2/g'
else
  time_regexp='s/user[ \t]*\([0-9]*\)m\([0-9.]*\)s/\1 \2/g'
fi

perf_values=$( (( for i in `seq 1 1 $ITERS`; do time $ENGINE "$BENCHMARK"; if [ $? -ne 0 ]; then exit 1; fi; done ) 2>&1 ) | \
               grep user | \
               sed "$time_regexp" | \
               awk '{ print ($1 * 60 + $2); }';
               if [ ${PIPESTATUS[0]} -ne 0 ]; then exit 1; fi; );

if [ "$PRINT_MIN" == "-min" ]
then
  perf_values=$( echo "$perf_values" | \
                 awk "BEGIN {
                        min_v = -1;
                      }
                      {
                        if (min_v == -1 || $1 < min_v) {
                          min_v = $1;
                        }
                      }
                      END {
                        print min_v
                      }" || exit 1;
               );
  calc_status=$?
else
  perf_values=$( echo "$perf_values" | \
                 awk "BEGIN {
                        n = 0
                      }
                      {
                        n++
                        a[n] = \$1
                      }
                      END {
                        #
                        # Values of 99% quantiles of two-sided t-distribution for given number of degrees of freedom
                        #
                        t_gamma_n_m1 [1]  = 63.657
                        t_gamma_n_m1 [2]  = 9.9248
                        t_gamma_n_m1 [3]  = 5.8409
                        t_gamma_n_m1 [4]  = 4.6041
                        t_gamma_n_m1 [5]  = 4.0321
                        t_gamma_n_m1 [6]  = 3.7074
                        t_gamma_n_m1 [7]  = 3.4995
                        t_gamma_n_m1 [8]  = 3.3554
                        t_gamma_n_m1 [9]  = 3.2498
                        t_gamma_n_m1 [10] = 3.1693
                        t_gamma_n_m1 [11] = 3.1058
                        t_gamma_n_m1 [12] = 3.0545
                        t_gamma_n_m1 [13] = 3.0123
                        t_gamma_n_m1 [14] = 2.9768
                        t_gamma_n_m1 [15] = 2.9467
                        t_gamma_n_m1 [16] = 2.9208
                        t_gamma_n_m1 [17] = 2.8982
                        t_gamma_n_m1 [18] = 2.8784
                        t_gamma_n_m1 [19] = 2.8609
                        t_gamma_n_m1 [20] = 2.8453
                        t_gamma_n_m1 [21] = 2.8314
                        t_gamma_n_m1 [22] = 2.8188
                        t_gamma_n_m1 [23] = 2.8073
                        t_gamma_n_m1 [24] = 2.7969
                        t_gamma_n_m1 [25] = 2.7874
                        t_gamma_n_m1 [26] = 2.7787
                        t_gamma_n_m1 [27] = 2.7707
                        t_gamma_n_m1 [28] = 2.7633
                        t_gamma_n_m1 [29] = 2.7564
                        t_gamma_n_m1 [30] = 2.75
                        t_gamma_n_m1 [31] = 2.744
                        t_gamma_n_m1 [32] = 2.7385
                        t_gamma_n_m1 [33] = 2.7333
                        t_gamma_n_m1 [34] = 2.7284
                        t_gamma_n_m1 [35] = 2.7238
                        t_gamma_n_m1 [36] = 2.7195
                        t_gamma_n_m1 [37] = 2.7154
                        t_gamma_n_m1 [38] = 2.7116
                        t_gamma_n_m1 [39] = 2.7079
                        t_gamma_n_m1 [40] = 2.7045
                        t_gamma_n_m1 [41] = 2.7012
                        t_gamma_n_m1 [42] = 2.6981
                        t_gamma_n_m1 [43] = 2.6951
                        t_gamma_n_m1 [44] = 2.6923
                        t_gamma_n_m1 [45] = 2.6896
                        t_gamma_n_m1 [46] = 2.687
                        t_gamma_n_m1 [47] = 2.6846
                        t_gamma_n_m1 [48] = 2.6822
                        t_gamma_n_m1 [49] = 2.68
                        t_gamma_n_m1 [50] = 2.6778

                        #
                        # Sort array of measurements
                        #
                        for (i = 2; i <= n; i++) {
                          j = i
                          k = a [j]
                          while (j > 1 && a [j - 1] > k) {
                            a [j] = a [j - 1]
                            j--
                          }
                          a [j] = k
                        }

                        #
                        # Remove 20% of lowest and 20% of highest values
                        #
                        n_20_percent = int (n / 5)

                        for (i = 1; i <= n_20_percent; i++) {
                          delete a[n]
                          n--
                        }

                        for (i = 1; i <= n - n_20_percent; i++) {
                          a[i] = a[i + n_20_percent]
                        }

                        n -= n_20_percent

                        #
                        # Calculate average
                        #
                        sum = 0
                        for (i = 1; i <= n; i++) {
                          sum += a[i]
                        }

                        avg = sum / n

                        if (n > 1) {
                          if (n - 1 <= 50) {
                            t_coef = t_gamma_n_m1 [n - 1]
                          } else {
                            # For greater degrees of freedom, values of corresponding quantiles
                            # are insignificantly less than the value.
                            #
                            # For example, the value for infinite number of freedoms is 2.5758
                            #
                            # So, to reduce table size, we take this, greater value,
                            # overestimating inaccuracy for no more than 4%.
                            #
                            t_coef = t_gamma_n_m1 [50]
                          }

                          #
                          # Calculate inaccuracy estimation
                          #
                          sum_delta_squares = 0
                          for (i = 1; i <= n; i++) {
                            sum_delta_squares += (avg - a[i]) ^ 2
                          }

                          delta = t_coef * sqrt (sum_delta_squares / (n * (n - 1)))

                          print avg, delta
                        } else {
                          print avg
                        }
                      }
                      " || exit 1;
               );
  calc_status=$?
fi

echo "$perf_values"

if [ $? -ne 0 ];
then
  exit 1;
fi;
