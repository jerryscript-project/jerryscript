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

var array = ["foo", [], Infinity, 4]
var new_arr = array.concat();

assert(new_arr.length === array.length)
for (i = 0; i < array.length; i++) {
	assert(array[i] === new_arr[i]);
}

var obj = {};
var arr1 = ["Apple", 6, "Peach"];
var arr2 = [obj, "Cherry", "Grape"];

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

// Checking behavior when unable to get length
var obj = { concat : Array.prototype.concat }
Object.defineProperty(obj, 'length', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
	obj.concat();
	assert(false);
} catch (e) {
	assert(e.message === "foo");
	assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { concat : Array.prototype.concat, length : 1 }
Object.defineProperty(obj, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
	obj.concat();
	assert(false);
} catch (e) {
	assert(e.message === "foo");
	assert(e instanceof ReferenceError);
}
