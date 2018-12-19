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

var s = new Set();
assert(s.size == 0);
assert(s.add(1) === s);
assert(s.has(1));
assert(s.size == 1);

assert(s.add(undefined) === s);
assert(s.size == 2);
assert(s.has(undefined));
/*
print(s.add(null) === s);
print(s.size == 3);
print(s.has(null));*/

var obj = { x:789 };
assert(s.add(obj) === s);
assert(s.size == 3);
assert(s.has(obj));

assert(s.add(12.25) === s);
assert(s.size == 4);
assert(s.has(12 + (function() { return 0.25 })()));

assert(s.delete(1))
assert(s.size == 3);
assert(!s.has(1));

assert(!s.delete(2));

assert (s.delete(12 + (function() { return 0.25 })()));
assert(s.size == 2)
assert(!s.has(12.25));


assert(s.delete(obj))
assert(s.delete(undefined))
assert(s.size == 0);
