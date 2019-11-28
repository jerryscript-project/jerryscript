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

function * gen1(a) {
  return "a: " + (yield a.p)
}

var f = gen1({})
check_result(f.return(4), 4, true)
check_result(f.next(), undefined, true)

f = gen1({ p:"x" })
check_result(f.next(), "x", false)
check_result(f.return(10), 10, true)
check_result(f.next(), undefined, true)

f = gen1({ p:"b" })
check_result(f.next(), "b", false)
check_result(f.next(), "a: undefined", true)
check_result(f.next(), undefined, true)

function*gen2() {
  try {
    for (let i in { x:1, y:2 })
    {
      assert((yield i) === "33")
    }
    assert(false)
  } catch (e) {
    assert(false)
  } finally {
    yield "z"
  }
}

f = gen2()
check_result(f.return("ret"), "ret", true)
check_result(f.next(), undefined, true)

f = gen2()
check_result(f.next(), "x", false)
check_result(f.return("ret"), "z", false)
check_result(f.next(), "ret", true)
check_result(f.next(), undefined, true)

function* gen3() {
  try {
    return 8
  } finally {
    yield 1
  }
}

f = gen3()
check_result(f.next(), 1, false)
check_result(f.return(2), 2, true)
