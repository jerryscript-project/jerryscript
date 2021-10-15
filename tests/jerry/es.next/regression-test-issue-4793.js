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

// Make sure that TypedArray filter correctly copies the data (avoid overflow).
// Test creates a smaller region for "output" TypedArray.
// Last number is intentionally a "big" float.
var big_array = new Float64Array([0.523565555, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 333333232134.1]);
big_array.constructor = Float32Array;

var result_float32_array = big_array.filter(x => x % 2 == 0);
assert(result_float32_array instanceof Float32Array);
assert(result_float32_array.length === 5);

// Create an even smaller result TypedArray.
big_array.constructor = Uint8Array;
var result_uint8_array = big_array.filter(x => x % 3 == 0);
assert(result_uint8_array instanceof Uint8Array);
assert(result_uint8_array.length === 3);

// Trigger a filter error when at the last element
try {
  big_array.filter(function(x, idx) {
    if (idx > 10) {
        throw new Error("Error test magic");
    }
    return x % 4 == 0;
  });
} catch (ex) {
  assert(ex instanceof Error);
  assert(ex.message === "Error test magic");
}
