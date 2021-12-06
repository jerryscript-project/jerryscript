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

var target = function () {};
var handler = { defineProperty (target) {
  throw 42;
}, construct () {
  return {};
}};

var proxy = new Proxy(target, handler);

// 22.1.2.3.8.c
Array.of.call(proxy, 5)

// test basic functionality
var g_target, g_name;

var handler = {
  defineProperty: function(target, name, desc) {
    g_target = target;
    g_name = name;
    return true;
  }
}

var target = {};
var proxy = new Proxy(target, handler);
var desc = { value: 1, writable: true, configurable: true };

Object.defineProperty(proxy, "foo", desc);

assert(target === g_target);
assert("foo" === g_name);

var handler = {
  defineProperty: function(target, name, desc) {
    Object.defineProperty(target, name, desc);
  }
}

var proxy = new Proxy(target, handler);

try {
  Object.defineProperty(proxy, "bar", desc);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var bar = Object.getOwnPropertyDescriptor(proxy, "bar");
assert(bar.value === 1);
assert(bar.writable);
assert(bar.configurable);

try {
  Object.defineProperty(proxy, "name", {});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}


// test when trap is not callable
var target = {};
var handler = {
  defineProperty: 1
}

var proxy = new Proxy(target, handler);

try {
  Object.defineProperty(proxy, "foo", {value: "foo"});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when trap is undefined
var target = {};
var handler = {
  defineProperty: undefined
}

var proxy = new Proxy(target, handler);
var desc = { value: 1 };

Object.defineProperty(proxy, "prop1", desc);
assert(proxy.prop1 === 1);

var target2 = {};
var proxy2 = new Proxy(target2, {});

Object.defineProperty(proxy2, "prop2", desc);
assert(proxy2.prop2 === 1);

// test when invariants gets violated
var target = {};
var handler = {
  defineProperty: function(target, name, desc) {
    return true;
  }
}

var proxy = new Proxy(target, handler);

Object.preventExtensions(target);

try {
  Object.defineProperty(proxy, "foo", {value: 1});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var target = {};
var desc = {value: 1, writable: true, configurable: false, enumerable: true};

var proxy = new Proxy(target, handler);

try {
  Object.defineProperty(proxy, "foo", desc);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var target = {};
var handler = {
  defineProperty: function(target, name, desc) {
    return true;
  }
}

var proxy = new Proxy(target, handler);

Object.defineProperty(target, "foo", {value: 1, writable: false, configurable: false});

try {
  Object.defineProperty(proxy, 'foo', {value: 2});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

target.bar = "baz";

try {
  Object.defineProperty(proxy, 'bar', {value: 2, configurable: false});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var trapCalls = 0;
var p = new Proxy({}, {
  defineProperty: function(t, prop, desc) {
    Object.defineProperty(t, prop, {
      configurable: false,
      writable: true,
    });

    trapCalls++;
    return true;
  },
});

try {
  Reflect.defineProperty (p, "prop", {
    writable: false,
  });
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert (trapCalls == 1)
