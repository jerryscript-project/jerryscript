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


LICENSE = """/* Copyright JS Foundation and other contributors, http://js.foundation
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
 */"""


class UniCodeSource(object):
    def __init__(self, filepath):
        self.__filepath = filepath
        self.__header = [LICENSE, ""]
        self.__data = []

    def complete_header(self, completion):
        self.__header.append(completion)
        self.__header.append("")  # for an extra empty line

    def add_table(self, table, table_name, table_type, table_descr):
        self.__data.append(table_descr)
        self.__data.append("static const %s jerry_%s[] JERRY_CONST_DATA =" % (table_type, table_name))
        self.__data.append("{")
        self.__data.append(format_code(table, 1))
        self.__data.append("};")
        self.__data.append("")  # for an extra empty line

    def generate(self):
        with open(self.__filepath, 'w') as generated_source:
            generated_source.write("\n".join(self.__header))
            generated_source.write("\n".join(self.__data))


def regroup(list_to_group, num):
    return [list_to_group[i:i+num] for i in range(0, len(list_to_group), num)]


def hex_format(char, digit_number):
    if isinstance(char, str):
        char = ord(char)

    return ("0x{:0%sx}" % digit_number).format(char)


def format_code(code, indent, digit_number=4):
    lines = []

    nums_per_line = 10
    width = nums_per_line * (digit_number + 4)
    # convert all characters to hex format
    converted_code = [hex_format(char, digit_number) for char in code]
    # 10 hex number per line
    for line in regroup(", ".join(converted_code), width):
        lines.append(('  ' * indent) + line.strip())
    return "\n".join(lines)
