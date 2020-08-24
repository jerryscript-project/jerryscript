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

var test = "bar"

assert(test.padStart("5", "foo") === "fobar")
assert(test.padStart(6, "foo") === "foobar")
assert(test.padStart(8, '1234')=== "12341bar")
assert(test.padEnd(5, "baz") === "barba")
assert(test.padEnd(6, "baz") === "barbaz")
assert(test.padEnd(8, '1234')=== "bar12341")

// Check for negative value
assert(test.padStart(-5, "foo") === "bar")
assert(test.padEnd(-5, "foo") === "bar")

// 21.1.3.15.1.6
assert(test.padStart(10) === "       bar")
assert(test.padEnd(10) === "bar       ")

// Empty FilString
assert(test.padEnd(10, "") === "bar")
assert(test.padStart(10, "") === "bar")

// Check with unicode surrogate characters
// 𐋀 = [55296, 57024]
var unicode_padded = "a".padStart(4 ,"𐋀")
assert(unicode_padded.charCodeAt(0) == 55296)
assert(unicode_padded.charCodeAt(1) == 57024)
assert(unicode_padded.charCodeAt(2) == 55296)
assert(unicode_padded.charCodeAt(3) == 97)

unicode_padded = "a".padEnd(4 ,"𐋀")
assert(unicode_padded.charCodeAt(0) == 97)
assert(unicode_padded.charCodeAt(1) == 55296)
assert(unicode_padded.charCodeAt(2) == 57024)
assert(unicode_padded.charCodeAt(3) == 55296)

// 𐋂 = [55296, 57026]
unicode_padded = "𐋂".padStart(4 ,"𐋀")
assert(unicode_padded.charCodeAt(0) == 55296)
assert(unicode_padded.charCodeAt(1) == 57024)
assert(unicode_padded.charCodeAt(2) == 55296)
assert(unicode_padded.charCodeAt(3) == 57026)

// Check for errors in length
try {
  test.padStart(Symbol("Will this fail?"), "It should" )
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
try {
  test.padEnd(Symbol("Will this fail?"), "It should" )
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
// Check for errors in fillString
try {
  test.padStart(10, Symbol("Fail, this should. " ))
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
try {
  test.padEnd(10, Symbol("Fail, this should. " ))
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
