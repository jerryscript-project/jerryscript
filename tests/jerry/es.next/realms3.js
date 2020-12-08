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

// Test1: reading global values from other realms

var a = 4.5

function f1()
{
  var realm = createRealm()
  var f = realm.eval("function g() { return a } g")

  realm.a = -6.25
  assert(f() === -6.25)
  assert(a === 4.5)
  assert(realm.g === f)

  realm.eval("var v1 = 6; eval('var v2 = 7.5')")
  assert(realm.v1 === 6)
  assert(realm.v2 === 7.5)

  var e = realm.eval

  eval("e('var v3 = -5.25'); var v3 = 1.5; e('v3++')")
  assert(realm.v3 === -4.25)
  assert(v3 === 1.5)
}
f1()

// Test2: recursive calls

var b = "S"

function f2()
{
  function f(fun) {
    return fun() + b;
  }

  var realm = createRealm()

  realm.eval("function h() { return b }")

  var g = realm.Function("fun", "return fun(h) + b")

  realm.b = "X"
  assert(g(f) === "XSX")
  assert(b === "S")
}
f2()

// Test3: built-in functions

function f3()
{
  var realm = createRealm()

  realm.f = function () { return eval("/a/") }

  var res = realm.eval("[f(), /a/, f(), /b/]")

  assert(Object.getPrototypeOf(res[0]) === RegExp.prototype)
  assert(Object.getPrototypeOf(res[1]) === realm.RegExp.prototype)
  assert(Object.getPrototypeOf(res[2]) === RegExp.prototype)
  assert(Object.getPrototypeOf(res[3]) === realm.RegExp.prototype)

  realm.g = function () { return non_existent }

  try {
    realm.eval("g()")
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
    assert(!(e instanceof realm.ReferenceError))
  }

  try {
    realm.eval("non_existent")
    assert(false)
  } catch (e) {
    assert(e instanceof realm.ReferenceError)
    assert(!(e instanceof ReferenceError))
  }
}
f3()

// Test4: generator functions

function f4()
{
  function f() {
    return b;
  }

  var realm = createRealm()

  function check_gen_result(result, value, done)
  {
    assert(Object.getPrototypeOf(result) === realm.Object.prototype)
    assert(result.value === value)
    assert(result.done === done)
  }

  realm.eval("var a = 'P', b = 'Q'")

  var gen = realm.eval("function* gen(f) { yield f() + a; yield f() + b; return a + b + f() }")

  var it = realm.gen(f)

  check_gen_result(it.next(), "SP", false)
  check_gen_result(it.next(), "SQ", false)
  check_gen_result(it.next(), "PQS", true)
}
f4()

// Test5: async functions

var successCount = 0

function f5()
{
  function f() {
    return b;
  }

  var r, p = new Promise(function(resolve, reject) { r = resolve })

  var realm = createRealm()

  realm.assert = assert
  realm.eval("async function asy(p, f) { assert(f() === 'S'); assert(await p === 4.5); return f() + b }")

  realm.b = "X"

  realm.asy(p, f).then(function(v) {
    assert(v === "SX")
    successCount++
  }, function() {
    assert(false)
  })
  r(4.5)
}
f5()

// Test6: async generator functions

function f6()
{
  function f() {
    return b;
  }

  var r, p = new Promise(function(resolve, reject) { r = resolve })

  var realm = createRealm()

  function check_fulfilled(p, value, done)
  {
    assert(p instanceof realm.Promise)

    p.then(function(v) {
      assert(v.value === value)
      assert(v.done === done)
      successCount++
    }, function() {
      assert(false)
    })
  }

  realm.assert = assert
  realm.eval("async function *asygen(p, f) { assert(f() === 'S'); assert(await p === -1.5); yield f() + a; return f() + b }")

  realm.a = "X"
  realm.b = "Y"

  var gen = realm.asygen(p, f)

  check_fulfilled(gen.next(), "SX", false)
  check_fulfilled(gen.next(), "SY", true)

  r(-1.5)
}
f6()

function __checkAsync() {
  assert(successCount === 3)
}
