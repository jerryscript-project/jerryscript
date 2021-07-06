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

/* Export the same namespace using two names */
export
  * as ns1 from
  "./module-export-04.mjs"
export * as
  ns2
  from "./module-export-04.mjs"
export *
  as ns3 from "./module-export-04.mjs"

/* Local bindings can have the same names as exports. */
import ns1, {x as ns2}
  from "./module-export-04.mjs"
let ns3 = 9.5

assert(ns1 === "str")
assert(ns2 === 41)
assert(ns3 === 9.5)
