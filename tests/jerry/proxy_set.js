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

// test basic funcionality
function Monster() {
  this.eyeCount = 4;
}

var handler = {
  set(obj, prop, value) {
    if (prop == 'eyeCount') {
      obj[prop] = value;
    } else {
      obj[prop] = "foo";
    }
  }
};

var monster = new Monster();
var proxy = new Proxy(monster, handler);

proxy.eyeCount = 1;
proxy.foo = "bar";

assert(monster.eyeCount === 1);
assert(monster.foo === "foo");

var target = { foo: "foo"};
var handler = {
  set: function(obj, prop, value) {
    obj[prop] = "";
  }
};
var proxy = new Proxy(target, handler);

proxy.foo = 12;
assert(target.foo === "");

var properties = ["bla", "0", 1, Symbol(), {[Symbol.toPrimitive]() {return "a"}}];

var target = {};
var handler = {};
var proxy = new Proxy(target, handler);

// test when property does not exist on target
/* TODO: Enable these tests when Proxy.[[GetOwnProperty]] has been implemented
for (var p of properties) {
  proxy.p = 42;
  assert(target.p === 42);
}

// test when property exists as writable data on target
for (var p of properties) {
  Object.defineProperty(target, p, {
    writable: true,
    value: 24
  });

  proxy.p = 42;
  assert(target.p === 42);
}
*/
// test when target is a proxy
var target = {};
var handler = {
  set(obj, prop, value) {
    obj[prop] = value;
  }
};

var proxy = new Proxy(target, handler);
var proxy2 = new Proxy(proxy, handler);

proxy2.prop = "foo";

assert(target.prop === "foo");

// test when handler is null
var target = {};
var handler = {
  set(obj, prop, value) {
    obj[prop] = value;
  }
};

var revocable = Proxy.revocable (target, {});
var proxy = revocable.proxy;
revocable.revoke();

try {
  proxy.prop = 42;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when invariants gets violated
var target = {};
var handler = { set() {return 42} };
var proxy = new Proxy(target, handler);

Object.defineProperty(target, "key", {
  configurable: false,
  writable: false,
  value: 0
});

try {
  proxy.key = 600;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

Object.defineProperty(target, "key2", {
  configurable: false,
  set: undefined
});

try {
  proxy.key2 = 500;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* Setting a new property should be configurable, enumerable, and writable by default
 * over a proxy.
 */
var setPropProxy = new Proxy({}, {});
setPropProxy["alma"] = 3;

var desc = Object.getOwnPropertyDescriptor(setPropProxy, "alma");

assert(desc.writable === true);
assert(desc.enumerable === true);
assert(desc.configurable === true);
assert(desc.value === 3);
