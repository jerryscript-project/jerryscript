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
assert(Object.getOwnPropertyDescriptor(String.prototype.substr, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.substr, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.substr, 'length').writable === false);

assert(String.prototype.substr.length === 2);

assert(String.prototype.substr.call(new String()) === "");

assert(String.prototype.substr.call({}) === "[object Object]");

// check this is undefined
try {
  String.prototype.substr.call(undefined);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check this is null
try {
  String.prototype.substr.call(null);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// simple checks
assert("Hello world!".substr(0, 11) === "Hello world");

assert("Hello world!".substr(11, 0) === "");

assert("Hello world!".substr(0, 12) === "Hello world!");

assert("Hello world!".substr(12, 0) === "");
// check NaN
assert("Hello world!".substr(NaN, 12) === "Hello world!");

// check NaN
assert("Hello world!".substr(2, NaN) === "");

// check end undefined
assert("Hello world!".substr(2, undefined) === "llo world!");

// check negative
assert("Hello world!".substr(-1,8) === "!");

// check negative
assert("Hello\tworld!".substr(5,-8) === "");

// check negative
assert("Hello world!".substr(-1,-8) === "");

// check ranges
assert("Hello world!".substr(-1,10000) === "!");

assert("Hello world!".substr(10000,1000000) === "");

assert("Hello world!".substr(100000,1) === "");

// check both undefined
assert("Hello world!".substr(undefined, undefined) === "Hello world!");

var undef_var;
assert("Hello world!".substr(undef_var, undef_var) === "Hello world!");

// check integer conversion
assert("Hello world!".substr(undefined, 5) === "Hello");

assert("Hello world!".substr(undefined, "bar") === "");

assert("Hello world!".substr(2, true) === "l");

assert("Hello world!".substr(2, false) === "");

assert("Hello world!".substr(5, obj) === " world!");

// check other objects
var obj = { substr : String.prototype.substr }

obj.toString = function() {
    return "Iam";
}
assert(obj.substr(0,1) === "I");

obj.toString = function() {
  throw new ReferenceError ("foo");
};

try {
  assert(obj.substr(100000,1));
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// check coercible - undefined
try {
  assert(true.substr() === "");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - null
try {
  assert(String.prototype.substr.call(null, 0, 1) === "");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - Boolean
assert(String.prototype.substr.call(true, 0, 1) === "t");

// check coercible - Object
var test_object = {firstName:"John", lastName:"Doe"};
assert(String.prototype.substr.call(test_object, 0, 7) === "[object");

// check coercible - Number
assert(String.prototype.substr.call(123, 0, 3) === "123");
