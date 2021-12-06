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

var o1 = Promise.reject("Err")
var async1 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:0, done:false }
      },
      throw(v) {
        assert(v === "Except")
        /* Failed result result. */
        throw o1
      }
    }
  }
}

async function *f1() {
  successCount++
  try {
    try {
      yield *async1
      assert(false)
    } finally {
      successCount++
    }
    assert(false)
  } catch (e) {
    assert(e === o1)
    successCount++
    return
  }
  assert(false)
}

var gen = f1()
check_fulfilled(gen.next(), 0, false)
check_fulfilled(gen.throw("Except"), undefined, true)

// Test 2

var o2 = Promise.resolve("Message")
var async2 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:1, done:false }
      },
      throw(v) {
        assert(v === o2)
        /* Successful result. */
        return { value:o2, done:false }
      }
    }
  }
}

async function *f2() {
  successCount++
  try {
    yield *async2
  } finally {
    /* Never completes. */
    assert(false)
  }
}

var gen = f2()
check_fulfilled(gen.next(), 1, false)
check_fulfilled(gen.throw(o2), "Message", false)

// Test 3

var o3 = Promise.resolve("Message")
var async3 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:2, done:false }
      },
      throw(v) {
        assert(v === -2.5)
        /* Successful result. */
        return { value:o3, done:true }
      }
    }
  }
}

async function *f3() {
  successCount++
  assert((yield *async3) === o3)
  successCount++
  return -4.25
}

var gen = f3()
check_fulfilled(gen.next(), 2, false)
check_fulfilled(gen.throw(-2.5), -4.25, true)

// Test 4

var async4 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:3, done:false }
      }
    }
  }
}

async function *f4() {
  successCount++
  try {
    yield *async4
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
    successCount++
  }
}

var gen = f4()
check_fulfilled(gen.next(), 3, false)
check_fulfilled(gen.throw(), undefined, true)

// Test 5

var async5 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:4, done:false }
      },
      return(v) {
        assert(v === undefined)
        throw "Close!"
      }
    }
  }
}

async function *f5() {
  successCount++
  try {
    yield *async5
    assert(false)
  } catch (e) {
    assert(e === "Close!")
    successCount++
  }
}

var gen = f5()
check_fulfilled(gen.next(), 4, false)
check_fulfilled(gen.throw(1), undefined, true)

// Test 6

var o6 = Promise.resolve("Return!")
var async6 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:5, done:false }
      }
    }
  }
}

async function *f6() {
  successCount++
  try {
    yield *async6
    assert(false)
  } finally {
    successCount++
  }
}

var gen = f6()
check_fulfilled(gen.next(), 5, false)
check_fulfilled(gen.return(o6), "Return!", true)

// Test 7

var arr = []
var o7 = Promise.resolve(arr)
var async7 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:6, done:false }
      },
      return(v) {
        assert(v === arr)
        /* Successful result. */
        return { value:o7, done:false }
      }
    }
  }
}

async function *f7() {
  successCount++
  try {
    yield *async7
  } finally {
    /* Never completes. */
    assert(false)
  }
}

var gen = f7()
check_fulfilled(gen.next(), 6, false)
check_fulfilled(gen.return(o7), arr, false)

// Test 8

var o8 = Promise.resolve(6.75)
var async8 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:7, done:false }
      },
      return(v) {
        assert(v === 6.75)
        /* Successful result. */
        return { value:o8, done:true }
      }
    }
  }
}

async function *f8() {
  successCount++
  try {
    yield *async8
  } finally {
    successCount++
  }
  /* Return skips this code path. */
  assert(false)
}

var gen = f8()
check_fulfilled(gen.next(), 7, false)
check_fulfilled(gen.return(o8), o8, true)

// Test 9

var o9 = Promise.reject("reject")
var async9 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:8, done:false }
      },
      throw(v) {
        assert(v === "reject")
        throw "End"
      },
      get return() {
        assert(false)
      }
    }
  }
}

async function *f9() {
  successCount++
  try {
    yield *async9
  } catch (e) {
    successCount++
    assert(e === "End")
    throw e
  }
  /* Throw skips this code path. */
  assert(false)
}

var gen = f9()
check_fulfilled(gen.next(), 8, false)
check_rejected(gen.return(o9), "End")

// Test 10

var o10 = Promise.reject(arr)
var async10 = {
  [Symbol.asyncIterator]() {
    return {
      next(v) {
        assert(v === undefined)
        return { value:9, done:false }
      },
      get return() {
        successCount++
        return function() {
          /* Only called during iterator close. */
          assert(arguments.length === 0)
          successCount++
        }
      }
    }
  }
}

async function *f10() {
  successCount++
  try {
    yield *async10
  } catch (e) {
    successCount++
    assert(e instanceof TypeError)
    throw -3.25
  }
  assert(false)
}

var gen = f10()
check_fulfilled(gen.next(), 9, false)
check_rejected(gen.return(o10), -3.25)

// END

function __checkAsync() {
  assert(successCount == 41)
}
