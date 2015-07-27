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
assert(Object.getOwnPropertyDescriptor(String.prototype.lastIndexOf, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.lastIndexOf, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.lastIndexOf, 'length').writable === false);

assert(String.prototype.lastIndexOf.length === 1);

// simple checks
assert("Hello welcome, welcome to the universe.".lastIndexOf("welcome") === 15);

assert("Hello world, welcome to the universe.".lastIndexOf("Hello world, welcome to the universe.") === 0);

assert("Hello welcome, welcome to the universe.".lastIndexOf("welcome", 5) === -1);

assert("Hello welcome, welcome to the universe.".lastIndexOf("welcome", -100) == -1);

assert("Hello welcome, welcome to the universe.".lastIndexOf("welcome", 15) === 15);

assert("Hello welcome, welcome to the universe o.".lastIndexOf("o", 10) === 10);

assert("Hello welcome, welcome to the universe o.".lastIndexOf("o", 25) === 24);

assert("Helloooo woooorld".lastIndexOf("oooo", 6) === 4);

// check empty string
assert(String.prototype.lastIndexOf.call(new String()) === -1);

assert(String.prototype.lastIndexOf.call("Hello world, welcome to the universe.","") === 37);

assert(String.prototype.lastIndexOf.call("","") === 0);

// check NaN
assert("Hello world, welcome to the universe.".lastIndexOf(NaN) === -1);

assert("Hello world, welcome to the universe.".lastIndexOf("o", NaN) === 22);

// check Object
assert(String.prototype.lastIndexOf.call({}) === -1);

// check +-Inf
assert("hello world!".lastIndexOf("world", -Infinity) === -1);

assert("hello world!".lastIndexOf("world", Infinity) === 6);

// check numbers
assert("hello world!".lastIndexOf(-1) === -1);

assert("hello 0 world!".lastIndexOf(-0) === 6);

// check undefined
assert("hello world!".lastIndexOf(undefined) === -1);

var undefined_var;
assert("Hello world, welcome to the universe.".lastIndexOf("welcome", undefined_var) === 13);

// check booleans
assert("true".lastIndexOf(true, false) === 0);

// check coercible - undefined
try {
  assert(String.prototype.lastIndexOf.call(undefined) === -1);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check coercible - null
try {
  assert(String.prototype.lastIndexOf.call(null, 0) === -1);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// check coercible - Boolean
assert(String.prototype.lastIndexOf.call(true, "e") === 3);
assert(String.prototype.lastIndexOf.call(false, "e") === 4);

// check coercible - Object
var test_object = {firstName:"John", lastName:"Doe"};
assert(String.prototype.lastIndexOf.call(test_object, "Obj") === 8);

// check coercible - Number
assert(String.prototype.lastIndexOf.call(123, "2") === 1);
