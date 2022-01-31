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

var types = [
  Uint16Array,
  Uint32Array,
  Float32Array,
  Float64Array,
  Int16Array,
  Int32Array,
]

var buffer = new ArrayBuffer (100);

for (var idx = 0; idx < types.length; idx++) {
  try {
    var target = types[idx];

    /* TypedArray should throw error on incorrect offset (offset % elementSize != 0)! */
    new target (buffer, target.BYTES_PER_ELEMENT + 1, 1);
    assert (false);
  } catch (ex) {
    assert (ex instanceof RangeError);
  }
}
