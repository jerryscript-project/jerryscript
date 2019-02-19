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

var array = ["foo", [], Infinity, 4]
var new_arr = array.concat();

assert(new_arr.length === array.length)
for (i = 0; i < array.length; i++) {
  assert(array[i] === new_arr[i]);
}

var obj = { concat : Array.prototype.concat };
var arr1 = ["Apple", 6, "Peach"];
var arr2 = [obj, "Cherry", "Grape"];

var new_array = obj.concat(arr1);
assert(new_array.length === 4);
assert(new_array[0] === obj);
assert(new_array[1] === "Apple");
assert(new_array[2] === 6);
assert(new_array[3] === "Peach");

var new_array = arr1.concat(arr2, obj, 1);

assert(new_array.length === 8);
assert(new_array[0] === "Apple");
assert(new_array[1] === 6);
assert(new_array[2] === "Peach");
assert(new_array[3] === obj);
assert(new_array[4] === "Cherry");
assert(new_array[5] === "Grape");
assert(new_array[6] === obj);
assert(new_array[7] === 1);

var arr1 = [1,2];
var arr2 = [4,5,6,7,8];
var arr3 = [,,9,10];
var arr4 = [];
var expected = [1,2,4,5,6,7,8,,,9,10];

var result = arr1.concat(arr2, arr3, arr4);

assert(result.length === expected.length)
for (i = 0; i < result.length; i++) {
  assert(result[i] === expected[i]);
}

var arr1 = [];
arr1.length = 2;
var arr2 = [];
arr2.length = 3;
assert(arr1.concat(arr2).length === arr1.length + arr2.length);

// Checking behavior when unable to get element
var arr = []
Object.defineProperty(arr, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });
arr.length = 1;

try {
  arr.concat();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.4.5.
   Checking behavior when unable to get element from a given array */
arr1 = [];
arr2 = [];
arr3 = [];
Object.defineProperty(arr2, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  arr1.concat(arr2, arr3);
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}
