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

function testLogicalOr(truish, falsish, value) {
  let a = truish;
  a ||= value;
  expectSame(a, truish);

  a = falsish;
  a ||= value;
  expectSame(a, value);

  a = {
    a: truish
  };
  a.a ||= value
  expectSame(a.a, truish);

  a = {
    a: falsish
  };
  a.a ||= value;
  expectSame(a.a, value);

  let called = false;
  let setter = function (_) {
    called = true
  };
  a = Object.defineProperty({}, "a", {
    set: setter
  });
  let obj = {
    a: value
  }
  a ||= obj;
  expectSame(called, false);
  expectSame(Object.getOwnPropertyDescriptor(a, "a").set, setter);

  a = falsish;
  a ||= function () {};
  expectSame(typeof a, 'function');
  expectSame(a.name, 'a');

  a = falsish;
  a ||= class {};
  expectSame(typeof a, 'function');
  expectSame(a.name, 'a');

  const b = truish;
  b ||= value;
  expectSame(b, truish);

  expectSame(this.b, undefined);
  this.b ||= value;
  expectSame(this.b, value);
}

function testLogicalAnd(truish, falsish, value) {
  let a = falsish;
  a &&= value;
  expectSame(a, falsish);

  a = truish;
  a &&= value;
  expectSame(a, value);

  a = {
    a: falsish
  };
  a.a &&= value
  expectSame(a.a, falsish);

  a = {
    a: truish
  };
  a.a &&= value;
  expectSame(a.a, value);

  let called = false;
  let setter = function (_) {
    called = true
  };
  a = Object.defineProperty({}, "a", {
    set: setter
  });
  let obj = {
    a: value
  };
  a &&= obj;
  expectSame(called, false);
  expectSame(Object.getOwnPropertyDescriptor(a, "a").value, obj.a);

  a = truish;
  a &&= function () {};
  expectSame(typeof a, 'function');
  expectSame(a.name, 'a');

  a = truish;
  a &&= class {};
  expectSame(typeof a, 'function');
  expectSame(a.name, 'a');

  const b = falsish;
  b &&= value;
  expectSame(b, falsish);

  expectSame(this.b, undefined);
  this.b &&= value;
  expectSame(this.b, undefined);
}

function testNullish(truish, falsish, value) {
  let a = falsish;
  a ??= value;
  expectSame(a, value);

  a = truish;
  a ??= value;
  expectSame(a, truish);

  a = {
    a: falsish
  };
  a.a ??= value
  expectSame(a.a, value);

  a = {
    a: truish
  };
  a.a ??= value;
  expectSame(a.a, truish);

  let called = false;
  let setter = function (_) {
    called = true
  };
  a = Object.defineProperty({}, "a", {
    set: setter
  });
  let obj = {
    a: value
  };
  a ??= obj;
  expectSame(called, false);
  expectSame(Object.getOwnPropertyDescriptor(a, "a").set, setter);

  a = falsish;
  a ??= function () {};
  expectSame(typeof a, 'function');
  expectSame(a.name, 'a');

  a = falsish;
  a ??= class {};
  expectSame(typeof a, 'function');
  expectSame(a.name, 'a');

  const b = truish;
  b ??= value;
  expectSame(b, truish);

  expectSame(this.b, undefined);
  this.b ??= value;
  expectSame(this.b, value);
}

const nullishValues = [undefined, null];
const logicalTrueValues = [1.1, Infinity, 1n, true, "foo", {}, Symbol()];
const logicalFalseValues = [0, 0.0, NaN, 0n, false, "", ...nullishValues];
const allValues = [...logicalTrueValues, ...logicalFalseValues];

function testValues(cb, falseValues) {
  for (const truish of logicalTrueValues) {
    for (const falsish of falseValues) {
      for (const value of allValues) {
        cb.call({}, truish, falsish, value);
      }
    }
  }
}

function expectSame(x, value) {
  if (typeof x === 'number' && isNaN(x)) {
    assert(typeof value === 'number');
    assert(isNaN(value));
  } else {
    assert(x === value);
  }
}

testValues(testLogicalOr, logicalFalseValues);
testValues(testLogicalAnd, logicalFalseValues);
testValues(testNullish, nullishValues);

function expectTypeError(str) {
  try {
    eval(str);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
}

expectTypeError("const c = 0; c ||= 8");
expectTypeError("const c = 1; c &&= 8");
expectTypeError("const c = undefined; c ??= 8");
