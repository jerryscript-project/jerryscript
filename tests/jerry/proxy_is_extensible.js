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

// error checks
var target = {};
var handler = { isExtensible (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 7.3.15
  Object.isFrozen(proxy)
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // 7.3.15
  Object.isSealed(proxy)
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // 7.2.5
  Object.isExtensible(proxy)
  assert(false);
} catch (e) {
  assert(e === 42);
}

// no trap
var target = {};
var handler = {};
var proxy = new Proxy(target, handler);
assert(Object.isExtensible(target) === true)
assert(Object.isExtensible(proxy) === true);
Object.preventExtensions(target);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy) === false);

// undefined trap
var target = {};
var handler =  { isExtensible: null };
var proxy = new Proxy(target, handler);
assert(Object.isExtensible(target) === true)
assert(Object.isExtensible(proxy) === true);
Object.preventExtensions(target);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy) === false);

// invalid trap
var target = {};
var handler =  { isExtensible: true };
var proxy = new Proxy(target, handler);

try {
  Object.isExtensible(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError); 
}

// valid trap
var target = { prop1: true };
var handler = {
  isExtensible(target) {
    target.prop1 = false;
    return Object.isExtensible(target);
  }
};

var proxy = new Proxy(target, handler);
assert(Object.isExtensible(proxy) === true);
assert(target.prop1 === false);
Object.preventExtensions(target);
assert(Object.isExtensible(target) === false);
assert(Object.isExtensible(proxy) === false);

// trap result is invalid
var target = {};
var handler = {
  isExtensible(target) {
    return false;
  }
};

var proxy = new Proxy(target, handler);

try {
  Object.isExtensible(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// handler is null
var target = {};
var handler = { 
  isExtensible (target) {
    return Object.isExtensible(target);
  }
};

var revocable = Proxy.revocable (target, {});
var proxy = revocable.proxy;
revocable.revoke();

try {
  Object.isExtensible(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// proxy within proxy
var target = {};
var handler1 = {
  isExtensible(target) {
    return Object.isExtensible(target);
  }
};

var handler2 = {
  isExtensible(target) {
    return false;
  }
};

var proxy = new Proxy(target, handler1);
var proxy2 = new Proxy(proxy, handler1);
assert(Object.isExtensible(proxy2) === true);

var proxy3 = new Proxy(proxy, handler2);

try {
  Object.isExtensible(proxy3);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var proxy4 = new Proxy(proxy2, handler1);
Object.preventExtensions(target);
assert(Object.isExtensible(proxy4) === false);
