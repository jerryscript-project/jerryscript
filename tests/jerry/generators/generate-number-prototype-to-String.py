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

import string
import random

digs = string.digits + string.ascii_letters


def int2base(x, base):
    if x < 0:
        sign = -1
    elif x == 0:
        return digs[0]
    else:
        sign = 1

    x *= sign
    digits = []

    while x:
        digits.append(digs[int(x % base)])
        x = int(x / base)

    if sign < 0:
        digits.append('-')

    digits.reverse()

    return ''.join(digits)

file = open("tests/jerry/number-prototype-to-string-2.js", "w")

random.seed(532566)

for i in range(2, 37):
    for j in range(20):
        N = random.randint(-100000000, 100000000)
        file.write("assert ((" + str(N) + ").toString(" + str(i) + ") === '" + str(int2base(N, i)) + "');\n")
        file.write("assert ((" + str(N) + ").toString(" + str(i) + ") !== '" + str(int2base(N + 1, i)) + "');\n")
        file.write("assert ((" + str(N) + ").toString(" + str(i) + ") !== '" + str(int2base(N - 1, i)) + "');\n\n")
    file.write("\n")

file.close()
