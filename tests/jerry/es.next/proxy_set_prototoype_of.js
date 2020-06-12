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
var handler = { setPrototypeOf (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 19.1.2.18
  Object.setPrototypeOf(proxy, {});
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test basic funcionality
var targetProto = {};
var target = {
  foo: false
};

var handler = {
  setPrototypeOf(target, targetrProto) {
    target.foo = true;
    return false;
  }
};

var proxy = new Proxy(target, handler);

assert(Reflect.setPrototypeOf(proxy, targetProto) === false);
assert(target.foo === true);

try {
  Object.setPrototypeOf(proxy, targetProto);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Object.setPrototypeOf(proxy, undefined);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Object.setPrototypeOf(proxy, 1);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var target = {};
var handler = {};
var prototype = [];

var proxy = new Proxy(target, handler);

Object.setPrototypeOf(proxy, prototype);
assert(Object.getPrototypeOf(target) === prototype);

handler.setPrototypeOf = function(target, proto) {
  return false;
};

try {
  Object.setPrototypeOf(proxy, {});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

handler.setPrototypeOf = function(target, proto) {
  return undefined;
};

try {
  Object.setPrototypeOf(proxy, {});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

handler.setPrototypeOf = function(proto) {};

try {
  Object.setPrototypeOf(proxy, {});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var proto = {};
Object.setPrototypeOf(proto, Function.prototype);

var seen_prototype;
var seen_target;

handler.setPrototypeOf = function(target, proto) {
  seen_target = target;
  seen_prototype = proto;
  return true;
}

Object.setPrototypeOf(proxy, proto);
assert(target === seen_target);
assert(proto === seen_prototype);

// test when target is proxy
var target = {};
var handler = {};
var handler2 = {};
var target2 = new Proxy(target, handler2);
var proxy2 = new Proxy(target2, handler);
var prototype = [2,3];

Object.setPrototypeOf(proxy2, prototype);

assert(prototype === Object.getPrototypeOf(target));

// test when invariants gets violated
var target = {};
var handler = {
    setPrototypeOf(target, proto) {
        Object.setPrototypeOf(target, Function.prototype);
        Object.preventExtensions(target);
        return true;
    }
}

var proxy = new Proxy(target, handler);

try {
  Object.setPrototypeOf(proxy, Array.prototype);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
