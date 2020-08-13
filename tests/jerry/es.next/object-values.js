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

// Test array
var arr = ["a", "b", "c"];
var values = Object.values(arr);

assert(values.indexOf("a") === 0);
assert(values.indexOf("b") === 1);
assert(values.indexOf("c") === 2);
assert(values.length === 3);

// Test object
var obj = {key1: "a", key3: "b", key2: "c", key4: "d", key5: "e"};
var values = Object.values(obj);

assert(values.indexOf("a") === 0);
assert(values.indexOf("b") === 1);
assert(values.indexOf("c") === 2);
assert(values.indexOf("d") === 3);
assert(values.indexOf("e") === 4);
assert(values.length === 5);

var obj2 = {};
Object.defineProperties(obj2, {
    key_one: {enumerable: true, value: "one"},
    key_two: {enumerable: false, value: "two"},
});

var values = Object.values(obj2);
// values should contain: key_one
assert(values.indexOf("one") === 0);
assert(values.indexOf("two") === -1);
assert(values.length === 1);

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

var values = Object.values(new Child());

assert(values.indexOf(5) === 0);
assert(values.indexOf(method) === 1);
assert(values.length === 2);

// Check enumerable properties
var o = {};

Object.defineProperty(o, "a", {
  value: "OK a",
  writable: true,
  enumerable: true,
  configurable: true
});

Object.defineProperty(o, "b", {
  value: "NOT_OK",
  writable: true,
  enumerable: false,
  configurable: true
});

Object.defineProperty(o, "c", {
  value: "OK c",
  writable: true,
  enumerable: true,
  configurable: true
});

var values = Object.values(o);
assert(values.length === 2);
assert(values[0] === "OK a");
assert(values[1] === "OK c");

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
var values = Object.values(proxy);

assert(values.length === 1);
assert(values[0] === "bar")
assert(handlers.length === 3);
assert(handlers.toString() === "D,D,G");

// exception during enumeration
var obj = {
  get a() { throw "error" },
  get b() { throw "shouldn't run" }
};

try {
  Object.values(obj);
} catch (err) {
  assert(err == "error")
}
