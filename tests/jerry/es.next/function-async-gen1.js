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

var successCount = 0

function check_fulfilled(p, value, done)
{
  assert(p instanceof Promise)

  p.then(function(v) {
    assert(v.value === value)
    assert(v.done === done)
    successCount++
  }, function() {
    assert(false)
  })
}

function check_rejected(p, value)
{
  assert(p instanceof Promise)

  p.then(function(v) {
    assert(false)
  }, function(v) {
    assert(v === value)
    successCount++
  })
}


function check_type_error(p)
{
  assert(p instanceof Promise)

  p.then(function(v) {
    assert(false)
  }, function(v) {
    assert(v instanceof TypeError);
    successCount++
  })
}

// Test 1

var gen, r, o

async function *f1(p, o) {
  assert((yield "Test1") === "Test2")
  await p

  assert((yield 1.5) === 2.5)
  await "String"

  return o
}

assert(Object.prototype.toString.call(f1) === "[object AsyncGeneratorFunction]")

o = {}
gen = f1(new Promise(function(resolve, reject) { r = resolve }), o)

assert(Object.prototype.toString.call(gen) === "[object AsyncGenerator]")

check_fulfilled(gen.next(), "Test1", false)
check_fulfilled(gen.next("Test2"), 1.5, false)
check_fulfilled(gen.next(2.5), o, true)
check_fulfilled(gen.next(), undefined, true)
check_fulfilled(gen.next(), undefined, true)
check_type_error(gen.next.call(undefined))
check_type_error(gen.throw.call(undefined))
check_type_error(gen.return.call(undefined))

r(1)

// Test 2

async function *f2(o) {
  try {
    await {}
    yield o
    assert(false)
  } catch (e) {
    assert(e === "Throw")
  }

  try {
    await "String"
    yield 12
    assert(false)
  } catch (e) {
    assert(e === o)

    throw o
  }
}

o = {}
gen = f2(o)
check_fulfilled(gen.next(), o, false)
check_fulfilled(gen.throw("Throw"), 12, false)
check_rejected(gen.throw(o), o)

check_fulfilled(gen.next(), undefined, true)
check_rejected(gen.throw(), undefined)
check_fulfilled(gen.return(), undefined, true)

// Test 3

async function *f3() {
  throw "Msg"
}

gen = f3()
check_rejected(gen.next(), "Msg")
gen = f3()
check_rejected(gen.throw("End"), "End")

// Test 4

async function *f4() {
  assert(++state === 1)
  await 1
  assert(++state === 4)
}

var state = 0
gen = f4()
gen.next()

assert(++state === 2)
gen.next()
assert(++state === 3)

// Test 5

async function *f5() {
  assert(++state2 === 1)
  yield 1
  assert(++state2 === 4)
}

var state2 = 0
gen = f5()
gen.next()

assert(++state2 === 2)
gen.next()
assert(++state2 === 3)

// Test 6

async function *f6() {
  return "Res"
}

var genLate = f6()
var p = genLate.next()

assert(p instanceof Promise)

p.then(function(v) {
  assert(v.value === "Res")
  assert(v.done === true)
  successCount++

  check_fulfilled(genLate.next(), undefined, true)
  var o = {}
  check_rejected(genLate.throw(o), o)
  check_fulfilled(genLate.return(), undefined, true)
}, function() {
  assert(false)
})

// Test 7

var AsyncGeneratorFun = Object.getPrototypeOf(async function *() {}).constructor;

o = {}
gen = AsyncGeneratorFun("p, o, x = 5.5", "assert((await p) === 'P'); yield o; return x")
gen = gen(new Promise(function(resolve, reject) { r = resolve }), o)

check_fulfilled(gen.next(), o, false)
check_fulfilled(gen.next(), 5.5, true)
check_fulfilled(gen.next(), undefined, true)
check_fulfilled(gen.next(), undefined, true)

r("P")

// Test 8

async function *f8() {
  var o = {}
  function f() {}

  check_fulfilled(selfGen.next(), o, false)
  check_fulfilled(selfGen.next(), f, true)
  check_fulfilled(selfGen.next(), undefined, true)

  yield "Str"
  yield o
  return f
}

var selfGen = f8();
check_fulfilled(selfGen.next(), "Str", false)

// Test 9

async function *f9() {
  try {
    yield "X"
  } finally {
    successCount++;
  }
  yield "Y"
  return 1;
}

gen = f9()
check_fulfilled(gen.next(), "X", false)
check_fulfilled(gen.return("Ret"), "Ret", true)
check_fulfilled(gen.next(), undefined, true)

// END

function __checkAsync() {
  assert(successCount === 32)
  assert(state === 4)
  assert(state2 === 4)
}
