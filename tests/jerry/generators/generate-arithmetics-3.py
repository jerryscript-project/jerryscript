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

file_to_open = os.path.join(data_folder, "..", "arithmetics-3.js")

with open(file_to_open, "w") as file:

    COUNT = 600

    random.seed(34673)

    for i in range(COUNT):

        N1 = random.randint(-49999999999, 49999999999)
        N2 = random.randint(-49999999999, 49999999999)

        file.write("assert ((" + str(N1) + ") + (" + str(N2) + ") == (" + str(int(N1 + N2)) + "));\n")
        file.write("assert ((" + str(N1) + ") + (" + str(N2) + ") != (" + str(int(N1 + N2 - 1)) + "));\n")
        file.write("assert ((" + str(N1) + ") + (" + str(N2) + ") != (" + str(int(N1 + N2 + 1)) + "));\n\n")

        file.write("assert ((" + str(N1) + ") - (" + str(N2) + ") == (" + str(int(N1 - N2)) + "));\n")
        file.write("assert ((" + str(N1) + ") - (" + str(N2) + ") != (" + str(int(N1 - N2 - 1)) + "));\n")
        file.write("assert ((" + str(N1) + ") - (" + str(N2) + ") != (" + str(int(N1 - N2 + 1)) + "));\n\n")

    for i in range(COUNT):

        N1 = random.randint(1, 99999)
        N2 = random.randint(1, 99999)

        file.write("assert ((" + str(N1) + ") % (" + str(N2) + ") == (" + str(int(N1 % N2)) + "));\n")
        file.write("assert ((" + str(N1) + ") % (" + str(N2) + ") != (" + str(int((N1 % N2) - 1)) + "));\n")
        file.write("assert ((" + str(N1) + ") % (" + str(N2) + ") != (" + str(int((N1 % N2) + 1)) + "));\n\n")

    for i in range(COUNT):

        N1 = random.randint(-9999, 9999)
        N2 = random.randint(-9999, 9999)

        file.write("assert ((" + str(N1) + ") * (" + str(N2) + ") == (" + str(int(N1 * N2)) + "));\n")
        file.write("assert ((" + str(N1) + ") * (" + str(N2) + ") != (" + str(int((N1 * N2) - 1)) + "));\n")
        file.write("assert ((" + str(N1) + ") * (" + str(N2) + ") != (" + str(int((N1 * N2) + 1)) + "));\n\n")
