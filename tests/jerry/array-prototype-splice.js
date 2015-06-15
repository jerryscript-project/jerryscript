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

function setDefaultValues()
{
	return [54, undefined, -127, "sunshine"];
}

var array = setDefaultValues();
var array1 = array.splice();

assert (array.length == 4);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array[2] == -127);
assert (array[3] == "sunshine");
assert (array1.length == 0);

// --------------------------------------------------------
array = setDefaultValues();
var array2 = array.splice(2);

assert (array.length == 2);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array2.length == 2);
assert (array2[0] == -127);
assert (array2[1] == "sunshine");

// --------------------------------------------------------
array = setDefaultValues();
var array3 = array.splice(2, 1);

assert (array.length == 3);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array[2] == "sunshine");
assert (array3.length == 1);
assert (array3[0] == -127);

// --------------------------------------------------------
array = setDefaultValues();
var array4 = array.splice(0, 3, 6720, "Szeged");

assert (array.length == 3);
assert (array[0] == 6720);
assert (array[1] == "Szeged");
assert (array[2] == "sunshine");
assert (array4.length == 3);
assert (array4[0] == 54);
assert (array4[1] == undefined);
assert (array4[2] == -127);

// --------------------------------------------------------
array = setDefaultValues();
var array5 = array.splice(-2, -2, 6720, "Szeged");

assert (array.length == 6);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array[2] == 6720);
assert (array[3] == "Szeged");
assert (array[4] == -127);
assert (array[5] == "sunshine");
assert (array5.length == 0);

// --------------------------------------------------------
array = setDefaultValues();
var array6 = array.splice(undefined, undefined, undefined);

assert (array.length == 5);
assert (array[0] == undefined);
assert (array[1] == 54);
assert (array[2] == undefined);
assert (array[3] == -127);
assert (array[4] == "sunshine");
assert (array6.length == 0);

// Checking behavior when unable to get length
var obj = {splice : Array.prototype.splice};
Object.defineProperty(obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
	obj.splice(1, 2, "item1", "item2");
	assert (false);
} catch (e) {
	assert (e.message === "foo");
	assert (e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = {length : 1, splice : Array.prototype.splice};
Object.defineProperty(obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
	obj.splice(0, 1, "item1", "item2");
	assert (false);
} catch (e) {
	assert (e.message === "foo");
	assert (e instanceof ReferenceError);
}
