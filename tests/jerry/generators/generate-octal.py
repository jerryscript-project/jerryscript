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

file = open("tests/jerry/octal.js", "w")

random.seed(532566)

for i in range(1000):
	N = random.randint(-99999999999, 99999999999)
	file.write("assert (" + str(N) + " === " + str(oct(N)) + ");\n")
	file.write("assert (" + str(N) + " !== " + str(oct(N + 1)) + ");\n")
	file.write("assert (" + str(N) + " !== " + str(oct(N - 1)) + ");\n")
	file.write("assert (typeof " + str(N) + " === 'number');\n")
	file.write("assert (typeof " + str(oct(N - 1)) + " === 'number');\n\n")


file.write("assert (Number.MAX_VALUE != Number.MIN_VALUE);\n")

file.close()
