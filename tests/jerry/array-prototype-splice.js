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

// This test will not pass on FLOAT32 due to precision issues

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
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array2 = array.splice(2);

assert (array.length == 2);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array2.length == 2);
assert (array2[0] == -127);
assert (array2[1] == "sunshine");

// --------------------------------------------------------
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array3 = array.splice(2, 1);

assert (array.length == 3);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array[2] == "sunshine");
assert (array3.length == 1);
assert (array3[0] == -127);

// --------------------------------------------------------
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
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
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
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
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array6 = array.splice(undefined, undefined, undefined);

assert (array.length == 5);
assert (array[0] == undefined);
assert (array[1] == 54);
assert (array[2] == undefined);
assert (array[3] == -127);
assert (array[4] == "sunshine");
assert (array6.length == 0);

// --------------------------------------------------------
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array7 = array.splice(Infinity, NaN);
assert (array.length == 4);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array[2] == -127);
assert (array[3] == "sunshine");
assert (array7.length == 0);

// --------------------------------------------------------
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array8 = array.splice(-Infinity, Infinity);

assert (array.length == 0);
assert (array8.length == 4);
assert (array8[0] == 54);
assert (array8[1] == undefined);
assert (array8[2] == -127);
assert (array8[3] == "sunshine");

// --------------------------------------------------------
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array9 = array.splice(NaN, -Infinity);
assert (array.length == 4);
assert (array[0] == 54);
assert (array[1] == undefined);
assert (array[2] == -127);
assert (array[3] == "sunshine");
assert (array9.length == 0);

// --------------------------------------------------------
array = setDefaultValues(); // 54, undefined, -127, "sunshine"
var array10 = array.splice(-3, 4, Infinity, "university");
assert (array.length == 3);
assert (array[0] == 54);
assert (array[1] == Infinity);
assert (array[2] == "university");
assert (array10.length == 3);
assert (array10[0] == undefined);
assert (array10[1] == -127);
assert (array10[2] == "sunshine");

var array = [];
array[4294967294] = "foo";
var result = array.splice(4294967294, 1, "x")
assert(result.length === 1)
assert(result[0] === "foo")
assert(array[4294967294] === "x")

array[0] = "bar";
var result = array.splice(-4294967295, 1, "y");
assert(result.length === 1)
assert(result[0] === "bar")
assert(array[0] === "y")

var arr = [1,2];
Array.prototype[0] = 3;
var newArr = arr.splice(0, 1);
delete Array.prototype[0];
assert(newArr.hasOwnProperty("0"));
assert(newArr[0] === 1);

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
