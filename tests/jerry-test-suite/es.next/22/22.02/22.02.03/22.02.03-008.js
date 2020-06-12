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

var a = new Uint8Array([1, 2, 3]);

var b = a.map(function(num) {
  return num * 2;
});

assert(a[0] === 1);
assert(a[1] === 2);
assert(a[2] === 3);
assert(b[0] === 2);
assert(b[1] === 4);
assert(b[2] === 6);
