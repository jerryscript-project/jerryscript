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

var array = [54, undefined, "Lemon", -127];

var array1 = array.slice();
var array2 = array.slice("a", "3");
var array3 = array.slice(-2);
var array4 = array.slice(-12, undefined);
var array5 = array.slice(undefined, -3);
var array6 = array.slice(Infinity, NaN);
var array7 = array.slice(-Infinity, Infinity);
var array8 = array.slice(NaN, -Infinity);

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

assert (array6.length == 0);

assert (array7.length == 4);
assert (array7[0] == 54);
assert (array7[1] == undefined);
assert (array7[2] == "Lemon");
assert (array7[3] == -127);

assert (array8.length == 0);

var array = [];
array[4294967293] = "foo";
array.length = 4294967295;
var result = array.slice(4294967293, -1)
assert(result.length === 1)
assert(result[0] === "foo")

array[0] = "bar";
var result = array.slice(-4294967295, -4294967294)
assert(result.length === 1)
assert(result[0] === "bar")

var array = [];
array[0] = "foo";
var result = array.slice(4294967296, 4294967297);
assert(result.length === 0);

array[4294967293] = "bar";
var result = array.slice(-4294967297, -4294967296);
assert(result.length === 0);

var arr = [1,2];
Array.prototype[0] = 3;
var newArr = arr.slice(0, 1);
delete Array.prototype[0];
assert(newArr.hasOwnProperty("0"));
assert(newArr[0] === 1);

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

/* ES v5.1 15.4.4.10.5.
   Checking behavior when start value throws exception */
var arg1 = { };
Object.defineProperty(arg1, 'valueOf', { 'get' : function () { throw new ReferenceError ("foo"); } });
var obj = { slice : Array.prototype.slice };

try {
  obj.slice(arg1);
  assert(false);
} catch (e) {
  assert(e.message === 'foo');
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.10.7.
   Checking behavior when end value throws exception */
var arg2 = { };
Object.defineProperty(arg2, 'valueOf', { 'get' : function () { throw new ReferenceError ("foo"); } });
var obj = { slice : Array.prototype.slice };

try {
  obj.slice(0, arg2);
  assert(false);
} catch (e) {
  assert(e.message === 'foo');
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.10.10.
   Checking behavior when unable to get element */
var obj = { length : 3, slice : Array.prototype.slice };
Object.defineProperty(obj, '1', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.slice(0, 3);
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}
