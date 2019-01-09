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

var obj = {};
var array = ["foo", 19, "bar", obj, "foo", 29, "baz"];

var index = array.indexOf("foo");
assert(index === 0);
assert(array[index] === "foo");

assert(array.indexOf("foo", 1) === 4);
assert(array.indexOf("foo", 5) === -1);

var index = array.indexOf("baz");
assert(index === 6);
assert(array[index] === "baz");

assert(array.indexOf("baz", 7) === -1);

var index = array.indexOf(obj);
assert(index === 3);
assert(array[index] === obj);

assert(array.indexOf("foo", NaN) === 0);
assert(array.indexOf("foo", Infinity) === -1);
assert(array.indexOf("foo", -Infinity) === 0);

assert([true].indexOf(true, -0) === 0);

// Checking behavior when length is zero
var obj = { indexOf : Array.prototype.indexOf, length : 0 };
assert(obj.indexOf("foo") === -1);

// Checking behavior when start index >= length
var arr = [11, 22, 33, 44];
assert(arr.indexOf(44, 4) === -1);

var fromIndex = {
  toString: function () {
    return {};
  },

  valueOf: function () {
    return {};
  }
};

try {
  [0, 1].indexOf(1, fromIndex);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Checking behavior when unable to get length
var obj = { indexOf : Array.prototype.indexOf}
Object.defineProperty(obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.indexOf("bar");
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get element
var obj = { indexOf : Array.prototype.indexOf, length : 1}
Object.defineProperty(obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj.indexOf("bar");
  assert(false);
} catch (e) {
  assert(e.message === "foo");
  assert(e instanceof ReferenceError);
}

// Checking behavior when values are unable to find
var a = [undefined, undefined, undefined, undefined, undefined];
for (var i = 0; i < a.length; i++) {
  delete a[i];
}
a.indexOf("bar");

// Checking behavior when this value is not coercible to object
try {
  Array.prototype.indexOf.call(null, "asd");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Checking behavior when length is not a number
try {
  var o = {};
  Object.defineProperty(o, 'toString', { 'get' : function () { throw new ReferenceError ("foo"); } });
  var a = { length : o };
  Array.prototype.indexOf.call(a, function () { });
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}

// Checking behavior when unable to get the first element
try {
  var a = [0, 1, 2, 3, 4];
  Object.defineProperty(a, '0', { 'get' : function () { throw new ReferenceError; } });
  Array.prototype.indexOf.call(a, "bar");
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}
