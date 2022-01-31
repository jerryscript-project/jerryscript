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

var obj = {
  get foo() { return "bar" },
  qux: 1,
  set bar(value) { this.x = value }
};

var foo = Object.prototype.__lookupGetter__.call(obj, "foo");
assert(foo() === "bar");
assert(Object.prototype.__lookupGetter__.call(obj, "qux") === undefined);
assert(Object.prototype.__lookupGetter__.call(obj, "baz") === undefined);
assert(Object.prototype.__lookupGetter__.call(obj, "bar") === undefined);

var foo = Object.prototype.__lookupGetter__.call(Object.create(obj), "foo");
assert(foo() === "bar");
assert(Object.prototype.__lookupGetter__.call(obj, "qux") === undefined);
assert(Object.prototype.__lookupGetter__.call(obj, "baz") === undefined);
assert(Object.prototype.__lookupGetter__.call(obj, "bar") === undefined);

var sym = Symbol();
var sym2 = Symbol();
var obj = {};
Object.defineProperty(obj, sym, { get: function() { return "bar"; }});
Object.defineProperty(obj, sym2, { value: 1 });
var foo = Object.prototype.__lookupGetter__.call(obj, sym);

assert(foo() === "bar");
assert(Object.prototype.__lookupGetter__.call(obj, sym2) === undefined);
assert(Object.prototype.__lookupGetter__.call(obj, Symbol()) === undefined);

Object.prototype.__lookupGetter__.call(1, 'key');

try {
  Object.prototype.__lookupGetter__.call(null, 'key');
  assert(false);
} catch(e){
  assert(e instanceof TypeError);
}

var a = {};
var b = Object.create(a);
b.foo = 1;
Object.defineProperty(a, "foo", {function () {}})
assert(b.__lookupGetter__("foo") === undefined);

var gopd = [];
var gpo = false;
var p = new Proxy({}, {
  getPrototypeOf: function(o) { gpo = true; return Object.getPrototypeOf(o); },
  getOwnPropertyDescriptor: function(o, v) { gopd.push(v); return Object.getOwnPropertyDescriptor(o, v); }
});

Object.prototype.__lookupGetter__.call(p, "foo");
assert(gopd + '' === "foo");
assert(gpo === true);

var __lookupGetter__ = Object.prototype.__lookupGetter__;
var counter = 0;
var key = {
  toString: function() {
    counter += 1;
  }
};

try {
  __lookupGetter__.call(undefined, key);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  __lookupGetter__.call(null, key);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

assert(counter === 0);
