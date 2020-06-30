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

var e = -1

function f1()
{
  assert(e === undefined)
  try {
    throw 0
  } catch (e) {
    var e = 1
    assert(e === 1)
  }
  assert(e === undefined)
}
f1()

function f2()
{
  assert(e === undefined)
  try {
    throw 0
  } catch (e) {
    {
      var e = 2
      assert(e === 2)
    }
    assert(e === 2)
  }
  assert(e === undefined)
}
f2()

function f3()
{
  assert(e === -1)
  try {
    throw [0]
  } catch ([e]) {
    {
      try {
        eval("var e = 2")
        assert(false)
      } catch (e) {
        assert(e instanceof SyntaxError)
      }
    }
    assert(e === 0)
  }
  assert(e === -1)
}
f3()

function f4()
{
  assert(e === undefined)
  try {
    throw 0
  } catch (e) {
    {
      function e() { return 3 }
      assert(e() === 3)
    }
    assert(e === 0)
  }
  assert(e() === 3)
}
f4()

function f5()
{
  assert(e === -1)
  try {
    throw 0
  } catch (e) {
    {
      eval("function e() { return 3 } assert(e === 0)")
      assert(e === 0)
    }
    assert(e === 0)
  }
  assert(e() === 3)
}
f5()

function f6()
{
  let e = 4;
  assert(e === 4)
  try {
    throw 0
  } catch (e) {
    {
      function e() { return 5 }
      assert(e() === 5)
    }
    assert(e === 0)
  }
  assert(e === 4)
}
f6()

function f7()
{
  let e = 6;
  assert(e === 6)
  try {
    throw 0
  } catch (e) {
    {
      eval("function e() { return 7 } assert(e() === 7)")
    }
    assert(e === 0)
  }
  assert(e === 6)
}
f7()

function f8()
{
  var cnt = 0;

  try {
    throw "A"
    asert(false)
  } catch {
    cnt++
  }

  let i = 0
  const j = 0

  try {
    throw {}
    asert(false)
  } catch {
    const i = 1.5
    let j = 2
    cnt += i * j
  }

  assert(i === 0)
  assert(j === 0)
  return cnt
}

try {
  assert(f8() === 4)
} catch {
  assert(false)
}
