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

// Test with regular arrays
var alpha = ['a', 'b', 'c'];
var numeric = [1, 2, 3];

var alphaNumeric = alpha.concat(numeric);
assert(JSON.stringify(alphaNumeric) === '["a","b","c",1,2,3]');
assert(alphaNumeric.length === 6);

numeric[Symbol.isConcatSpreadable] = false;
alphaNumeric = alpha.concat(numeric);
assert(JSON.stringify(alphaNumeric) === '["a","b","c",[1,2,3]]');
assert(alphaNumeric.length === 4);

numeric[Symbol.isConcatSpreadable] = true;

// Test with array-like object
var fakeArray = {
  [Symbol.isConcatSpreadable]: true,
  length: 2,
  0: 4,
  1: 5
}

var numericArray = numeric.concat(fakeArray);
assert(JSON.stringify(numericArray) === '[1,2,3,4,5]');
assert(numericArray.length === 5);

fakeArray[Symbol.isConcatSpreadable] = false;
numericArray = numeric.concat(fakeArray);
assert(JSON.stringify(numericArray) === '[1,2,3,{"0":4,"1":5,"length":2}]');
assert(numericArray.length === 4);

// Test with object
var obj = { 0: 'd' };

var alphaObj = alpha.concat(obj);
assert(JSON.stringify(alphaObj) === '["a","b","c",{"0":"d"}]');
assert(alphaObj.length === 4);

obj[Symbol.isConcatSpreadable] = true;
alphaObj = alpha.concat(obj);
assert(JSON.stringify(alphaObj) === '["a","b","c"]');
assert(alphaObj.length === 3);

// Test with boolean
var bool = true;
var numericBool = numeric.concat(bool);
assert(JSON.stringify(numericBool) === '[1,2,3,true]');
assert(numericBool.length === 4);

bool[Symbol.isConcatSpreadable] = false;
numericBool = numeric.concat(bool);
assert(JSON.stringify(numericBool) === '[1,2,3,true]');
assert(numericBool.length === 4);

// Test when unable to concat
var array1 = [];
var array2 = [];
Object.defineProperty(array2, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  array1.concat(array2);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}
