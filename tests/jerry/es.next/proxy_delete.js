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
var handler = { deleteProperty (target) {
  throw 42;
}, get (object, propName) {
  if (propName == "length") {
    return 5;
  }
}};

var proxy = new Proxy(target, handler);

var a = 5;

// ecma_op_delete_binding
with (proxy) {
  delete a
}

try {
  // 22.1.3.16.6.e
  Array.prototype.pop.call(proxy);
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test basic functionality
var target = {foo: "bar"};
var handler = {
  deleteProperty(obj, prop) {
    delete obj[prop];
  }
}

var proxy = new Proxy(target, handler);

assert(target.foo === "bar")
assert(proxy.foo === "bar");

assert(delete proxy.foo === false);

assert(target.foo === undefined);
assert(proxy.foo === undefined);

assert(target.bar === undefined);
assert(delete proxy.bar == false);
assert(target.bar === undefined);

var handler2 = {
  deleteProperty(obj, prop) {
    delete obj[prop];
    return true;
  }
}

var proxy = new Proxy(target, handler2);

assert(target.bar === undefined);
assert(delete proxy.bar == true);
assert(target.bar === undefined);

// test with no trap
var target = {1: 42};
var handler = {};
var proxy = new Proxy(target, handler);

assert(target[1] === 42)
assert(delete proxy[1] === true)
assert(target[1] === undefined);

// test with undefined trap
var target = {2: 52};
var handler = { deleteProperty: null};
var proxy = new Proxy(target, handler);

assert(target[2] === 52)
assert(delete proxy[2] === true)
assert(target[2] === undefined);

// test when trap is invalid
var target = {};
var handler = { deleteProperty: true };
var proxy = new Proxy(target, handler);

try {
  delete proxy[0];
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when handler is null
var revocable = Proxy.revocable ({}, {});
var proxy = revocable.proxy;
revocable.revoke();

try {
  delete proxy.foo;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when target is proxy
var target = {prop: "foo"};
var handler = {
  deleteProperty(obj, prop) {
    delete obj[prop];
  }
};

var proxy1 = new Proxy(target, handler);
var proxy2 = new Proxy(proxy1, handler);

assert(target.prop === "foo");
assert(proxy1.prop === "foo");
assert(proxy2.prop === "foo");

delete proxy2.prop;

assert(target.prop === undefined);
assert(proxy1.prop === undefined);
assert(proxy2.prop === undefined);

// tests when invariants gets violated
var target = {};
var handler = {
  deleteProperty(obj, prop) {
    delete obj[prop];
    return true;
  }
};

Object.defineProperty(target, "foo", {
  configurable: false,
  value: "foo"
});

var proxy = new Proxy (target, handler);

try {
  delete proxy.foo;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var trapCalls = 0;
var p = new Proxy({prop: 1}, {
  deleteProperty: function(t, prop) {
    Object.preventExtensions(t);
    trapCalls++;
    return true;
  },
});

try {
  Reflect.deleteProperty (p, "prop");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert (trapCalls == 1);
