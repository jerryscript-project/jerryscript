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

assert(Array.prototype.includes.length === 1);
assert(Array.prototype.includes.name === "includes");

var num_arr = [1, 2, 3,,4, 5];
var str_arr = ['foo', 'bar', 'baz', NaN, 'foo'];
var obj = {};
var obj_arr = [1, obj, 2];
var empry_arr = [];

assert(num_arr.includes(3) === true);
assert(num_arr.includes(3, 4) === false);
assert(num_arr.includes(3, -5) === true);
assert(num_arr.includes(undefined) === true);
assert(num_arr.includes(3, Infinity) === false);
assert(num_arr.includes(3, -0) === true);
assert(str_arr.includes(NaN) === true);
assert(str_arr.includes('foo', 4) === true);
assert(str_arr.includes('f') === false);
assert(obj_arr.includes(obj) === true);
assert(obj_arr.includes({}) === false);
assert(empry_arr.includes() === false);
assert(empry_arr.includes(3) === false);
assert([undefined].includes() === true);

Object.defineProperty(num_arr, "1", { get: function() {throw 42}});

try {
  num_arr.includes(4);
  assert(false);
} catch (e) {
  assert(e === 42);
}

var sym = Symbol('foo');

try {
  num_arr.includes(3, sym);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Remove the buffer
var array = [1, 2, 3, 4, 5];
var found = array.includes(4, {
    valueOf: function() {
        array.length = 0;
    }
})

assert(found === false);

// Extend the buffer
var array = [1, 2, 3];
var found = array.includes(2, {
    valueOf: function() {
        array.length = 5;
    }
})

assert(found === true);

// Reduce the buffer
var array = [1, 2, 3, 4, 5, 6, 7];
var found = array.includes(6, {
    valueOf: function() {
        array.length = 5;
    }
})

assert(found === false);
