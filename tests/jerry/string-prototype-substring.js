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
assert(Object.getOwnPropertyDescriptor(String.prototype.substring, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.substring, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.substring, 'length').writable === false);

assert(String.prototype.substring.length === 2);

assert(String.prototype.substring.call(new String()) === "");

assert(String.prototype.substring.call({}) === "[object Object]");

// check this is undefined
try {
  String.prototype.substring.call(undefined);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check this is null
try {
  String.prototype.substring.call(null);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// simple checks
assert("hello world!".substring(0, 11) === "hello world");

assert("hello world!".substring(11, 0) === "hello world");

assert("hello world!".substring(0, 12) === "hello world!");

assert("hello world!".substring(12, 0) === "hello world!");

// check NaN
assert("hello world!".substring(NaN, 12) === "hello world!");

// check NaN
assert("hello world!".substring(2, NaN) === "he");

// check end undefined
assert("hello world!".substring(2, undefined) === "llo world!");

// check negative
assert("hello world!".substring(-1,8) === "hello wo");

// check negative
assert("hello\tworld!".substring(5,-8) === "hello");

// check negative
assert("hello world!".substring(-1,-8) === "");

// check ranges
assert("hello world!".substring(-1,10000) === "hello world!");

assert("hello world!".substring(10000,1000000) === "");

assert("hello world!".substring(100000,1) === "ello world!");

// check both undefined
assert("hello world!".substring(undefined, undefined) === "hello world!");

var undef_var;
assert("hello world!".substring(undef_var, undef_var) === "hello world!");

// check integer conversion
assert("hello world!".substring(undefined, 5) === "hello");

assert("hello world!".substring(undefined, "bar") === "");

assert("hello world!".substring(2, true) === "e");

assert("hello world!".substring(2, false) === "he");

assert("hello world!".substring(5, obj) === " world!");

// check other objects
var obj = { substring : String.prototype.substring }

obj.toString = function() {
    return "Iam";
}
assert(obj.substring(100000,1) === "am");

obj.toString = function() {
  throw new ReferenceError ("foo");
};

try {
  assert(obj.substring(100000,1));
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// check coercible - undefined
try {
  assert(true.substring() === "");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - null
try {
  assert(String.prototype.substring.call(null, 0, 1) === "");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - Boolean
assert(String.prototype.substring.call(true, 0, 1) === "t");

// check coercible - Object
var test_object = {firstName:"John", lastName:"Doe"};
assert(String.prototype.substring.call(test_object, 0, 7) === "[object");

// check coercible - Number
assert(String.prototype.substring.call(123, 0, 3) === "123");
