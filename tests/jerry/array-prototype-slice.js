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

var array = [54, undefined, "Lemon", -127];

var array1 = array.slice();
var array2 = array.slice("a", "3");
var array3 = array.slice(-2);
var array4 = array.slice(-12, undefined);
var array5 = array.slice(undefined, -3);

assert (array1.length == 4);
assert (array1[0] == 54);
assert (array1[1] == undefined);
assert (array1[2] == "Lemon");
assert (array1[3] == -127);

assert (array2.length == 3);
assert (array2[0] == 54);
assert (array2[1] == undefined);
assert (array2[2] == "Lemon");

assert (array3.length == 2);
assert (array3[0] == "Lemon");
assert (array3[1] == -127);

assert (array4.length == 4);
assert (array4[0] == 54);
assert (array4[1] == undefined);
assert (array4[2] == "Lemon");
assert (array4[3] == -127);

assert (array5.length == 1);
assert (array5[0] == 54);

// Checking behavior when unable to get length
var obj = { slice : Array.prototype.slice };
Object.defineProperty(obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
	obj.slice(1, 2);
	assert (false);
} catch (e) {
	assert (e.message === "foo");
	assert (e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { length : 1, slice : Array.prototype.slice };
Object.defineProperty(obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
	obj.slice(0, 1);
	assert (false);
} catch (e) {
	assert (e.message === "foo");
	assert (e instanceof ReferenceError);
}
