#!/usr/bin/env python

# Copyright JS Foundation and other contributors, http://js.foundation
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

import random

file = open("tests/jerry/math-abs-2.js", "w")

random.seed(532566)

for i in range(1000):
    N = random.randint(-99999999999, 99999999999)
    ABS = N
    if (N < 0):
        file.write("assert (Math['abs'](" + str(N) + ") !== " + str(ABS) + ");\n")
        ABS = -N

    file.write("assert (Math['abs'](" + str(N) + ") === " + str(ABS) + ");\n")
    file.write("assert (Math['abs'](" + str(N) + ") !== " + str(int(ABS - 1)) + ");\n")
    file.write("assert (Math['abs'](" + str(N) + ") !== " + str(int(ABS + 1)) + ");\n\n")

file.close()
