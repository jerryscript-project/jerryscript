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

var array = [4, 3, 2, 1, 0]

array.reverse();

for (i = 0; i < array.length; i++) {
  assert(array[i] === i);
}

// Checking behavior when unable to get length
var obj = { reverse : Array.prototype.reverse };
Object.defineProperty(obj, 'length', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.reverse();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { reverse : Array.prototype.reverse, length : 3 };
Object.defineProperty(obj, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.reverse();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.8.6.e.
   Checking behavior when unable to get the last element */
var obj = { reverse : Array.prototype.reverse, length : 4 };
Object.defineProperty(obj, '3', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.reverse();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.8.6.h.i.
   Checking behavior when first 3 elements are not writable */
try {
  var arr = [,,, 3, 4, 5, 6,,,,,,,,,0, 1, 2, 3, 4, 5, 6];
  Object.defineProperty(arr, '0', {});
  Object.defineProperty(arr, '1', {});
  Object.defineProperty(arr, '2', {});
  Array.prototype.reverse.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.8.6.h.ii.
   Checking behavior when last 3 elements are not writable */
try {
  var arr = [0, 1, 2, 3, 4, 5, 6,,,,,,,,,0, 1, 2, 3,,,];
  Object.defineProperty(arr, '19', {});
  Object.defineProperty(arr, '20', {});
  Object.defineProperty(arr, '21', {});
  Array.prototype.reverse.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.8.6.i.i.
   Checking behavior when first elements do not exist and the array is freezed */
try {
  var arr = [,,,,,,,,,,,,,,,,0, 1, 2, 3, 4, 5, 6];
  arr = Object.freeze(arr);
  Array.prototype.reverse.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.8.6.i.ii.
   Checking behavior when unable to get the first 2 elements */
var obj = { reverse : Array.prototype.reverse, length : 4 };
Object.defineProperty(obj, '2', { value : 0 });
Object.defineProperty(obj, '3', { value : 0 });
try {
  obj.reverse();
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.8.6.j.i.
   Checking behavior when unable to get the last 2 elements */
var obj = { reverse : Array.prototype.reverse, length : 4 };
Object.defineProperty(obj, '0', { value : 0 });
Object.defineProperty(obj, '1', { value : 0 });
try {
  obj.reverse();
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
