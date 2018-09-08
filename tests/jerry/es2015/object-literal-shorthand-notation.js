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

var x = 1, y = 2, z = 3;
var obj = { x, y, z: { z } };
assert(obj.x === 1);
assert(obj.y === 2);
assert(obj.z.z === 3);

try {
  var a = { get, set };
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}

function checkReservedAsVars() {
  var get = 1, set = 2;
  var a = { get, set };
  assert(a.get + a.set === 3);
}
checkReservedAsVars();

var one = 1;
assert({ one, one }.one === 1);
assert({ one: 0, one }.one === 1);
assert({ one, one: 0 }.one === 0);

var obj2 = { one };
assert({ obj2 }.obj2.one === 1);

try {
  eval('({ true, false, null })');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval('({ 1 })');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval('({ "foo" })');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval('({ {} })');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval('({ 1, "foo", {} })');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval('({ static foo() {} })');
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

var obj3 = { f() { return 1; } };
assert(obj3.f() === 1);

var obj4 = { one, one() { return one; } };
assert(typeof obj4.one === 'function');
assert(obj4.one() === 1);

var obj5 = { x: 123, getX() { return this.x; } };
assert(obj5.getX() === 123);

var obj6 = {
  if() { return 1; }, else() { return 1; }, try() { return 1; }, catch() { return 1; },
  finally() { return 1; }, let() { return 1; }, true() { return 1; }, false() { return 1; },
  null() { return 1; }, function() { return 1; }
};
assert(
  obj6.if() + obj6.else() + obj6.try() + obj6.catch() + obj6.finally() + obj6.let() +
  obj6.true() + obj6.false() + obj6.null() + obj6.function() === 10
);

var obj7 = {
  _x: 0,

  get x() {
    return thix._x;
  },

  set x(x_value) {
    this._x = x_value;
  }
};
assert(obj7._x === 7);
assert(obj7.x === 7);
obj7.x = 1;
assert(obj7._x === 1);
assert(obj7.x === 1);

var obj8 = {
  constructor() { return 1; }, static() { return 1; }, get() { return 1; }, set() { return 1; }
};
assert(obj8.constructor() + obj8.static() + obj8.get() + obj8.set() === 4);
