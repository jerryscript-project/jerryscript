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
