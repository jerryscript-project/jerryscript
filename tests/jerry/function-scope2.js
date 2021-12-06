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

var a = 1;
var b = 2;

function f(x = eval("eval('var a = 3; function b() { return 4 } () => a')"), y = b) {
  eval("eval('var a = 5; function b() { return 6 }')");

  assert(a === 5);
  assert(b() === 6);

  assert(x() === 3);
  assert(y() === 4);

  delete a;
  delete b;

  assert(a === 3);
  assert(b() === 4);

  assert(x() === 3);
  assert(y() === 4);

  delete a;
  delete b;

  assert(a === 1);
  assert(b === 2);

  assert(x() === 1);
  assert(y() === 4);
}
f()

function g() {
  'use strict'

  function h(x, y = function() { return x }) {
    var x = 2;

    /* This should not overwrite y. */
    eval("var y = 3; assert (y === 3)");

    assert(x === 2);
    assert(typeof y === "function");
    assert(y() === 1);
  }
  h(1);
}
g();

function h(a, get = () => a, set = (v) => a = v) {
  assert(a === 1);

  var a = 2;

  assert(a === 2);
  assert(get() === 1);

  set(3)
  a = 4;

  assert(a === 4);
  assert(get() === 3);
}
h(1);

function i([a], get = () => a, set = (v) => a = v) {
  assert(a === 1);

  var a;
  assert(a === 1);

  a = 2;

  assert(a === 2);
  assert(get() === 1);

  set(3)
  a = 4;

  assert(a === 4);
  assert(get() === 3);
}
i([1]);

function j(a = eval()) {
  var a = 3.14;

  try {
    eval("throw 1; function a() { return 8; }")
    assert(false)
  } catch (e) {
    assert(e === 1)
  }

  assert(a() === 8)
}
j()
