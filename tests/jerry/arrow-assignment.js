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

var y = 0
var prot = Object.getPrototypeOf(/ /)

prot.setY = function (v) { y = v }

assert(y === 0)
// Since arrow function is an assignment expression, this affects certain constructs
var f = x => {}
/ /.setY(5)
assert(y === 5)

var s
// This is not a function call
assert(eval("s = x => { return 1 }\n(3)") === 3)
assert(typeof s === "function")

// This is a function call
assert(eval("s = function () { return 1 }\n(3)") === 1)
assert(s === 1)

var f = 5 ? x => 1 : x => 2
assert(f() === 1)

var f = [x => 2][0]
assert(f() === 2)

var f = 123; f += x => y
assert(typeof f === "string")

// Comma operator
assert(eval("x => {}, 5") === 5)
