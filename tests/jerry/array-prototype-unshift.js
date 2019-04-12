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

var array = []

assert(array.length === 0);

array.unshift("foo");
assert(array.length === 1);
assert(array[0] === "foo");

array.unshift(new Array())
assert(array.length === 2);
assert(array[0] instanceof Array);
assert(array[1] === "foo")

array.unshift(Infinity);
assert(array.length === 3);
assert(array[0] === Infinity);
assert(array[1] instanceof Array);
assert(array[2] === "foo")

array.unshift("bar", 0);
assert(array.length === 5);
assert(array[0] === "bar");
assert(array[1] === 0);
assert(array[2] === Infinity);
assert(array[3] instanceof Array);
assert(array[4] === "foo")


// Checking behavior when no length property defined
var obj = { unshift : Array.prototype.unshift };

assert(obj.length === undefined);
obj.unshift(1,2,3);
assert(obj.length === 3);

// Checking behavior when unable to get length
var obj = { unshift : Array.prototype.unshift };
Object.defineProperty(obj, 'length', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.unshift(1);
  assert(false)
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to set length
var obj = { unshift : Array.prototype.unshift };
Object.defineProperty(obj, 'length', { 'set' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.unshift(2);
  assert(false)
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable shift elements
var obj = { unshift : Array.prototype.unshift, length : 1 };
Object.defineProperty(obj, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.unshift(3);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

var obj = { unshift : Array.prototype.unshift, length : 1 };
Object.defineProperty(obj, '0', { 'set' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.unshift(4);
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when a property is not defined
var obj = { '0' : "foo", '2' : "bar", length : 3, unshift : Array.prototype.unshift };
assert(obj.unshift("baz") === 4);
assert(obj[0] === "baz");
assert(obj[1] === "foo");
assert(obj[2] === undefined);
assert(obj[3] === "bar");

/* ES v5.1 15.4.4.13.6.d.ii.
   Checking behavior when the array is freezed */
try {
  var arr = [0, 1];
  Object.freeze(arr);
  Array.prototype.unshift.call(arr, 2, 3);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.13.6.e.i.
   Checking behavior when the array has only one property and bigger length */
try {
  var arr = { length : 9 };
  Object.defineProperty(arr, '6', { value : 2 });
  Array.prototype.unshift.call(arr, 2, 3);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
