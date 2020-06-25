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

// Test 1

var o1 = {}
var arr1 = []
var async1 = {
  [Symbol.asyncIterator]() {
    arr1 = []
    var i = 0
    return {
      next(v) {
        var res

        if (i == 0) {
          assert(v === undefined)
          res = { value:"Res", done:false }
        } else if (i == 1) {
          assert(v === "B")
          res = { value:{}, done:false }
        } else if (i == 2) {
          assert(v === o1)
          res = Promise.resolve("Nested")
          res = { value:res, done:false }
        } else {
          assert(v === -1.5)
          res = { value:3.5, done:true }
        }
        i++

        arr1.push(res)
        return Promise.resolve(res)
      }
    }
  }
}

async function *f1() {
  successCount++
  assert((yield *async1) === 3.5)
  successCount++
  return "End"
}

async function f1_run() {
  var gen = f1()

  var res = await gen.next("A")
  assert(res != arr1[0])
  assert(res.value === "Res")
  assert(arr1[0].value === "Res")
  assert(res.done === false)
  assert(arr1[0].done === false)
  successCount++

  var res = await gen.next("B")
  assert(res != arr1[1])
  assert(res.value === arr1[1].value)
  assert(res.done === false)
  assert(arr1[1].done === false)
  successCount++

  var res = await gen.next(o1)
  assert(res != arr1[2])
  assert(res.value === "Nested")
  assert(arr1[2].value instanceof Promise)
  assert(res.done === false)
  assert(arr1[2].done === false)
  successCount++

  var res = await gen.next(-1.5)
  assert(res.value === "End")
  assert(res.done === true)
  successCount++
}

f1_run()

// Test 2

var o2 = {}
var async2 = {
  [Symbol.asyncIterator]() {
    return {
      next() {
        throw "Except"
      }
    }
  }
}

async function *f2() {
  successCount++
  try {
    try {
      yield *async2
      assert(false)
    } finally {
      successCount++
    }
    assert(false)
  } catch (e) {
    assert(e === "Except")
    successCount++
    throw o2
  }
  assert(false)
}

var gen = f2()
check_rejected(gen.next(), o2)

// Test 3

var o3 = {}
var async3 = {
  [Symbol.asyncIterator]() {
    var i = 0
    return {
      next() {
        if (i == 0) {
          i++
          return { value:6.25, done:false }
        }
        throw o3
      }
    }
  }
}

async function *f3() {
  successCount++
  try {
    try {
      yield *async3
      assert(false)
    } finally {
      successCount++
    }
    assert(false)
  } catch (e) {
    assert(e === o3)
    successCount++
    return o3
  }
  assert(false)
}

var gen = f3()
check_fulfilled(gen.next(), 6.25, false)
check_fulfilled(gen.next(), o3, true)

// Test 4

var async4 = {
  [Symbol.asyncIterator]() {
    return {
      next() {
        /* Returns with a promise which fails. */
        return { value:Promise.reject("Failed!"), done:false }
      }
    }
  }
}

async function *f4() {
  successCount++
  try {
    try {
      yield *async4
      assert(false)
    } finally {
      successCount++
    }
    assert(false)
  } catch (e) {
    assert(e === "Failed!")
    successCount++
    return
  }
  assert(false)
}

var gen = f4()
check_fulfilled(gen.next(), undefined, true)

// Test 5

var async5 = {
  [Symbol.asyncIterator]() {
    return {
      next() {
        /* Returns with a promise which fails. */
        return { value:Promise.reject("FailedAndDone!"), done:true }
      }
    }
  }
}

async function *f5() {
  successCount++
  try {
    var p = yield *async5
    assert(p instanceof Promise)
    check_rejected(p, "FailedAndDone!")
    successCount++
  } catch (e) {
    assert(false)
  }
}

var gen = f5()
check_fulfilled(gen.next(), undefined, true)

// Test 6

var state = 0

var o6 = {}
var async6 = {
  [Symbol.asyncIterator]() {
    var i = 0
    assert(++state === 2)

    return {
      next() {
        i++
        if (i == 1) {
          assert(++state === 3)
          return { value:5.75, done:false }
        } else if (i == 2) {
          assert(++state === 7)
          return { value:o6, done:false }
        } else if (i == 3) {
          assert(++state === 8)
          return { value:"Val", done:true }
        }
      }
    }
  }
}

async function *f6() {
  assert(++state === 1)

  assert((yield *async6) === "Val")

  assert(++state === 9)
  return "End"
}

var gen = f6()
check_fulfilled(gen.next(), 5.75, false)
assert(++state === 4)
check_fulfilled(gen.next(), o6, false)
assert(++state === 5)
check_fulfilled(gen.next(), "End", true)
assert(++state === 6)

// END

function __checkAsync() {
  assert(successCount === 26)
  assert(state === 9)
}
