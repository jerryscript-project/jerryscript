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

var realm = createRealm()
var ev = realm.eval

assert(realm.Math != Math)
assert(typeof realm.Math === "object")
assert(realm.Object != Object)
assert(typeof realm.Object === "function")
assert(realm.eval != eval)
assert(typeof realm.eval === "function")

// Share our 'assert' function with the realm
realm.assert = assert

// Test1: var1 and var2 must be different properties

realm.var1 = 3.5
assert(realm.var2 === undefined)

var var1 = "X"
var var2 = "Y"

ev("assert(var1 === 3.5); \
    try { realm; assert(false) } catch (e) { assert(e instanceof ReferenceError) } \
    assert(this === globalThis); \
    var var2 = this")
assert(realm.var2 === realm)

assert(var1 === "X")
assert(var2 === "Y")

// Test2: constructing any objects (including errors) must use the realm

assert (realm.RangeError != RangeError)
assert (realm.RangeError.prototype != RangeError.prototype)

realm.RangeError.prototype.myProperty = "XY"
assert(RangeError.prototype.myProperty === undefined)

try {
  var NumberToString = realm.Number.prototype.toString;
  NumberToString.call(0, 0)
  assert(false)
} catch (e) {
  assert(e instanceof realm.RangeError)
  assert(!(e instanceof RangeError))
  assert(e.myProperty === "XY")
}

assert (realm.SyntaxError != SyntaxError)
assert (realm.SyntaxError.prototype != SyntaxError.prototype)

realm.SyntaxError.prototype.myProperty = "AB"
assert(SyntaxError.prototype.myProperty === undefined)

try {
  ev("5 +")
  assert(false)
} catch (e) {
  assert(e instanceof realm.SyntaxError)
  assert(!(e instanceof SyntaxError))
  assert(e.myProperty === "AB")
}

// Test3: only the realm corresponding to the function matters

realm.Boolean.prototype.valueOf.a = Function.prototype.apply
Boolean.prototype.valueOf.a = realm.Function.prototype.apply

try {
  realm.Boolean.prototype.valueOf.a()
} catch (e) {
  assert(e instanceof realm.TypeError)
  assert(!(e instanceof TypeError))
}

try {
  Boolean.prototype.valueOf.a()
} catch (e) {
  assert(e instanceof TypeError)
  assert(!(e instanceof realm.TypeError))
}
