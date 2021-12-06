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

var object = {};
var symbol = Symbol("symbol");

Object.defineProperties(object, {
  a: {
    value: 42,
    enumerable: true
  },
  b: {
    value: "foo",
    enumerable: false
  },
  [symbol]: {
    value: "symbol",
    enumerable: true
  }
});

// Object.keys and Object.entries should have the same keys
var keys = Object.keys(object);
var entries = Object.entries(object);

assert(keys.length === entries.length);
for (let i = 0; i < keys.length; i++) {
  assert(keys[i] === entries[i][0]);
}

// Test object entries
var entries = Object.entries(object);
assert(entries instanceof Array);
assert(entries.length === 1);
assert(entries[0].length === 2);
assert(entries[0][0] === "a");
assert(entries[0][1] === 42);

// Test array entries
var array = [1, 2, "three"];
var entries = Object.entries(array);

assert(entries instanceof Array);
assert(entries.length === array.length);

for (let i = 0; i < entries.length; i++) {
  assert(entries[i][0] === i + "");
  assert(entries[i][1] === array[i]);
}

// Test prototype chain
function Parent() {}
Parent.prototype.inheritedMethod = function() {};

function method() {};
function Child() {
  this.prop = 5;
  this.method = method;
}

Child.prototype = new Parent;
Child.prototype.prototypeMethod = function() {};

var entries = Object.entries (new Child());
assert(entries.length === 2);
assert(entries[0][0] === "prop");
assert(entries[0][1] === 5);
assert(entries[1][0] === "method");
assert(entries[1][1] === method);

// Test with primitive values
var entries = Object.entries(true);
assert(entries instanceof Array);
assert(entries.length === 0);

try {
  Object.entries(undefined);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError)
}

try {
  Object.entries(null);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError)
}

// Test proxy object
var object = {};

Object.defineProperties(object, {
  a: {
    value: "foo",
    enumerable: false
  },
  b: {
    value: "bar",
    enumerable: true,
    writable: false
  }
});

var proxy = new Proxy(object, {
  getOwnPropertyDescriptor: function(o, v) {
    handlers.push("D");
    return Object.getOwnPropertyDescriptor(o, v);
  },
  get: function(o, v) {
    handlers.push("G");
    return o[v];
  }
});

var handlers = [];
var entries = Object.entries(proxy);

assert(entries.length === 1);
assert(entries[0][0] === "b");
assert(entries[0][1] === "bar");
assert(handlers.length === 3);
assert(handlers.toString() === "D,D,G");

// exception during enumeration
var obj = {
  get a() { throw "error" },
  get b() { throw "shouldn't run" }
};

try {
  Object.entries(obj);
} catch (err) {
  assert(err == "error")
}
