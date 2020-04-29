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

try {
  eval('var o = { __proto__ : 5, __proto__ : 5 }');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

var f = function(){};
assert((new f()).__proto__ === f.prototype);

var o = {};
o.__proto__ = Array.prototype;
assert(o instanceof Array);

var obj = Object.create(null)
p = {};
obj.__proto__ = p;
assert(Object.getPrototypeOf(obj) !== p);

var Circle = function () {};
var shape = {};
var circle = new Circle();

shape.__proto__ = circle;

assert(Object.getPrototypeOf(shape) === circle);
assert(shape.__proto__ === circle);

assert(Object.prototype.hasOwnProperty('__proto__') === true);

var desc = Object.getOwnPropertyDescriptor(Object.prototype,"__proto__");
assert((desc && "get" in desc && "set" in desc && desc.configurable && !desc.enumerable) === true);

assert((Object.getOwnPropertyNames(Object.prototype).indexOf('__proto__') > -1) === true);

try {
  shape.__proto__ = shape;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var o2 = { ["__proto__"] : null };
assert(o2.__proto__ === null);
assert(Object.getPrototypeOf(o2) === Object.prototype);

var o3 = { __proto__ : null };
assert(o3.__proto__ === undefined);
assert(Object.getPrototypeOf(o3) === null);

var o4 = { "__proto__" : null };
assert(o4.__proto__ === undefined);
assert(Object.getPrototypeOf(o4) === null);

var __proto__ = [];
var o5 = { __proto__ };
assert(o5.__proto__ === __proto__);
assert(Object.getPrototypeOf(o5) === Object.prototype);

var o6 = { __proto__() { return "42" } };
assert(o6.__proto__() === "42");
assert(Object.getPrototypeOf(o6) === Object.prototype);

var o7 = { __\u0070r\u006ft\u006f__: null };
assert(o7.__proto__ === undefined);
assert(Object.getPrototypeOf(o7) === null);

var o8 = { };
o8.__proto__ = Array.prototype;
assert(Object.getPrototypeOf(o8) === Array.prototype);

var str1 = '{"__proto__": [] }';
var obj1 = JSON.parse(str1);
assert(Object.getPrototypeOf(obj1) === Object.prototype);
assert(Array.isArray(obj1.__proto__));
