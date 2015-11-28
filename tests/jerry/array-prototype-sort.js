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

var array = ["Peach", "Apple", "Orange", "Grape", "Cherry", "Apricot", "Grapefruit"];
array.sort();

assert(array[0] === "Apple");
assert(array[1] === "Apricot");
assert(array[2] === "Cherry");
assert(array[3] === "Grape");
assert(array[4] === "Grapefruit");
assert(array[5] === "Orange");
assert(array[6] === "Peach");

var array = [6, 4, 5, 1, 2, 9, 7, 3, 0, 8];

// Default comparison
array.sort();
for (i = 0; i < array.length; i++) {
  assert(array[i] === i);
}

// Using custom comparison function
function f(arg1, arg2) {
  if (arg1 < arg2) {
    return 1;
  } else if (arg1 > arg2) {
    return -1;
  } else {
    return 0;
  }
}

array.sort(f);
for (i = 0; i < array.length; i++) {
  assert(array[array.length - i - 1] === i);
}

// Sorting sparse array
var array = [1,,2,,3,,4,undefined,5];
var expected = [1,2,3,4,5,undefined,,,,];

array.sort();

assert(array.length === expected.length);
for (i = 0; i < array.length; i++) {
  assert(expected.hasOwnProperty (i) === array.hasOwnProperty (i));
  assert(array[i] === expected[i]);
}

// Checking behavior when provided comparefn is not callable
var obj = {};
var arr = [];
try {
  arr.sort(obj);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Checking behavior when unable to get length
var obj = { sort : Array.prototype.sort}
Object.defineProperty(obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.sort();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { sort : Array.prototype.sort, length : 1}
Object.defineProperty(obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.sort();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}
