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

var two = 2
var three = 3

// Precedence (right-to-left) tests
assert(2 ** 3 ** 2 === 512)
assert((2 ** 3) ** 2 === 64)
assert(2 ** (3 ** 2) === 512)
assert(two ** three ** two === 512)
assert((two ** three) ** two === 64)
assert(two ** (three ** two) === 512)

assert(2 ** 2 * 3 ** 3 === 4 * 27)
assert(two ** two * three ** three === 4 * 27)

var a = 3
assert((a **= 3) === 27)
assert(a === 27)

a = 2
assert((a **= a **= 2) === 16)
assert(a === 16)

function assertThrows(src) {
  try {
    eval(src)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

assertThrows("-2 ** 2")
assertThrows("+a ** 2")
assertThrows("!a ** 2")
assertThrows("~a ** 2")
assertThrows("void a ** 2")
assertThrows("typeof a ** 2")
assertThrows("delete a ** 2")
assertThrows("!(-a) ** 2")

assert((-2) ** 2 === 4)

a = 3
assert((-+-a) ** 3 === 27)

a = 0
assert((!a) ** 2 === 1)

a = 0
assert(isNaN((void a) ** 3))

a = -4
assert(++a ** 2 === 9)
assert(a === -3)

a = -2
assert(a-- ** 2 === 4)
assert(a === -3)
