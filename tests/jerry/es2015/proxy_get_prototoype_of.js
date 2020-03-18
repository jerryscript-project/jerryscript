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

// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
  assert(e === 42);
}

try {
  // 19.1.3.3
  Object.prototype.isPrototypeOf(proxy);
  assert(false);
} catch (e) {
  assert(e === 42);
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
    assert(e === 42);
  }

  // ecma_op_implicit_class_constructor_has_instance [[GetPrototypeOf]]
  try {
    g instanceof e;
    assert(false);
  } catch (e) {
    assert(e === 42);
  }
})();

try {
  // 9.4.1.3.3
  Function.prototype.bind.call(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test basic functionality
var target = {};
var handler = {
  getPrototypeOf(target) {
    return Array.prototype;
  }
}
var proxy = new Proxy(target, handler);

assert(Object.getPrototypeOf(proxy) === Array.prototype);
assert(Reflect.getPrototypeOf(proxy) === Array.prototype);
assert(Array.prototype.isPrototypeOf(proxy));
assert(proxy instanceof Array);

var obj = Object.preventExtensions({});
assert(Object.getPrototypeOf(obj) === Object.prototype);

var handler = {
  getPrototypeOf(target) {
    return Object.prototype;
  }
}
var proxy = new Proxy(target, handler);
assert(Object.getPrototypeOf(proxy) === Object.prototype);

// test with no trap
var target = {};
var handler = {};
var proxy = new Proxy(target, handler);

assert(Object.getPrototypeOf(proxy) === Object.prototype);

// test with "undefined" trap
var target = {};
var handler = { getPrototypeOf: null };
var proxy = new Proxy(target, handler);

assert(Object.getPrototypeOf(proxy) === Object.prototype);

// test with invalid trap
var target = {};
var handler = { getPrototypeOf: 42 };
var proxy = new Proxy(target, handler);

try {
  Object.getPrototypeOf(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when target is proxy
var target = {};
var handler = {};
var proxy = new Proxy(target, handler);

var target_prototype = {};
handler.getPrototypeOf = function() {
  return target_prototype ;
}

var proxy2 = new Proxy(proxy, handler);
assert(Object.getPrototypeOf(proxy2) === target_prototype);

// test when invariants gets violated
var target = {};
var handler = {
  getPrototypeOf(target) {
    return 'foo';
  }
}
var proxy = new Proxy(target, handler);

try {
  Object.getPrototypeOf(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var target = Object.preventExtensions({});
var handler = {
  getPrototypeOf(target) {
    return {};
  }
}

var proxy = new Proxy(target, handler);

try {
  Object.getPrototypeOf(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
