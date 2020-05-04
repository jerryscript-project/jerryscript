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

function f1(a)
{
  assert(a === 2)
  {
    assert(a() === 1)
    function a() { return 1 }
  }
  assert(a === 2)
}
f1(2)

function f2([a])
{
  assert(a === 4)
  {
    assert(a() === 3)
    function a() { return 3 }
  }
  assert(a === 4)
}
f2([4])

function f3(a)
{
  assert(a() === 5)
  {
    assert(a() === 6)
    function a() { return 6 }
  }
  assert(a() === 5)

  function a() { return 5 }
}
f3(7)

function f4(a)
{
  assert(a === 8)
  {
    eval("function a() { return 9 }")
    assert(a() === 9)
  }
  assert(a() === 9)
}
f4(8)

function f5(a, b = function() { return a }) {
  function a() { return 9 }

  assert(a() === 9)
  assert(b() === 10)
}
f5(10)
