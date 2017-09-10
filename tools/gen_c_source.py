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


def format_code(code, indent, digit_number=4):
    def regroup(list_to_group, num):
        return [list_to_group[i:i+num] for i in range(0, len(list_to_group), num)]

    def hex_format(char, digit_number):
        if isinstance(char, str):
            char = ord(char)

        return ("0x{:0%sx}" % digit_number).format(char)

    lines = []

    nums_per_line = 10
    width = nums_per_line * (digit_number + 4)
    # convert all characters to hex format
    converted_code = [hex_format(char, digit_number) for char in code]
    # 10 hex number per line
    for line in regroup(", ".join(converted_code), width):
        lines.append(('  ' * indent) + line.strip())
    return "\n".join(lines)
