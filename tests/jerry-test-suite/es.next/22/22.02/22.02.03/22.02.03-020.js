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

var uint8 = new Uint8Array(4);

uint8.set([10, "11", 12]);
assert(uint8[0] === 10 && uint8[1] === 11 && uint8[2] === 12);

uint8.set([13, 14.3, 15], 1);
assert(uint8[0] === 10 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([16], NaN);
assert(uint8[0] === 16 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([17], "");
assert(uint8[0] === 17 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([18], "0");
assert(uint8[0] === 18 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([19], false);
assert(uint8[0] === 19 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([20], 0.2);
assert(uint8[0] === 20 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([21], 0.9);
assert(uint8[0] === 21 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([22], null);
assert(uint8[0] === 22 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([23], {});
assert(uint8[0] === 23 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([24], []);
assert(uint8[0] === 24 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

uint8.set([25], true);
assert(uint8[0] === 24 && uint8[1] === 25 && uint8[2] === 14 && uint8[3] === 15);

