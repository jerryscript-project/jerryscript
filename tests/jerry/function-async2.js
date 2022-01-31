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

/* This test checks async modifiers (nothing else). */

function check_syntax_error (code)
{
  try {
    eval (code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

check_syntax_error("function async f() {}")
check_syntax_error("(a,b) async => 1")
/* SyntaxError because arrow declaration is an assignment expression. */
check_syntax_error("async * (a,b) => 1")
check_syntax_error("({ *async f() {} })")
check_syntax_error("class C { async static f() {} }")
check_syntax_error("class C { * async f() {} }")
check_syntax_error("class C { static * async f() {} }")


function check_promise(p, value)
{
  assert(p instanceof Promise)

  p.then(function(v) {
    assert(v === value)
  })
}

var o = {
  async f() { return 1 },
  async() { return 2 },
  async *x() {}, /* Parser test, async iterators are needed to work. */
}

check_promise(o.f(), 1)
assert(o.async() === 2)

class C {
  async f() { return 3 }
  async *x() {} /* Parser test, async iterators are needed to work. */
  static async f() { return 4 }
  static async *y() {} /* Parser test, async iterators are needed to work. */
  async() { return 5 }
  static async() { return 6 }
}

var c = new C

check_promise(c.f(), 3)
check_promise(C.f(), 4)
assert(c.async() === 5)
assert(C.async() === 6)
