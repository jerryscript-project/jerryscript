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

// Remove the buffer
var array = [1, 2, 3, 4, 5];
var value = array.indexOf(4, {
    valueOf: function() {
        array.length = 0;
    }
})

assert(value === -1);

// Extend the buffer
var array = [1, 2, 3];
var value = array.indexOf(2, {
    valueOf: function() {
        array.length = 5;
    }
})

assert(value === 1);

// Reduce the buffer
var array = [1, 2, 3, 4, 5, 6, 7];
var value = array.indexOf(6, {
    valueOf: function() {
        array.length = 5;
    }
})

assert(value === -1);
