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
assert(f2.length === 0)

function f3(a = 1, b = 1 + 1, c,) {}
assert(f3.length === 0)

var f4 = async(a,) => {}
assert(f4.length === 1)

var f5 = async(a = 1,b,) => {}
assert(f5.length === 0)

assert(((a, b = 1 + 1, c,) => {}).length === 1)

assert(((a = 1, b, c = 1 + 1,) => {}).length === 0)

function f6([a=1, b], [c, [d = 5] = []], e, [{f} = {}, g],) {}
assert(f6.length === 4)

function f7([a, {b = 1, 4 : c = 2} = {}], {'cc' : d = 5, e}, f, {g, h} = a = {},) {}
assert(f7.length === 3)

function f8(a, [b] = [], ...e) {}
assert(f8.length === 1)
