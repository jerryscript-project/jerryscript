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

var a = new Float32Array([0.1, -3.4, 65535.9]);
var b = new Int16Array(a);
var c = new Uint8Array(a);
var d = new Int32Array(a);

assert(b[0] === 0);
assert(b[1] === -3);
assert(b[2] === -1);
assert(c[0] === 0);
assert(c[1] === 253);
assert(c[2] === 255);
assert(d[0] === 0);
assert(d[1] === -3);
assert(d[2] === 65535);
