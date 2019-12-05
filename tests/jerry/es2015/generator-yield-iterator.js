/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function check_result(result, value, done)
{
  assert(result.value === value)
  assert(result.done === done)
}

function *gen1() {
  yield 1
  yield *[2,3,4]
  yield 5
}

var g = gen1()
check_result(g.next(), 1, false)
check_result(g.next(), 2, false)
check_result(g.next(), 3, false)
check_result(g.next(), 4, false)
check_result(g.next(), 5, false)
check_result(g.next(), undefined, true)

function *gen2() {
  yield * true
}

try {
  g = gen2()
  g.next()
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

var t0 = 0, t1 = 0

function *gen3() {
  function *f() {
    try {
      yield 5
    } finally {
      t0 = 1
    }
  }

  try {
    yield *f()
  } finally {
    t1 = 1
  }
}

g = gen3()
check_result(g.next(), 5, false)
check_result(g.return(13), 13, true)
assert(t0 === 1)
assert(t1 === 1)

t0 = -1
t1 = 0

function *gen4() {
  function next(arg)
  {
    t0++;

    if (t0 === 0)
    {
      assert(arg === undefined);
      return { value:2, done:false }
    }
    if (t0 === 1)
    {
      assert(arg === -3);
      return { value:3, done:false }
    }
    assert(arg === -4);
    return { value:4, done:true }
  }

  var o = { [Symbol.iterator]() { return { next } } }
  assert((yield *o) === 4)
  return 5;
}

g = gen4()
check_result(g.next(-2), 2, false)
check_result(g.next(-3), 3, false)
check_result(g.next(-4), 5, true)

function *gen5() {
  function *f() {
    try {
      yield 1
      assert(false)
    } catch (e) {
      assert(e === 10)
    }
    return 2
  }

  assert((yield *f()) === 2)
  yield 3
}

g = gen5()
check_result(g.next(), 1, false)
check_result(g.throw(10), 3, false)
