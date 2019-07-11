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

import incrementer, {aa, c_, x,} from "module-export-03.js"
var i = new incrementer(3);
assert(i.incr() === 4);
assert(i.incr() === 5);
assert(i.incr() === 6);

assert (aa === "a");
assert (x === 42);
assert (c_(x) == 84);
