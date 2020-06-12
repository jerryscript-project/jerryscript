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

// Test with regular inputs
var array1 = Array.of(1, 2, 3, 4, 5);
assert(array1.length === 5);
assert(array1[2] === 3);

// Test with no input
var array2 = Array.of();
assert(array2.length === 0);
assert(array2[0] === undefined);

// Test when an input is another array
var array3 = Array.of(array1, 6, 7);
assert(array3.length === 3);
assert(array3[0] instanceof Array);
assert(array3[0][3] === 4);
assert(array3[2] === 7);

// Test with undefined
var array4 = Array.of(undefined);
assert(array4.length === 1);
assert(array4[0] === undefined);

// Test when input is an object
var obj = {
  0: 0,
  1: 1
};

var array5 = Array.of(obj, 2, 3);
assert(array5[0] instanceof Object);
assert(array5[0][0] === 0);
assert(array5[0][1] === 1);
assert(array5[2] === 3);

// Test with array holes
var array6 = Array.of.apply(null, [,,undefined]);
assert(array6.length === 3);
assert(array6[0] === undefined);

// Test with another class
var hits = 0;
function Test() {
    hits++;
}
Test.of = Array.of;

hits = 0;
var array6 = Test.of(1, 2);
assert(hits === 1);
assert(array6.length === 2);
assert(array6[1] === 2);

// Test with bounded builtin function
var boundedBuiltinFn = Array.of.bind(Array);
var array7 = Array.of.call(boundedBuiltinFn, boundedBuiltinFn);
assert(array7.length === 1);
assert(array7[0] === boundedBuiltinFn);

// Test superficial features
var desc = Object.getOwnPropertyDescriptor(Array, "of");
assert(desc.configurable === true);
assert(desc.writable === true);
assert(desc.enumerable === false);
assert(Array.of.length === 0);
