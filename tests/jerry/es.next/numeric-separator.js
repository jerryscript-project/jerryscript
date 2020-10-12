// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// few valid numeric separators
assert (1_1 == 11)
assert (0x11_11 == 4369)
assert (0b11_11 == 15)
assert (0o11_11 == 585)
assert (1_1n == 11n)
assert (100_001.11_00 == 100001.11)
assert (0x11_11n == 4369n)

// checker functions

function invalid_numeric_separator (expression)
{
  try {
    var a = eval (expression)
    assert (false)
  } catch (e) {
    assert (true)
  }
}

// few invalid numeric separators
invalid_numeric_separator ("_1_1")
invalid_numeric_separator ("1__1")
invalid_numeric_separator ("1_1_n")
invalid_numeric_separator ("1_1n_")
invalid_numeric_separator ("0b_11")
invalid_numeric_separator ("0x_11")
invalid_numeric_separator ("0o_11")
invalid_numeric_separator ("0_0.123")
