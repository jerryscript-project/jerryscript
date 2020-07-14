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

function array_check(result_array, expected_array) {
  assert(result_array instanceof Array);
  assert(result_array.length === expected_array.length);
  for (var idx = 0; idx < expected_array.length; idx++) {
    assert(result_array[idx] === expected_array[idx]);
  }
}

var target = {};
var handler = { ownKeys (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 19.1.2.14.4
  Object.keys(proxy);
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // 19.1.2.7.1
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e === 42);
}

try {
  // 19.1.2.8.1
  Object.getOwnPropertySymbols(proxy);
  assert(false);
} catch (e) {
  assert(e === 42);
}

var handler = { ownKeys (target) {
  return ["a"];
}};

var proxy = new Proxy(target, handler);
var sort_result = Array.prototype.sort.call(proxy);
assert(Object.keys(sort_result).length === 0);

// test basic functionality
var symA = Symbol("smA");
var symB = Symbol("smB");
var target = { prop1: "prop1", prop2: "prop2"};
target[symB] = "s3";
var handler = {
  ownKeys: function(target) {
    return ["foo", "bar", symA];
  }
}

var proxy = new Proxy(target, handler);

array_check(Reflect.ownKeys(proxy), ["foo", "bar", symA]);
array_check(Object.getOwnPropertyNames(proxy), ["foo", "bar"]);
array_check(Object.keys(proxy), []);
array_check(Object.getOwnPropertySymbols(proxy), [symA]);

handler.ownKeys = function(target) {return Object.getOwnPropertyNames(target);};

array_check(Reflect.ownKeys(proxy), ["prop1", "prop2"]);
array_check(Object.getOwnPropertyNames(proxy), ["prop1", "prop2"]);
array_check(Object.keys(proxy), ["prop1", "prop2"]);
array_check(Object.getOwnPropertySymbols(proxy), []);

// test with no trap
var target = { prop1: "prop1", prop2: "prop2"};
var handler = {};
var proxy = new Proxy(target, handler);

assert(JSON.stringify(Object.getOwnPropertyNames(proxy)) === '["prop1","prop2"]');

// test wtih "undefined" trap
var target = { prop1: "prop1", prop2: "prop2"};
var handler = {ownKeys: null};
var proxy = new Proxy(target, handler);

assert(JSON.stringify(Object.getOwnPropertyNames(proxy)) === '["prop1","prop2"]');

// test with invalid trap
var target = { prop1: "prop1", prop2: "prop2"};
var handler = {ownKeys: 42};
var proxy = new Proxy(target, handler);

try {
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when CreateListFromArrayLike called on non-object
var target = { prop1: "prop1", prop2: "prop2"};
var handler = {
	ownKeys: function(target) {
      return "foo";
    }
};

var proxy = new Proxy(target, handler);

try {
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test with invalid property key
var target = {};
var handler = {
	ownKeys: function(target) {
      return [5];
    }
};

var proxy = new Proxy(target, handler);

try {
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test with duplicated keys
var p = new Proxy({}, {
	ownKeys: function(target) {
	  return ["foo", "foo"];
	}
});

try {
  Object.keys(p);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test with lots of keys
var keyslist = [];

var handler = {
	ownKeys: function(target) {
      for (var idx = 0; idx < 30; idx++) {
        keyslist.push("K" + idx);
      }
      return keyslist;
    }
};

var proxy = new Proxy(target, handler);
assert(JSON.stringify(Object.getOwnPropertyNames(proxy)) === JSON.stringify(keyslist));

// test when invariants gets violated
var target = {
  "target_one": 1
};
Object.defineProperty(target, "nonconf", {value: 1, configurable: false});

var keys = ["foo"];

var handler = {
	ownKeys: function(target) {
      return keys;
    }
};

var proxy = new Proxy(target, handler);

try {
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

keys = ["nonconf"];
assert(JSON.stringify(Object.getOwnPropertyNames(proxy)) === '["nonconf"]');

Object.preventExtensions(target);
keys = ["foo", "nonconf"];

try {
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

keys = ["target_one", "nonconf"];

assert(JSON.stringify(Object.getOwnPropertyNames(proxy)) === '["target_one","nonconf"]');

keys = ["target_one", "nonconf", "foo"];

try {
  Object.getOwnPropertyNames(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var object = {};
Object.defineProperties(object, {
  a: { value: 42, enumerable: false },
  b: { value: "foo", enumerable: true },
  c: { value: "bar", enumerable: false }
});

var handler = {
  ownKeys(target) {
    return Reflect.ownKeys(target);
  }
};

var proxy = new Proxy(object, handler);

assert(Object.keys(proxy).length === 1);
assert(Object.keys(proxy)[0] === "b");