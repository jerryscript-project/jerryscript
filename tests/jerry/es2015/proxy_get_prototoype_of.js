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

// TODO: Update these tests when the internal routine has been implemented

var target = {};
var handler = { getPrototypeOf (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 19.1.2.9.2
  Object.getPrototypeOf(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 19.1.3.3
  Object.prototype.isPrototypeOf(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

(function () {
  class e extends Array {};
  function f () {};
  function g () {};

  Object.setPrototypeOf(g, proxy);

  // 7.3.19.7.b
  try {
    g instanceof f;
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }

  // ecma_op_implicit_class_constructor_has_instance [[GetPrototypeOf]]
  try {
    g instanceof e;
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
})();

try {
  // 9.4.1.3.3
  Function.prototype.bind.call(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
