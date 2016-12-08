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
assert(Object.getOwnPropertyDescriptor(String.prototype.indexOf, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.indexOf, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.indexOf, 'length').writable === false);

assert(String.prototype.indexOf.length === 1);

assert("Hello world, welcome to the universe.".indexOf("welcome") === 13);

assert("Hello world, welcome to the universe.".indexOf("Hello world, welcome to the universe.") === 0);

assert("Hello world, welcome to the universe.".indexOf("welcome",10) == 13);

assert("Hello world, welcome to the universe.".indexOf("welcome",-100) == 13);

assert("Hello world, welcome to the universe.".indexOf("welcome", 15) === -1);

assert("Hello world, welcome to the universe.".indexOf("o", 15) === 17);

// check utf8 strings
assert("\uFFA2".indexOf("\uFFA2") === 0);

assert("\uFFA2".indexOf("A") === -1);

assert("w2\uFFA2A".indexOf("A") === 3);

assert("w2\u1D306A".indexOf("A") === 4);

assert("\uD834\uDF06".indexOf("\uDF06") === 1);

assert("\uD834\uDF06w2\u1D306D".indexOf("D") === 6);

assert("\ud800\dc00".indexOf("\dc00") === 1);

assert("\u8000\u0700\u8000\u8000A".indexOf("A", 3) === 4);

// check prefix search
assert("aaaabaaa".indexOf("aaaba") === 1);

// check empty string
assert(String.prototype.indexOf.call(new String()) === -1);

assert(String.prototype.indexOf.call("","") === 0);

// check NaN
assert("Hello world, welcome to the universe.".indexOf(NaN) === -1);

assert("Hello world, welcome to the universe.".indexOf("welcome",NaN) === 13);

// check Object
assert(String.prototype.indexOf.call({}) === -1);

// check +-Inf
assert("hello world!".indexOf("world", -Infinity) === 6);

assert("hello world!".indexOf("world", Infinity) === -1);

// check numbers
assert("hello world!".indexOf(-1) === -1);

assert("hello 0 world!".indexOf(-0) === 6);

// check undefined
assert("hello world!".indexOf(undefined) === -1);

var undefined_var;
assert("Hello world, welcome to the universe.".indexOf("welcome", undefined_var) === 13);

// check booleans
assert("true".indexOf(true, false) === 0);

// check coercible - undefined
try {
  String.prototype.indexOf.call(undefined);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check coercible - null
try {
  String.prototype.indexOf.call(null);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check coercible - Boolean
assert(String.prototype.indexOf.call(true, "e") === 3);
assert(String.prototype.indexOf.call(false, "e") === 4);

// check coercible - Object
var test_object = {firstName:"John", lastName:"Doe"};
assert(String.prototype.indexOf.call(test_object, "Obj") === 8);

// check coercible - Number
assert(String.prototype.indexOf.call(123, "2") === 1);
