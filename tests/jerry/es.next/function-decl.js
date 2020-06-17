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

function check_syntax_error (code)
{
  try {
    eval (code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

check_syntax_error ("function f(,) {}")
check_syntax_error ("function f(...a,) {}")
check_syntax_error ("function f(a = 1 + 1,,) {}")
check_syntax_error ("function f(a,,b) {}")
check_syntax_error ("function f(,a) {}")

function f1(a,) {}
assert(f1.length === 1)

function f2(a = 1,) {}
assert(f2.length === 1)

function f3(a = 1, b = 1 + 1, c,) {}
assert(f3.length === 3)

var f4 = async(a,) => {}
assert(f4.length === 1)

var f5 = async(a = 1,) => {}
assert(f5.length === 1)

assert(((a = 1, b = 1 + 1, c,) => {}).length === 3)

assert(((a = 1, b, c = 1 + 1,) => {}).length === 3)
