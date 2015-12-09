// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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
assert(Object.getOwnPropertyDescriptor(String.prototype.charCodeAt, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.charCodeAt, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.charCodeAt, 'length').writable === false);

assert(String.prototype.charCodeAt.length === 1);

// check empty string
assert(isNaN(String.prototype.charCodeAt.call(new String())));

// check Object with NaN pos
assert(String.prototype.charCodeAt.call({}) === 91);

// simple checks
assert("hello world!".charCodeAt(0) === 104);

assert("hello world!".charCodeAt(1) === 101);

assert("HELLO WORLD".charCodeAt(10) === 68);

// check +-Inf
assert(isNaN("hello world!".charCodeAt(-Infinity)));

assert(isNaN("hello world!".charCodeAt(Infinity)));

assert("hello world!".charCodeAt(11) === 33);

assert(isNaN("hello world!".charCodeAt(12)));

// check unicode
assert("hello\u000B\u000C\u0020\u00A0world!".charCodeAt(8) === 160);

assert("hello\uD834\uDF06world!".charCodeAt(6) === 57094);

assert("hell\u006F\u006F w\u006F\u006Frld!".charCodeAt(8) === 111);

assert(isNaN("\u00A9\u006F".charCodeAt(2)));

// check negative
assert(isNaN("hello world!".charCodeAt(-1)));

assert(isNaN("hello world!".charCodeAt(-9999999)));

assert("hello world!".charCodeAt(-0) === 104);

// check undefined
assert("hello world!".charCodeAt(undefined) === 104);

// check booleans
assert("hello world!".charCodeAt(true) === 101);

assert("hello world!".charCodeAt(false) === 104);

// check index above uint32_t
assert(isNaN("hello world!".charCodeAt(4294967299)));

// check coercible - undefined
try {
  assert(isNaN(String.prototype.charCodeAt.call(undefined)));
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - null
try {
  assert(isNaN(String.prototype.charCodeAt.call(null, 0)));
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - Boolean
assert(String.prototype.charCodeAt.call(true, 1) === 114);
assert(String.prototype.charCodeAt.call(true) === 116);

// check coercible - Object
var test_object = {firstName:"John", lastName:"Doe"};
assert(String.prototype.charCodeAt.call(test_object, 1) === 111);

// check coercible - Number
assert(String.prototype.charCodeAt.call(123, 2) === 51);
