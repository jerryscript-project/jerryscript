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

export var a = 0
export let b = 1

function other() {
  return 33.5
}

export function early() {
  // This function is called before the module is executed
  assert(other() === 33.5)
  assert(a === undefined)
  a = "X"

  try {
    b
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }
}

early = "Loaded"

import * as o from "module-circular-02.mjs"
