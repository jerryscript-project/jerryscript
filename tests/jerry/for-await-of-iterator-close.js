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

var successCount = 0;

// Test 1

var asyncIter1 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: "Val", done: false })
      }
      /* No return() function */
    }
  }
}

async function f1() {
  for await (var v of asyncIter1) {
    assert(v === "Val")
    break
  }
  successCount++

exit:
  for await (var v of asyncIter1) {
    for await (var v of asyncIter1) {
      assert(v === "Val")
      break exit
    }
    assert(false)
  }
  successCount++

  try {
    for await (var v of asyncIter1) {
      assert(v === "Val")
      throw 3.75
    }
    assert(false)
  } catch (e) {
    assert(e === 3.75)
    successCount++
  }

  try {
    for await (var v of asyncIter1) {
      assert(v === "Val")
      return {}
    }
    assert(false)
  } finally {
    successCount++
  }
  assert(false)
}

f1()

// Test 2

var o2 = {}
var returnCount2 = 0
var asyncIter2 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: o2, done: false })
      },
      return(...v) {
        assert(v.length === 0)
        returnCount2++
        return Promise.resolve({})
      }
    }
  }
}

async function f2() {
  for await (var v of asyncIter2) {
    assert(v === o2)
    break
  }
  successCount++

exit:
  for await (var v of asyncIter2) {
    for await (var v of asyncIter2) {
      assert(v === o2)
      break exit
    }
    assert(false)
  }
  successCount++

  try {
    for await (var v of asyncIter2) {
      assert(v === o2)
      throw o2
    }
    assert(false)
  } catch (e) {
    assert(e === o2)
    successCount++
  }

  try {
    for await (var v of asyncIter2) {
      assert(v === o2)
      return "Ret"
    }
    assert(false)
  } finally {
    successCount++
  }
  assert(false)
  print("OK")
}

f2()

// Test 3

var asyncIter3 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      return() {
        throw "Error"
      }
    }
  }
}

async function *f3() {
  try {
    for await (var v of asyncIter3) {
      assert(v === -4.5)
      break
    }
    assert(false)
  } catch (e) {
    assert(e === "Error")
    successCount++
  }

  try {
    for await (var v of asyncIter3) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } catch (e) {
    assert(e === "Error")
    successCount++
  }

  try {
    for await (var v of asyncIter3) {
      assert(v === -4.5)
      throw "Exit"
    }
    assert(false)
  } catch (e) {
    assert(e === "Exit")
    successCount++
  }
}

f3().next()

// Test 4

var o4 = {}
var asyncIter4 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      get return() {
        throw o4
      }
    }
  }
}

async function *f4() {
  try {
    for await (var v of asyncIter4) {
      assert(v === -4.5)
      break
    }
    assert(false)
  } catch (e) {
    assert(e === o4)
    successCount++
  }

  try {
    for await (var v of asyncIter4) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } catch (e) {
    assert(e === o4)
    successCount++
  }

  try {
    for await (var v of asyncIter4) {
      assert(v === -4.5)
      throw 9.25
    }
    assert(false)
  } catch (e) {
    assert(e === 9.25)
    successCount++
  }
}

f4().next()

// Test 5

var asyncIter5 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      get return() {
        return "Not callable"
      }
    }
  }
}

async function f5() {
  try {
    for await (var v of asyncIter5) {
      assert(v === -4.5)
      break
    }
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
    successCount++
  }

  try {
    for await (var v of asyncIter5) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
    successCount++
  }

  var o = {}
  try {
    for await (var v of asyncIter5) {
      assert(v === -4.5)
      throw o
    }
    assert(false)
  } catch (e) {
    assert(e === o)
    successCount++
  }
}

f5()

// Test 6

var asyncIter6 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      return() {
        return Promise.resolve(4.5)
      }
    }
  }
}

async function f6() {
  try {
    for await (var v of asyncIter6) {
      assert(v === -4.5)
      break
    }
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
    successCount++
  }

  try {
    for await (var v of asyncIter6) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
    successCount++
  }

  try {
    for await (var v of asyncIter6) {
      assert(v === -4.5)
      throw true
    }
    assert(false)
  } catch (e) {
    assert(e === true)
    successCount++
  }
}

f6()

// Test 7

var asyncIter7 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      return() {
        return Promise.reject("Rejected")
      }
    }
  }
}

async function f7() {
  try {
    for await (var v of asyncIter7) {
      assert(v === -4.5)
      break
    }
    assert(false)
  } catch (e) {
    assert(e === "Rejected")
    successCount++
  }

  try {
    for await (var v of asyncIter7) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } catch (e) {
    assert(e === "Rejected")
    successCount++
  }

  try {
    for await (var v of asyncIter7) {
      assert(v === -4.5)
      throw true
    }
    assert(false)
  } catch (e) {
    assert(e === true)
    successCount++
  }
}

f7()

// Test 8

var asyncIter8 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      return() {
        return {}
      }
    }
  }
}

async function f8() {
  for await (var v of asyncIter8) {
    assert(v === -4.5)
    break
  }
  successCount++

  try {
    for await (var v of asyncIter8) {
      assert(v === -4.5)
      throw null
    }
    assert(false)
  } catch (e) {
    assert(e === null)
    successCount++
  }

  try {
    for await (var v of asyncIter8) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } finally {
    successCount++
  }
}

f8()

// Test 9

var asyncIter9 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        assert(++idx === 1)
        return Promise.resolve({ value: -4.5, done: false })
      },
      return() {
        throw "Except"
      }
    }
  }
}

async function f9() {
  try {
    for await (var v of asyncIter9) {
      assert(v === -4.5)
      break
    }
    assert(false)
  } catch (e) {
    assert(e === "Except")
    successCount++
  }

  try {
    for await (var v of asyncIter9) {
      assert(v === -4.5)
      throw 7.5
    }
    assert(false)
  } catch (e) {
    assert(e === 7.5)
    successCount++
  }

  try {
    for await (var v of asyncIter9) {
      assert(v === -4.5)
      for await (var v of asyncIter9) {
        assert(v === -4.5)
        for await (var v of asyncIter9) {
          assert(v === -4.5)
          throw "Leave"
        }
        assert(false)
      }
      assert(false)
    }
    assert(false)
  } catch (e) {
    assert(e === "Leave")
    successCount++
  }

  try {
    for await (var v of asyncIter9) {
      assert(v === -4.5)
      return
    }
    assert(false)
  } catch (e) {
    assert(e === "Except")
    successCount++
  }
}

f9()

// Test 10

var asyncIter10 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        throw "NoNext"
      },
      return() {
        assert(false)
      },
      throw() {
        assert(false)
      }
    }
  }
}

async function f10() {
  try {
    try {
      for await (var v of asyncIter10) {
        assert(false)
      }
      assert(false)
    } finally {
      successCount++
    }
  } catch (e) {
    assert(e === "NoNext")
    successCount++
  }
}

f10()

// Test 11

var asyncIter11 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    return {
      next() {
        if (++idx < 3)
          return Promise.resolve({ value: -4.5, done: false })
        throw "NoNext"
      },
      return() {
        assert(false)
      },
      throw() {
        assert(false)
      }
    }
  }
}

async function f11() {
  try {
    try {
      for await (var v of asyncIter11) {
        assert(v === -4.5)
      }
      assert(false)
    } finally {
      successCount++
    }
  } catch (e) {
    assert(e === "NoNext")
    successCount++
  }
}

f11()

// Test 12

var o12 = {}
async function *gen12()
{
  try {
    yield 9.5
    assert(false)
  } finally {
    successCount++
  }
  assert(false)
}

async function f12()
{
  for await (var v of gen12())
  {
    assert(v === 9.5)
    break;
  }
  successCount++
}

f12()

// END

function __checkAsync() {
  assert(returnCount2 === 5)
  assert(successCount === 36)
}
