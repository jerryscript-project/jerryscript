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

file = open("tests/jerry/relational-2.js", "w")

random.seed(532566)

for i in range(1000):
    N = random.randint(-99999999999, 99999999999)
    N2 = random.randint(-99999999999, 99999999999)
    REL = ""

    if (N >= N2):
        REL = ">="
    else:
        REL = "<="

    file.write("assert (" + str(N) + " " + REL + " " + str(N2) + ");\n")

file.close()
