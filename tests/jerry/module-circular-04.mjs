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

import {a as aa, b as bb, early} from "module-circular-03.mjs"

export var a = 5

if (early != "Loaded")
{
  // The scope of module-circular-03.mjs is initialized and functions are usable
  // However, the module script has not been executed

  assert(aa === undefined)
  early()
  assert(aa === "X")

  try {
    bb
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  a = 7

  try {
    aa = "X"
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
  }

  try {
    c = 4
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }
}
