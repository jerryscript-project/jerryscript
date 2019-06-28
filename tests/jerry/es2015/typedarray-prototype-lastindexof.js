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

var normal_typedarrays = [
  new Uint8Array([0, 1, 2, 3, 0, 5, 6]),
  new Uint16Array([0, 1, 2, 3, 0, 5, 6]),
  new Uint32Array([0, 1, 2, 3, 0, 5, 6]),
  new Float32Array([0, 1, 2, 3, 0, 5, 6]),
  new Float64Array([0, 1, 2, 3, 0, 5, 6]),
  new Int8Array([0, 1, 2, 3, 0, 5, 6]),
  new Int16Array([0, 1, 2, 3, 0, 5, 6]),
  new Int32Array([0, 1, 2, 3, 0, 5, 6])
];

normal_typedarrays.forEach(function(e){
  try{
    e.prototype.lastIndexOf.call (undefined);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  var index = e.lastIndexOf(0);
  assert(index === 4);
  assert(e[index] === 0);

  // Regular input
  assert(e.lastIndexOf(0, 3) === 0);
  assert(e.lastIndexOf(0, -8) === -1);
  assert(e.lastIndexOf(2) === 2);
  assert(e.lastIndexOf(5, 3) === -1);
  assert(e.lastIndexOf(3, 6) === 3);

  // Empty input
  assert(e.lastIndexOf() === -1);

  // String input
  assert(e.lastIndexOf("foo") === -1);
  assert(e.lastIndexOf(0, "foo") === 0);

  // Nan, +/-Infinity
  assert(e.lastIndexOf(0, NaN) === 0);
  assert(e.lastIndexOf(0, Infinity) === 4);
  assert(e.lastIndexOf(0, -Infinity) === -1);

  // Checking behavior when fromIndex is undefined
  e.set([1, 2]);

  assert(e.lastIndexOf(2, undefined) === -1);
  assert(e.lastIndexOf(2) === 2);

  // Checking behavior when start index >= length
  e.set([11, 22, 33, 44]);

  assert(e.lastIndexOf(44, 8) === 3);

  var fromIndex = {
    toString: function () {
      return {};
    },

    valueOf: function () {
      return {};
    }
  };

  e.set([0, 1]);

  try {
    e.lastIndexOf(1, fromIndex);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

// Checking behavior when length is zero
var empty_typedarrays = [
  new Uint8Array([]),
  new Uint16Array([]),
  new Uint32Array([]),
  new Float32Array([]),
  new Float64Array([]),
  new Int8Array([]),
  new Int16Array([]),
  new Int32Array([])
];

empty_typedarrays.forEach(function(e){
  assert(e.lastIndexOf(0) === -1);
});
