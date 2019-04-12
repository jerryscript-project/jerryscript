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

var array = ["foo", [], Infinity, 4]

assert(array.length === 4);

assert(array.shift() === "foo");
assert(array.length === 3);

var a = array.shift();
assert(a instanceof Array);
assert(array.length === 2);

assert(array.shift() === Infinity);
assert(array.length === 1);

assert(array.shift() === 4);
assert(array.length === 0);

assert(array.shift() === undefined);
assert(array.length === 0);

var referenceErrorThrower = function () {
  throw new ReferenceError ("foo");
}

// Checking behavior when unable to get length
var obj = { shift : Array.prototype.shift };
Object.defineProperty(obj, 'length', { 'get' : referenceErrorThrower });

try {
  obj.shift();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to set length
var obj = { shift : Array.prototype.shift };
Object.defineProperty(obj, 'length', { 'set' : referenceErrorThrower });

try {
  obj.shift();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when no length property defined
var obj = { shift : Array.prototype.shift };
assert (obj.length === undefined)
assert (obj.shift() === undefined)
assert (obj.length === 0)

// Checking behavior when unable to get element
var obj = { shift : Array.prototype.shift, length : 1 };
Object.defineProperty(obj, '0', { 'get' : referenceErrorThrower });

try {
  obj.shift();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.9.7.c.
   Checking behavior when the array is freezed */
try {
  f = function () { throw new ReferenceError("getter"); };
  arr =  { length : 9 };
  Object.defineProperty(arr, '8', { 'get' : f });
  Array.prototype.shift.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
  assert(e.message == "getter");
}

/* ES v5.1 15.4.4.9.7.d.ii.
   Checking behavior when the array is freezed */
try {
  arr =  { length : 9 };
  Object.defineProperty(arr, '8', { value : 8 });
  Object.defineProperty(arr, '7', { value : 7 });
  Array.prototype.shift.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.9.7.e.i.
   Checking behavior when the first element is null */
try {
  arr = { length : 9 };
  Object.defineProperty(arr, '0', { value : null });
  Array.prototype.shift.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.9.8.
   Checking behavior when last element is not writable */
try {
  arr = { length : 9 };
  Object.defineProperty(arr, '8', { writable : false });
  Array.prototype.shift.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.9.9.
   Checking behavior when the array is freezed */
try {
  arr = { length : 9 };
  Object.freeze(arr);
  Array.prototype.shift.call(arr);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
