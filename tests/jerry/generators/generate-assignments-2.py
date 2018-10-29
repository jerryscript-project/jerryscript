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
import os

data_folder = os.path.dirname(os.path.realpath(__file__))

file_to_open = os.path.join(data_folder, "..", "assignments-2.js")

with open(file_to_open, "w") as file:

    COUNT = 550

    random.seed(47563)

    for i in range(COUNT):

        N1 = random.randint(-49999999, 49999999)
        N2 = random.randint(-49999999, 49999999)
        N3 = N1 + N2

        file.write("var b = " + str(N1) + ";\n")

        file.write("assert ((b += (" + str(N2) + ")) == (" + str(N3) + "));\n")
        file.write("assert (b != (" + str(int(N3 - 1)) + "));\n")
        file.write("assert (b != (" + str(int(N3 + 1)) + "));\n")
        file.write("assert (!(b > (" + str(int(N3 + 1)) + ") || b < " + str(int(N3 - 1)) + "));\n")

        file.write("assert ((b -= (" + str(N2) + ")) == (" + str(N1) + "));\n")
        file.write("assert (b != (" + str(int(N1 - 1)) + "));\n")
        file.write("assert (b != (" + str(int(N1 + 1)) + "));\n")
        file.write("assert (!(b > (" + str(int(N1 + 1)) + ") || b < " + str(int(N1 - 1)) + "));\n")

        multiplier = random.randint(-99, 99)
        N3 = N1 * multiplier

        file.write("assert ((b *= (" + str(multiplier) + ")) == (" + str(N3) + "));\n")
        file.write("assert (b != (" + str(int(N3 - 1)) + "));\n")
        file.write("assert (b != (" + str(int(N3 + 1)) + "));\n")
        file.write("assert (!(b > (" + str(int(N3 + 1)) + ") || b < " + str(int(N3 - 1)) + "));\n")

        divisor = random.randint(-99, 99)
        while divisor == 0:
                divisor = random.randint(-99, 99)

        N3 = float(N3) / divisor

        file.write("b /= (" + str(divisor) + ");\n")
        file.write("assert (b != (" + str(int(N3 - 1)) + "));\n")
        file.write("assert (b != (" + str(int(N3 + 1)) + "));\n")
        file.write("assert (!(b > (" + str(float(N3 + 1)) + ") || b < " + str(float(N3 - 1)) + "));\n")

        N1 = random.randint(-49999999, 49999999)

        if (((N1 < 0 and divisor > 0) or (N1 > 0 and divisor < 0)) and (N1 % divisor != 0) ):
            N3 = ((N1 % divisor) - divisor)
        else:
            N3 = (N1 % divisor)

        file.write("b = (" + str(N1) + ");\n")
        file.write("assert ((b %= (" + str(divisor) + ")) == (" + str(N3) + "));\n")
        file.write("assert (b != (" + str(int(N3 - 1)) + "));\n")
        file.write("assert (b != (" + str(int(N3 + 1)) + "));\n")
        file.write("assert (!(b > (" + str(int(N3 + 1)) + ") || b < " + str(int(N3 - 1)) + "));\n")
