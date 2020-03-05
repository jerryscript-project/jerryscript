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
var handler = { preventExtensions (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 7.3.14
  Object.freeze(proxy)
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // 7.3.14
  Object.seal(proxy)
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test with no trap
var target = {};
var handler = {};
var proxy = new Proxy(target, handler);

assert(Object.isExtensible(target) === true);
assert(Object.isExtensible(proxy) === true);
Object.preventExtensions(proxy);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy) === false);

// test with "undefined" trap
var target = {};
var handler = { preventExtensions: null };
var proxy = new Proxy(target, handler);

assert(Object.isExtensible(target) === true);
assert(Object.isExtensible(proxy) === true);
Object.preventExtensions(proxy);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy) === false);

// test with invalid trap
var target = {};
var handler = { preventExtensions: 42 };
var proxy = new Proxy(target, handler);

try {
  Object.preventExtensions(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test with valid trap
var target = { foo: "bar" };
var handler = {
  preventExtensions(target) {
    target.foo = "foo"
    Object.preventExtensions(target);
    return true;
  }
}

var proxy = new Proxy(target, handler);
assert(Object.isExtensible(target) === true);
assert(Object.isExtensible(proxy) === true);
assert(target.foo === "bar");
Object.preventExtensions(proxy);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy) === false);
assert(target.foo === "foo");

// test when invariants gets violated
var target = {};
var handler = {
  preventExtensions(target) {
    return true;
  }
}

var proxy = new Proxy(target, handler);

try {
  Object.preventExtensions(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when target is proxy
var target = {};
var handler = {
  preventExtensions(target) {
    Object.preventExtensions(target);
    return true;
  }
}

var proxy1 = new Proxy(target, handler);
var proxy2 = new Proxy(proxy1, handler);

assert(Object.isExtensible(target) === true);
assert(Object.isExtensible(proxy1) === true);
assert(Object.isExtensible(proxy2) === true);
Object.preventExtensions(proxy2);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy1) === false);
assert(Object.isExtensible(proxy2) === false);

var target = {};
var handler = {
  preventExtensions(target) {
    return true;
  }
}

var proxy1 = new Proxy(target, handler);
var proxy2 = new Proxy(proxy1, handler);

try {
  Object.preventExtensions(proxy2);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
