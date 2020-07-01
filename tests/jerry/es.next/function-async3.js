// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* This test async prototype. */

var proto1 = Object.getPrototypeOf(async () => {})
var proto2 = Object.getPrototypeOf(async function () {})

assert(proto1 === proto2)
assert(typeof proto1 === "object")
assert(proto1[Symbol.toStringTag] === "AsyncFunction")
assert(typeof proto1.constructor === "function")
assert(proto1.constructor.name === "AsyncFunction")

var successCount = 0
var f = proto1.constructor("p", "assert(await p === 'Res'); successCount++")
f(Promise.resolve("Res"))

function __checkAsync() {
  assert(successCount === 1)
}
