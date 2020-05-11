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

{
  try {
    f()
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  let a = 1;

  try {
    f()
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  let [b] = [2];

  try {
    f()
    assert(false)
  } catch (e) {
    assert(e instanceof ReferenceError)
  }

  const {c} = { c:3 };

  f();
  function f() { assert(a + b + c === 6) }
}

{
  let a = 3;
  let [b] = [4];
  const {c} = { c:5 };

  let f = (function () { assert(a + b + c === 12) })
  f();

  {
    function g() { assert(a + b + c === 12) }
    g();
  }
}

{
  class C {}

  assert(function () { return C } () === C);
}
