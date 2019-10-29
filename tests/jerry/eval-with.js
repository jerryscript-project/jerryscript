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

function f() {
  try {
    a;
    assert (false);
  } catch (e) {
    assert (e instanceof ReferenceError);
  }

  var o = { a:1 };

  with (o)
  {
    eval("var a = 2")
  }

  assert (o.a === 2);
  assert (a === undefined);
}
f();

function g() {
  try {
    a;
    assert (false);
  } catch (e) {
    assert (e instanceof ReferenceError);
  }

  var o = { a:1 };

  with (o)
  {
    eval("function a() { return 8; }")
  }

  assert (o.a === 1);
  assert (a() === 8);
}
g();
