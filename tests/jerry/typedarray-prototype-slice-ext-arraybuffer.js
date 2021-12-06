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

var typedarrays = [
  Uint8ClampedArray,
  Uint8Array,
  Uint16Array,
  Uint32Array,
  Float32Array,
  Float64Array,
  Int8Array,
  Int16Array,
  Int32Array,
];

for (var typeIdx = 0; typeIdx < typedarrays.length; typeIdx++) {
  var buffer = new ArrayBuffer(100);
  var typed = new typedarrays[typeIdx](buffer, 8, 6);
  try {
    typed.prototype.slice.call(undefined);
    assert(false);
  } catch (err) {
    assert(err instanceof TypeError);
  }

  for (var idx = 0; idx < typed.length; idx++) {
    typed[idx] = idx;
  }

  // Test with normal inputs
  assert(typed.slice(1, 3).toString() === "1,2");
  assert(typed.slice(2, 5).toString() === "2,3,4");
  assert(typed.slice(0, 6).toString() === "0,1,2,3,4,5");

  // Test witn negative inputs
  assert(typed.slice(-2, 5).toString() === "4");
  assert(typed.slice(0, -3).toString() === "0,1,2");
  assert(typed.slice(-1, -4).toString() === "");

  // Test with bigger inputs then length
  assert(typed.slice(7, 1).toString() === "");
  assert(typed.slice(2, 9).toString() === "2,3,4,5");

  // Test with undefined
  assert(typed.slice(undefined, 4).toString() === "0,1,2,3");
  assert(typed.slice(0, undefined).toString() === "0,1,2,3,4,5");
  assert(typed.slice(undefined, undefined).toString() === "0,1,2,3,4,5");

  // Test with NaN and +/-Infinity
  assert(typed.slice(NaN, 3).toString() === "0,1,2");
  assert(typed.slice(2, Infinity).toString() === "2,3,4,5");
  assert(typed.slice(-Infinity, Infinity).toString() === "0,1,2,3,4,5");

  // Test with default inputs
  assert(typed.slice().toString() === "0,1,2,3,4,5");
  assert(typed.slice(4).toString() === "4,5");

  delete buffer;
}
