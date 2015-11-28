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

var array = ["foo", [], Infinity, 4];

function f(arg1, arg2, arg3) {
  assert(arg1 === array[arg2]);
  assert(arg3 === array);
  return false;
}

assert(array.some(f) === false);

function g(arg1, arg2, arg3) {
  if (arg1 === 1) {
    return true;
  } else {
    return false;
  }
}

var arr1 = [2, 2, 2, 2, 2, 2];
assert(arr1.some(g) === false);

var arr2 = [2, 2, 2, 2, 2, 1];
assert(arr2.some(g) === true);

// Checking behavior when unable to get length
var obj = { some : Array.prototype.some };
Object.defineProperty(obj, 'length', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.some(f);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { some : Array.prototype.some, length : 1};
Object.defineProperty(obj, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.some(f);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}
