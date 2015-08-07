// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

var mul = function(a, b) {
  return a * b;
};

var triple = mul.bind(null, 3);
delete mul;
assert (triple(20) === 60);
assert (triple.prototype === undefined);

var dupl = triple.bind({}, 2);
assert (dupl() === 6);
assert (dupl.prototype === undefined);

try {
  var obj = {};
  var new_func = obj.bind(null, 'foo');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var obj1 = {num : 36};

var f1 = function(a) {
  return this.num + a;
}

var add36 = f1.bind(obj1);
assert (add36(24) === 60);

var appendfoo = f1.bind(obj1, "foo");
assert (appendfoo() === "36foo");

var f2 = function(a) {
  return this.num + a.num;
}

var sum = f2.bind(obj1, obj1);
assert (sum() === 72);

function P(x, y) {
  this.x = x;
  this.y = y;
}

var P1 = P.bind({}, 2);
var _p1 = new P1();
assert (_p1.x === 2);
assert (_p1.y === undefined);
assert (_p1 instanceof P);
assert (_p1 instanceof P1);

var P2 = P1.bind(null);
var _p2 = new P2();
assert (_p2.x === 2);
assert (_p2.y === undefined);

_p2 = new P2(12, 60);
assert (_p2.x === 2);
assert (_p2.y === 12);

_p2 = new P2({}, 12);
assert (_p2.x === 2);
assert (Object.getPrototypeOf(_p2.y) === Object.prototype);
assert (_p2 instanceof P);
assert (_p2 instanceof P1);
assert (_p2 instanceof P2);

var P3 = P2.bind({}, 5);
var _p3 = new P3(8);
assert (_p3.x === 2);
assert (_p3.y === 5);
assert (_p3 instanceof P);
assert (_p3 instanceof P1);
assert (_p3 instanceof P2);
assert (_p3 instanceof P3);

var P4 = P.bind();
P4(4, 5);
assert (x === 4);
assert (y === 5);

var _x = x;
var _y = y;

var P5 = P.bind(undefined);
P5(5, 4);
assert (x === _y);
assert (y === _x);

var number = Number.constructor;
var bound = number.bind(null, 24);
var foo = new bound();
assert (foo() === undefined);

var number = Number;
var bound = number.bind(null, 3);
var foo = new bound();
assert (foo == 3);
assert (foo instanceof Number);
assert (foo.prototype === undefined);

var func = Number.prototype.toString.bind('foo');
assert (func instanceof Function);

try {
  var math = Math.sin;
  var bound = math.bind(null, 0);
  var foo = new bound();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var foo = function(x, y) { }

var bound = foo.bind(null);
assert(bound.length === 2);

bound = foo.bind(null, 9);
assert(bound.length === 1);

bound = foo.bind(null, 9, 8);
assert(bound.length === 0);

