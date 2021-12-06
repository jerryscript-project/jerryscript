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

function check_syntax_error (code)
{
  try {
    eval (code);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

check_syntax_error ("for await (a of b)");
check_syntax_error ("async function f() { for await (a in b) ; }");
check_syntax_error ("async function f() { for await ( ; ; ) ; }");
check_syntax_error ("async function f() { for await (let a = 0; a < 4; a++) ; }");

var successCount = 0;

// Test 1

var promise1 = Promise.resolve("Resolved");

var asyncIter1 = {
  [Symbol.asyncIterator]() {
    var idx = 0;

    function next() {
      idx++;
      if (idx == 1) {
        return { value:"Val", done: false }
      } else if (idx == 2) {
        return { value:promise1, done: false }
      } else if (idx == 3) {
        return { value:4.5, done: false }
      }
      return { value:promise1, done: true }
    }

    successCount++
    return { next }
  }
}

function checkAsyncIter1(v, idx)
{
  if (idx === 1) {
    assert(v === "Val")
  } else if (idx === 2) {
    assert(v === promise1)
  } else if (idx === 3) {
    assert(v === 4.5)
  } else {
    assert(false)
  }
}

async function f1a() {
  var idx = 0;
  for await (var v of asyncIter1) {
    checkAsyncIter1(v, ++idx);
    successCount++;
  }
  successCount++;
}

f1a()

async function f1b() {
  await promise1

  var idx = 0;
  for await (var v of asyncIter1) {
    checkAsyncIter1(v, ++idx);
    successCount++;
  }
  successCount++;
}

f1b()

async function *f1c() {
  var idx = 0;
  for await (var v of asyncIter1) {
    checkAsyncIter1(v, ++idx);
    successCount++;
  }
  successCount++;
}

f1c().next()

async function *f1d() {
  await promise1

  var idx = 0;
  for await (var v of asyncIter1) {
    checkAsyncIter1(v, ++idx);
    successCount++;
  }
  successCount++;
}

f1d().next()

// Test 2

var state2 = 0
var promise2 = Promise.reject("Rejected");

var asyncIter2 = {
  [Symbol.asyncIterator]() {
    var idx = 0;
    assert(++state2 === 1)

    function next() {
      idx++;
      if (idx == 1) {
        assert(++state2 === 2)
        return { value:"Str", done: false }
      } else if (idx == 2) {
        assert(++state2 === 4)
        return { value:promise2, done: false }
      } else if (idx == 3) {
        assert(++state2 === 6)
        return { value:-3.5, done: false }
      }
      assert(++state2 === 8)
      return { value:promise2, done: true }
    }

    successCount++
    return { next }
  }
}

function checkAsyncIter2(v, idx)
{
  if (idx === 1) {
    assert(v === "Str")
  } else if (idx === 2) {
    assert(v === promise2)
  } else if (idx === 3) {
    assert(v === -3.5)
  } else {
    assert(false)
  }
}

async function *f2a() {
  var idx = 0;
  for await (var v of asyncIter2) {
    checkAsyncIter2(v, ++idx)
    yield v
  }
  successCount++;
}

async function f2b() {
  var idx = 0;
  var g = f2a();
  var v;

  while (true) {
    v = await g.next()

    if (v.done) {
      break;
    }

    checkAsyncIter2(v.value, ++idx)
    ++state2
    assert(state2 === 3 || state2 === 5 || state2 === 7)
  }

  successCount++;
}

f2b();

// Test 3

var o3 = {}
async function* gen3()
{
  yield o3
  yield "Res"
}

async function f3()
{
  var idx = 0

  for await (var v of gen3())
  {
    idx++

    if (idx === 1)
    {
      assert(v === o3)
    }
    else
    {
      assert(idx === 2)
      assert(v === "Res")
    }
  }
  successCount++
}

f3()

// END

function __checkAsync() {
  assert(state2 === 8)
  assert(successCount === 24)
}
