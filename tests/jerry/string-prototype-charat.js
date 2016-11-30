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
assert(Object.getOwnPropertyDescriptor(String.prototype.charAt, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.charAt, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.charAt, 'length').writable === false);

assert(String.prototype.charAt.length === 1);

// check empty string
assert(String.prototype.charAt.call(new String()) === "");

// check NaN
assert("hello world!".charAt(NaN) === "h");

// check Object
assert(String.prototype.charAt.call({})  === "[");

// simple checks
assert("hello world!".charAt(0) === "h");

assert("hello world!".charAt(1) === "e");

// check +-Inf
assert("hello world!".charAt(-Infinity) === "");

assert("hello world!".charAt(Infinity) === "");

assert("hello world!".charAt(11) === "!");

assert("hello world!".charAt(12) === "");

// check unicode
assert("hello\u000B\u000C\u0020\u00A0world!".charAt(8) === "\u00A0");

assert("hello\uD834\uDF06world!".charAt(6) === "\uDF06");

assert("hell\u006F\u006F w\u006F\u006Frld!".charAt(8) === "\u006F");

assert("\u00A9\u006F".charAt(2) === "");

// check negative
assert("hello world!".charAt(-1) === "");

assert("hello world!".charAt(-9999999) === "");

assert("hello world!".charAt(-0) === "h");

// check undefined
assert("hello world!".charAt(undefined) === "h");

// check booleans
assert("hello world!".charAt(true) === "e");

assert("hello world!".charAt(false) === "h");

// check this is undefined
try {
  String.prototype.charAt.call(undefined);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check this is null
try {
  String.prototype.charAt.call(null);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check coercible - undefined
try {
  assert(true.charAt() === "");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - null
try {
  assert(String.prototype.charAt.call(null, 0) === "");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - Boolean
assert(String.prototype.charAt.call(true, 1) === "r");

// check coercible - Object
var test_object = {firstName:"John", lastName:"Doe"};
assert(String.prototype.charAt.call(test_object, 1) === "o");

// check coercible - Number
assert(String.prototype.charAt.call(123, 2) === "3");
