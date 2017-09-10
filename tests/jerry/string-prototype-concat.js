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

// check properties
assert(Object.getOwnPropertyDescriptor(String.prototype.concat, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.concat, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.concat, 'length').writable === false);

// simple checks
var s1 = "Hello ";
var s2 = "world!";
var s3 = " ";
assert(s1.concat(s2, s3, 3, 10, "  ", ".") === "Hello world! 310  .");
assert("Hello ".concat(s1) === "Hello Hello ");

assert(s1.concat(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9) === "Hello 012345678901234567890123456789");

assert("".concat() === "");

// check unicode
assert("\u0041".concat("\u0041", "\u1041") === "\u0041\u0041\u1041");
assert("\u0041\u1D306A".concat("\u0041", "\u1041") === "\u0041\u1D306A\u0041\u1041");

// check undefined
var y;
assert("Check ".concat(y) === "Check undefined");

// check toString error in this object
var y = {};
y.toString = function () { throw new ReferenceError ("foo");}
y.concat = String.prototype.concat;
try {
  y.concat("cat");
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}

// check toString error in arguments
var x = {};
x.toString = function () { throw new ReferenceError ("foo");}
try {
  "a".concat(x);
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}
