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
var handler = { has (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 12.9.3
  "foo" in proxy;
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // 8.1.1.2.1
  with (proxy) {
    p;
    assert(false);
  }
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // ecma_op_put_value_lex_env_base/[[HasProperty]]
  with (proxy) {
    function a (){}
    assert(false);
  }
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test basic functionality
var target = {
  "target_one": 1
};

var handler = {
  has: function(target, prop) {
    return prop == "present";
  }
}

var proxy = new Proxy(target, handler);

assert("present" in proxy === true);
assert("non_present" in proxy === false);

var target = {
  foo: 5,
  bar: 10
};

var handler = {
  has: function(target, prop) {
    if (prop[0] === 'f') {
      return false;
    }
    return prop in target;
  }
};

var proxy = new Proxy(target, handler);

assert("foo" in proxy === false);
assert("bar" in proxy === true);

// test with no trap
var target = {
  foo: "foo"
};
var handler = {};
var proxy = new Proxy(target, handler);

assert("foo" in proxy === true);

// test with "undefined" trap
var target = {
  foo: "foo"
};
var handler = {has: null};
var proxy = new Proxy(target, handler);

assert("foo" in proxy === true);

// test with invalid trap
var target = {
  foo: "foo"
};
var handler = {has: 42};
var proxy = new Proxy(target, handler);

try {
  "foo" in proxy;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when target is proxy
var target = {
  foo: "foo"
};

var handler = {
  has: function(target, prop) {
    return prop in target;
  }
}

var proxy1 = new Proxy(target, handler);
var proxy2 = new Proxy(proxy1, handler);

assert("foo" in proxy2 === true);

// test when invariants gets violated
var target = {};

Object.defineProperty(target, "prop", {
  configurable: false,
  value: 10
})

var handler = {
  has: function(target, prop) {
    return false;
  }
}

var proxy = new Proxy(target, handler);

try {
  'prop' in proxy;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var target = { a: 10 };
Object.preventExtensions(target);

var proxy = new Proxy(target, handler);

try {
  'a' in proxy;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
