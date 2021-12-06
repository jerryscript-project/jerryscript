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

// Test1: realm cannot be gc-ed until it is referenced

function f1() {
  var ev = createRealm().eval

  gc()

  try {
    ev("5 +")
  } catch (e) {
    assert(e instanceof ev("this").SyntaxError)
    assert(!(e instanceof SyntaxError))
  }
}
f1()

// Test2: built-ins cannot be gc-ed until the realm exists

function f2() {
  var realm = createRealm()

  var str = new realm.String("A");
  // Assign a property to String.prototype
  Object.getPrototypeOf(str).myProperty = "XY"

  str = null
  // No reference to String.prototype
  gc()

  str = new realm.String("A")
  assert(Object.getPrototypeOf(str).myProperty === "XY")
  assert(realm.String.prototype.myProperty === "XY")
}
f2()
