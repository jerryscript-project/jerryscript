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

assert(array.pop() === 4)
assert(array.length === 3);

assert(array.pop() === Infinity);
assert(array.length === 2);

var a = array.pop()
assert(a instanceof Array);
assert(array.length === 1);

assert(array.pop() === "foo");
assert(array.length === 0);

assert(array.pop() === undefined);
assert(array.length === 0);

// Checking behavior when unable to get length
var obj = { pop : Array.prototype.pop };
Object.defineProperty(obj, 'length', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.pop();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to set length
var obj = { pop : Array.prototype.pop };
Object.defineProperty(obj, 'length', { 'set' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.pop();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when no length property defined
var obj = { pop : Array.prototype.pop };
assert(obj.length === undefined)
assert(obj.pop() === undefined)
assert(obj.length === 0)

// Checking behavior when unable to get element
var obj = { pop : Array.prototype.pop, length : 1 };
Object.defineProperty(obj, '0', { 'get' : function () {throw new ReferenceError ("foo"); } });

try {
  obj.pop();
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

/* ES v5.1 15.4.4.6.5.c
   Checking behavior when unable to delete property */
var obj = {pop : Array.prototype.pop, length : 2};
Object.defineProperty(obj, '1', function () {});

try {
  obj.pop();
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* ES v5.1 15.4.4.6.5.d
   Checking behavior when array is not modifiable */
var obj = {pop : Array.prototype.pop, length : 2};
Object.freeze(obj);

try {
  obj.pop();
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
