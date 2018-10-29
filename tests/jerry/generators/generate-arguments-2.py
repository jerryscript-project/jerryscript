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

file_to_open = os.path.join(data_folder, "..", "arguments-2.js")

with open(file_to_open, "w") as file:

    COUNT = 400

    random.seed(87673)

    file.write("function f_arg (arguments)\n{\n    return arguments;\n};\n")
    file.write("function f (a, b, c)\n{\n    return arguments;\n};\n")

    for i in range(COUNT):

        N1 = random.randint(-99999999999, 99999999999)
        N2 = random.randint(-99999999999, 99999999999)
        N3 = random.randint(-99999999999, 99999999999)

        file.write("assert (f_arg (" + str(N1) + ") === (" + str(N1) + "));\n")
        file.write("assert (f_arg (" + str(N1) + ") !== (" + str(int (N1 - 1)) + "));\n")
        file.write("assert (f_arg (" + str(N1) + ") !== (" + str(int(N1 + 1)) + "));\n")

    for i in range(COUNT):

        N1 = random.randint(-99999999999, 99999999999)
        N2 = random.randint(-99999999999, 99999999999)
        N3 = random.randint(-99999999999, 99999999999)
        N4 = random.randint(-99999999999, 99999999999)
        N5 = random.randint(-99999999999, 99999999999)
        if random.randint(0, 1):
            N6 = random.randint(5, 99999999999)
        else:
            N6 = random.randint(-99999999999, -1)

        file.write("args = f (" + str(N1) + ", " + str(N2) + ", " + str(N3) + ", " + str(N4) + ", " + str(N5) + ");\n")
        file.write("assert (args[0] === " + str(N1) + ");\n")
        file.write("assert (args[1] === " + str(N2) + ");\n")
        file.write("assert (args[2] === " + str(N3) + ");\n")
        file.write("assert (args[3] === " + str(N4) + ");\n")
        file.write("assert (args[4] === " + str(N5) + ");\n")
        file.write("assert (args[" + str(N6) + "] === undefined);\n")

        file.write("assert (args[0] !== " + str(int(N1 + 1)) + ");\n")
        file.write("assert (args[1] !== " + str(int(N2 + 1)) + ");\n")
        file.write("assert (args[2] !== " + str(int(N3 + 1)) + ");\n")
        file.write("assert (args[3] !== " + str(int(N4 + 1)) + ");\n")
        file.write("assert (args[4] !== " + str(int(N5 + 1)) + ");\n")

        file.write("assert (args[0] !== " + str(int(N1 - 1)) + ");\n")
        file.write("assert (args[1] !== " + str(int(N2 - 1)) + ");\n")
        file.write("assert (args[2] !== " + str(int(N3 - 1)) + ");\n")
        file.write("assert (args[3] !== " + str(int(N4 - 1)) + ");\n")
        file.write("assert (args[4] !== " + str(int(N5 - 1)) + ");\n")
