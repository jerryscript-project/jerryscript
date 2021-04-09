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

import * as o from "module-circular-01.mjs"

if (o.early != "Loaded")
{
  // The scope of module-circular-01.mjs is initialized and functions are usable
  // However, the module script has not been executed

  assert(o.a === undefined)
  o.early()
  assert(o.a === "X")

  try {
    o.b
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  try {
    o.a = "X"
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
  }

  assert(o[Symbol.toStringTag] === "Module")

  var result = Object.getOwnPropertyDescriptor(o, "a")
  assert(result.value === "X")
  assert(result.configurable === false)
  assert(result.enumerable === true)
  assert(result.writable === true)

  try {
    Object.getOwnPropertyDescriptor(o, "b")
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  result = Object.getOwnPropertyDescriptor(o, Symbol.toStringTag)
  assert(result.value === "Module")
  assert(result.configurable === false)
  assert(result.enumerable === false)
  assert(result.writable === false)

  Object.defineProperty(o, "a", { value: "X" })

  try {
    Object.defineProperty(o, "a", { value: "Y" })
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
  }

  try {
    Object.defineProperty(o, "b", { value:5 })
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  Object.defineProperty(o, Symbol.toStringTag, { value: "Module", writable:false })

  try {
    Object.defineProperty(o, Symbol.toStringTag, { writable:true })
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
  }
}
