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
var handler = { get (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // vm_op_get_value
  proxy.a
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // ecma_op_get_value_object_base
  proxy[2];
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // @@toPrimitive symbol
  proxy + "foo";
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test basic funcionality
var target = {
  target_one: 1,
  prop: "value"
};

var handler = {handler: 1};
var proxy = new Proxy(target, handler);

assert(proxy.prop === "value");
assert(proxy.nothing === undefined);
assert(proxy.handler === undefined);

handler.get = function () {return "value 2"};

assert(proxy.prop === "value 2");
assert(proxy.nothing === "value 2");
assert(proxy.handler === "value 2");

var handler2 = new Proxy({get: function() {return "value 3"}}, {});
var proxy2 = new Proxy(target, handler2);

assert(proxy2.prop === "value 3");
assert(proxy2.nothing === "value 3");
assert(proxy2.handler === "value 3");

var get = [];
var p = new Proxy([0,,2,,4,,], { get: function(o, k) { get.push(k); return o[k]; }});
Array.prototype.reverse.call(p);

assert(get + '' === "length,0,4,2");

// test when get throws an error
var handler = new Proxy({}, {get: function() {throw 42;}});
var proxy = new Proxy ({}, handler);

try {
  proxy.prop;
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when trap is undefined
var handler = new Proxy({}, {get: function() {return undefined}});
var target = {prop: "value"};
var proxy = new Proxy(target, handler);
assert(proxy.prop === "value");
assert(proxy.prop2 === undefined);

// test when invariants gets violated
var target = {};
var handler = {get: function(r, p){if (p != "key4") return "value"}}
var proxy = new Proxy(target, handler);

assert(proxy.key === "value");
assert(proxy.key2 === "value");
assert(proxy.key3 === "value");
assert(proxy.key4 === undefined);

Object.defineProperty(target, "key", {
  configurable: false,
  writable: false,
  value: "different value"
});

try {
  proxy.key;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError)
}

Object.defineProperty(target, "key2", {
  configurable: false,
  get: function() {return "different value"}
});

assert(proxy.key2 === "value");

Object.defineProperty(target, "key3", {
  configurable: false,
  set: function() {}
});

try {
  proxy.key3;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError)
}
