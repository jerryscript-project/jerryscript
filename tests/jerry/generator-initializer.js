/* Copyright JS Foundation and other contributors, http://js.foundation
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
 */

/* This file checks core generator operations. */

function check_syntax_error (code)
{
  try {
    eval (code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

check_syntax_error ("({ * })")
check_syntax_error ("({ *, b:4 })")
check_syntax_error ("({ *a:4 })")
check_syntax_error ("({ *['a']:4 })")
check_syntax_error ("({ *a(yield) {} })")
check_syntax_error ("({ get *a() {} })")
check_syntax_error ("({ set *b(v) {} })")

check_syntax_error ("class C { * }")
check_syntax_error ("class C { static * }")
check_syntax_error ("class C { *() {} }")
check_syntax_error ("class C { static * () {} }")
check_syntax_error ("class C { *['a'] {} }")

function check_result(result, value, done)
{
  assert(result.value === value)
  assert(result.done === done)
}

function postfix(a) { return a + "b" }

var o = {
  * a () {
    yield 1
    return 2
  },
  *2(x) {
    yield x + 1
    return x + 2
  },
  *[postfix("a")]() {
    var o = { get yield() { return 3 + 2 } }

    yield o.yield
    return 6
  },
  *yield() {
    var o = { yield:7 }

    yield o.yield
    return 8
  }
}

var f = o.a()
check_result(f.next(), 1, false)
check_result(f.next(), 2, true)

var f = o[2](2)
check_result(f.next(), 3, false)
check_result(f.next(), 4, true)

var f = o.ab()
check_result(f.next(), 5, false)
check_result(f.next(), 6, true)

var f = o.yield()
check_result(f.next(), 7, false)
check_result(f.next(), 8, true)

class C {
  * a () {
    yield 1
    return 2
  }

  *3(x) {
    yield x + 1
    return x + 2
  }

  *[postfix("a")]() {
    var o = { get yield() { return 3 + 2 } }

    yield o.yield
    return 6
  }

  static *yield() {
    var o = { yield:7 }

    yield o.yield
    return 8
  }

  static * [postfix("b") ] (v = 9) {
    return v
  }
}

var c = new C

var f = c.a()
check_result(f.next(), 1, false)
check_result(f.next(), 2, true)

var f = c[3](2)
check_result(f.next(), 3, false)
check_result(f.next(), 4, true)

var f = c.ab()
check_result(f.next(), 5, false)
check_result(f.next(), 6, true)

var f = C.yield()
check_result(f.next(), 7, false)
check_result(f.next(), 8, true)

var f = C.bb()
check_result(f.next(), 9, true)
