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

/* Declaring let */

/* Note: l\u0065t is let */

function check_syntax_error (code) {
  try {
    eval(code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

function check_strict_syntax_error (code) {
  "use strict"

  try {
    eval(code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

check_syntax_error("let let = 5")
check_syntax_error("const [let] = [1]")
check_syntax_error("const l\u0065t = 6")
check_syntax_error("let [l\u0065t] = [2]")
check_syntax_error("l\\u0065t a")
check_strict_syntax_error("var let = 5")
check_strict_syntax_error("function let() {}")
check_strict_syntax_error("for (let in []) ;")
check_strict_syntax_error("l\\u0065t;")

var let = 1
assert(let === 1)

var [let] = [2]
assert(let === 2)

var x = 0;
let = [ () => x = 1 ]

l\u0065t[0]()
assert(x === 1)

function f1()
{
  var a = 0

  function let(l\u0065t) {
    a = let
  }

  let(3)

  assert(a === 3)
}
f1()

function f2()
{
  var let = [1]

  /* First: destructuring pattern definition */
  let
  [a] = [2]

  assert(a === 2)

  a = 0

  /* Second: property access */
  l\u0065t
  [a] = [3]

  assert(let[0][0] === 3)
}
f2()

var arr = []

for (let in ["x","y"])
  arr.push(let)

assert(arr[0] === "0")
assert(arr[1] === "1")

/* Let and arrow */

for (let => 4; false ; ) ;

let => 5

/* Let label */
let: break let
