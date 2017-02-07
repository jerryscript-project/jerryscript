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

var a = new Float32Array([1.25, 2.5, 3.75]);

var b = a.map(function(num) {
  return num * 2;
});

assert(a[0] === 1.25);
assert(a[1] === 2.5);
assert(a[2] === 3.75);
assert(b[0] === 2.5);
assert(b[1] === 5);
assert(b[2] === 7.5);
