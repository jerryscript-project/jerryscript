/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    e.prototype.indexOf.call (undefined);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  var index = e.indexOf(0);
  assert(index === 0);
  assert(e[index] === 0);

  // Regular input
  assert(e.indexOf(0, 1) === 4);
  assert(e.indexOf(0, 5) === -1);
  assert(e.indexOf(3, -2) === -1);
  assert(e.indexOf(5, -6) === 5);
  assert(e.indexOf(2, -2) === -1);

  // Empty input
  assert(e.indexOf() === -1);

  // String input
  assert(e.indexOf("foo") === -1);
  assert(e.indexOf(3, "foo") === 3);

  // Nan, +/-Infinity
  assert(e.indexOf(0, NaN) === 0);
  assert(e.indexOf(0, Infinity) === -1);
  assert(e.indexOf(3, -Infinity) === 3);

  // Checking behavior when fromIndex is undefined
  e.set([1, 2]);

  assert(e.indexOf(2, undefined) === 1);
  assert(e.indexOf(2) === 1);

  // Checking behavior when start index >= length
  e.set([11, 22, 33, 44]);

  assert(e.indexOf(44, 5) === -1);

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
    e.indexOf(1, fromIndex);
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
  assert(e.indexOf(0) === -1);
});
