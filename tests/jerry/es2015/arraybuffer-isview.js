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

assert(ArrayBuffer.isView() === false);
assert(ArrayBuffer.isView([]) === false);
assert(ArrayBuffer.isView({}) === false);
assert(ArrayBuffer.isView(null) === false);
assert(ArrayBuffer.isView(undefined) === false);
assert(ArrayBuffer.isView(new ArrayBuffer(10)) === false);

assert(ArrayBuffer.isView(new Int8Array()) === true);
assert(ArrayBuffer.isView(new Uint8Array()) === true);
assert(ArrayBuffer.isView(new Uint8ClampedArray()) === true);
assert(ArrayBuffer.isView(new Int16Array()) === true);
assert(ArrayBuffer.isView(new Uint16Array()) === true);
assert(ArrayBuffer.isView(new Int32Array()) === true);
assert(ArrayBuffer.isView(new Uint32Array()) === true);
assert(ArrayBuffer.isView(new Float32Array()) === true);
assert(ArrayBuffer.isView(new Float64Array()) === true);

assert(ArrayBuffer.isView(new Int8Array(10).subarray(0, 3)) === true);

var buffer = new ArrayBuffer(2);
var dv = new DataView(buffer);
assert(ArrayBuffer.isView(dv) === true);
