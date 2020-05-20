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

async function f1()
{
  throw 1
}

async function f2()
{
  try {
    assert(await f1() && false)
  } catch (e) {
    assert(e === 1)
    return 2
  } finally {
    return 3
  }
}

async function f3()
{
  return await f2() + 1
}

async function f4()
{
  return await f1()
}

async function f5()
{
  var o = { a:f2, b:f2, c:f2, d:f2 }

  for (i in o) {
    var p1 = f3()
    var p2 = f4()

    assert(await o[i]() === 3)
    assert(await p1 === 4)

    try {
      assert(await p2 && false)
    } catch (e) {
      assert(e === 1)
    }

    successCount++
  }
}

f5()

function __checkAsync() {
  assert(successCount === 4)
}
