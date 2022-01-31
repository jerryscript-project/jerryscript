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

import def, {a} from "./module-namespace-03.mjs"
/* Nothing is tracked, only the result value is exported. */
export default def
export {a}

export * from "./module-namespace-03.mjs"
export * from "./module-namespace-04.mjs"

import * as o1 from "./module-namespace-03.mjs"
import * as o2 from "./module-namespace-04.mjs"

assert(def === 8.5)
assert(o1.default === 8.5)
assert(o2.default === undefined)
