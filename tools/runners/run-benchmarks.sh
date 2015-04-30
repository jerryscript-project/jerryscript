#!/bin/bash

# Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

ENGINE=$1

function run ()
{
    echo "Running test: $1.js"
        ./tools/perf.sh 5 $ENGINE ./tests/benchmarks/$1.js
        ./tools/rss-measure.sh $ENGINE ./tests/benchmarks/$1.js
}

echo "Running Sunspider:"
#run jerry/sunspider/3d-morph // too fast
run jerry/sunspider/bitops-3bit-bits-in-byte
run jerry/sunspider/bitops-bits-in-byte
run jerry/sunspider/bitops-bitwise-and
run jerry/sunspider/controlflow-recursive
run jerry/sunspider/math-cordic
run jerry/sunspider/math-partial-sums
run jerry/sunspider/math-spectral-norm

echo "Running Jerry:"
run jerry/cse
run jerry/cse_loop
run jerry/cse_ready_loop
run jerry/empty_loop
run jerry/function_loop
run jerry/loop_arithmetics_10kk
run jerry/loop_arithmetics_1kk

echo "Running UBench:"
run ubench/function-closure
run ubench/function-empty
run ubench/function-correct-args
run ubench/function-excess-args
run ubench/function-missing-args
run ubench/function-sum
run ubench/loop-empty-resolve
run ubench/loop-empty
run ubench/loop-sum


