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

var obj = {};
var array = ["foo", 19, "bar", obj, "foo", 29, "baz"];

var index = array.lastIndexOf("foo");
assert(index === 4);
assert(array[index] === "foo");

assert(array.lastIndexOf("foo", 3) === 0);
assert(array.lastIndexOf("foo", -8) === -1);

var index = array.lastIndexOf("baz");
assert(index === 6);
assert(array[index] === "baz");

assert(array.lastIndexOf("baz", -2) === -1);

var index = array.lastIndexOf(obj);
assert(index === 3);
assert(array[index] === obj);

assert(array.lastIndexOf("foo", NaN) === 0);
assert(array.lastIndexOf("foo", Infinity) === 4);
assert(array.lastIndexOf("foo", -Infinity) === -1);

var arr = [];
arr[4294967294] = "foo";
assert(arr.lastIndexOf("foo", -1) === 4294967294)

var arr = [1,2];
assert(arr.lastIndexOf(2, undefined) === -1);
assert(arr.lastIndexOf(2) === 1);

// Checking behavior when unable to get length
var obj = { lastIndexOf : Array.prototype.lastIndexOf}
Object.defineProperty(obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.lastIndexOf("bar");
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { lastIndexOf : Array.prototype.lastIndexOf, length : 1}
Object.defineProperty(obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.lastIndexOf("bar");
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}
