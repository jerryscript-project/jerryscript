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

var o = {};
o.__defineSetter__('foo', function(val) { this.bar = val; });
o.foo = 5;
assert(o.foo === undefined); // undefined
assert(o.bar === 5); 

var obj = {};
function bar() {}
Object.prototype.__defineSetter__.call(obj, "foo", bar);
var prop = Object.getOwnPropertyDescriptor(obj, "foo");
assert(prop.set === bar);
assert(!prop.writable);
assert(prop.configurable);
assert(prop.enumerable);

var obj = {};
var sym = Symbol();
function bar(baz) {}
Object.prototype.__defineSetter__.call(obj, sym, bar);
var prop = Object.getOwnPropertyDescriptor(obj, sym);
assert(prop.set === bar);
assert(!prop.writable);
assert(prop.configurable);
assert(prop.enumerable);

var key = '__accessors_test__';
Object.prototype.__defineSetter__.call(1, key, function(){});

try {
  Object.prototype.__defineSetter__.call(null, key, function(){});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var def = [];
var p = new Proxy({}, { defineProperty: function(o, v, desc) { def.push(v); Object.defineProperty(o, v, desc); return true; }});
Object.prototype.__defineSetter__.call(p, "foo", Object);
assert(def + '' === "foo");

var func = function() {};
var subject = Object.defineProperty(
  {}, 'attr', { value: 1, configurable: false }
);

try {
  subject.__defineSetter__('attr', func);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var subject = Object.preventExtensions({ existing: null });

subject.__defineSetter__('existing', func);

try {
  subject.__defineSetter__('brand new', func);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var __defineSetter__ = Object.prototype.__defineSetter__;
var counter = 0;
var key = {
  toString: function() {
    counter += 1;
  }
};

try {
  __defineSetter__.call(undefined, key, func);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  __defineSetter__.call(null, key, func);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

assert(counter === 0);

var subject = {};
var symbol = Symbol('');
var counter = 0;
var key = {
  toString: function() {
    counter += 1;
  }
};

try {
  subject.__defineSetter__(key, '');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  subject.__defineSetter__(key, 23);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  subject.__defineSetter__(key, true);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  subject.__defineSetter__(key, symbol);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  subject.__defineSetter__(key, {});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

assert(counter === 0);
