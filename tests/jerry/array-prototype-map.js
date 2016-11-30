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

// helper function - simple implementation
Array.prototype.equals = function (array) {
  if (this.length != array.length)
    return false;

  for (var i = 0; i < this.length; i++) {
    if (this[i] instanceof Array && array[i] instanceof Array) {
      if (!this[i].equals(array[i]))
        return false;
      }
      else if (this[i] != array[i]) {
        return false;
    }
  }

  return true;
}

// check function type
try {
  [0].map(new Object());
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// various checks
assert ([1, 4, 9].map(Math.sqrt).equals([1, 2, 3]));

assert (isNaN([1, 4, "X"].map(Number)[2]));

var func = function(val, idx) {
  return val + idx;
};

assert ([1, 4, 9].map(func).equals([1,5,11]));

assert ([1, "X", 10].map(func).equals([1, "X1", 12]));

var arr = [1,2,3];
arr.length = 5;
assert(arr.map(func).length === arr.length);

var long_array = [0, 1];
assert (long_array.map(func).equals([0,2]));

long_array[100] = 1;
assert (long_array.map(func).equals([0,2,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,101]));

var arr = [1,2];
Array.prototype[0] = 3;
var newArr = arr.map(function(value) { return value; });
delete Array.prototype[0];
assert(newArr.hasOwnProperty("0"));
assert(newArr[0] === 1);

// check behavior when unable to get length
var obj = {};
Object.defineProperty(obj, 'length', { 'get' : function () {throw new ReferenceError ("foo"); } });
obj.map = Array.prototype.map;

try {
  obj.map(func);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// check behavior when unable to get element
var obj = {}
obj.length = 1;
Object.defineProperty(obj, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });
obj.map = Array.prototype.map

try {
  obj.map(func);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// check thisArg
var thisArg = {add: 10};
var func2 = function(value) {
  return this.add + value;
}
assert([1,2].map(func2, thisArg).equals([11, 12]));

// check passed Object
var array_example = [1,2];
Object.defineProperty(array_example, 'const', { 'get' : function () {return "CT";} });
var func3 = function(value, idx, thisobj) {
  return value * idx + thisobj.const;
}
assert(array_example.map(func3).equals(["0CT", "2CT"]));
