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

import * as o from "./module-namespace-02.mjs"
import def from "./module-namespace-02.mjs"

assert(o.default === 8.5)
assert(def === 8.5)

var val = {}

assert(o.a === 2.5);
assert(o.b === undefined);
assert(o.c(val) === "c")
assert(o.d === "d")

assert(o.a === val);
assert(o.default === 8.5)
assert(def === 8.5)
