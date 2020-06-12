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

function f() { return 4 }

exit: {
  assert(f() === 6);
  break exit;
  function f() { return 6; }
}
assert(f() === 4);

{
  assert(f() === 6);
  f = 1;
  assert(f === 1);
  function f() { return 6; }
  f = 2;
  assert(f === 2);
}
assert(f === 1);

function g() { return 3 }
exit: {
  assert(g() === 5);
  function g() { return 4; }
  break exit;
  function g() { return 5; }
}
assert(g() === 5);

function h() {
  try {
    x;
    assert(false);
  } catch (e) {
    assert(e instanceof ReferenceError);
  }

  eval("exit: { assert(x() === 8); x = 4; break exit; function x() { return 8; } }");
  assert(x === undefined);
}
h();
