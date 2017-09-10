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

var a = new Int32Array(8);

a[0] = 0xffffffff;
a[1] = 0xff00000001;
a[2] = 0xff80000001;
a[3] = -2.3;
a[4] = Number.NEGATIVE_INFINITY;
a[5] = NaN;
a[6] = 10e17;
a[7] = -10e17;

assert(a[0] === -1);
assert(a[1] === 1);
assert(a[2] === -2147483647);
assert(a[3] === -2);
assert(a[4] === 0);
assert(a[5] === 0);
assert(a[6] === -1486618624);
assert(a[7] === 1486618624);
