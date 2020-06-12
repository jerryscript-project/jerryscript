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

// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Some methods are taken from v8/test/mjsunit/mjsunit.js

function classOf(object) {
  // Argument must not be null or undefined.
  var string = Object.prototype.toString.call(object);
  // String has format [object <ClassName>].
  return string.substring(8, string.length - 1);
}

/**
 * Compares two objects for key/value equality.
 * Returns true if they are equal, false otherwise.
 */
function deepObjectEquals(a, b) {
  var aProps = Object.keys(a);
  aProps.sort();
  var bProps = Object.keys(b);
  bProps.sort();
  if (!deepEquals(aProps, bProps)) {
    return false;
  }
  for (var i = 0; i < aProps.length; i++) {
    if (!deepEquals(a[aProps[i]], b[aProps[i]])) {
      return false;
    }
  }
  return true;
}

var assertInstanceof = function assertInstanceof(obj, type) {
  if (!(obj instanceof type)) {
    var actualTypeName = null;
    var actualConstructor = Object.getPrototypeOf(obj).constructor;
    if (typeof actualConstructor == "function") {
      actualTypeName = actualConstructor.name || String(actualConstructor);
    } print("Object <" + obj + "> is not an instance of <" + (type.name || type) + ">" + (actualTypeName ? " but of < " + actualTypeName + ">" : ""));
  }
};

function deepEquals(a, b) {
  if (a === b) {
    if (a === 0) return (1 / a) === (1 / b);
    return true;
  }
  if (typeof a != typeof b) return false;
  if (typeof a == "number") return isNaN(a) && isNaN(b);
  if (typeof a !== "object" && typeof a !== "function") return false;
  var objectClass = classOf(a);
  if (objectClass !== classOf(b)) return false;
  if (objectClass === "RegExp") {
    return (a.toString() === b.toString());
  }
  if (objectClass === "Function") return false;
  if (objectClass === "Array") {
    var elementCount = 0;
    if (a.length != b.length) {
      return false;
    }
    for (var i = 0; i < a.length; i++) {
      if (!deepEquals(a[i], b[i]))
      return false;
    }
    return true;
  }
  if (objectClass == "String" || objectClass == "Number" || objectClass == "Boolean" || objectClass == "Date") {
    if (a.valueOf() !== b.valueOf()) return false;
  }
  return deepObjectEquals(a, b);
}

var assertEquals = function assertEquals(expected, found, name_opt) {
  if (!deepEquals(found, expected)) {
    assert(false);
  }
};

function assertArrayLikeEquals(value, expected, type) {
  assertInstanceof(value, type);
  assert(expected.length === value.length);
  for (var i=0; i<value.length; ++i) {
    assertEquals(expected[i], value[i]);
  }
}

assert(1 === Array.from.length);

// Assert that constructor is called with "length" for array-like objects

var myCollectionCalled = false;
function MyCollection(length) {
  myCollectionCalled = true;
  assert(1 === arguments.length);

  assert(5 === length);
}

Array.from.call(MyCollection, {length: 5});
assert(myCollectionCalled === true);
// Assert that calling mapfn with / without thisArg in sloppy and strict modes
// works as expected.

var global = this;
function non_strict(){ assert(global === this); }
function strict(){ "use strict"; assert(void 0 === this); }
function strict_null(){ "use strict"; assert(null === this); }
Array.from([1], non_strict);
Array.from([1], non_strict, void 0);
Array.from([1], strict);
Array.from([1], strict, void 0);

function testArrayFrom(thisArg, constructor) {
  assertArrayLikeEquals(Array.from.call(thisArg, [], undefined), [], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, NaN), [], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, Infinity), [], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, 10000000), [], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, 'test'), ['t', 'e', 's', 't'], constructor);

  assertArrayLikeEquals(Array.from.call(thisArg,
    { length: 1, '0': { 'foo': 'bar' } }), [{'foo': 'bar'}], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, { length: -1, '0': { 'foo': 'bar' } }), [], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg,
    [ 'foo', 'bar', 'baz' ]), ['foo', 'bar', 'baz'], constructor);
  var kSet = new Set(['foo', 'bar', 'baz']);
  assertArrayLikeEquals(Array.from.call(thisArg, kSet), ['foo', 'bar', 'baz'], constructor);
  var kMap = new Map(['foo', 'bar', 'baz'].entries());
  assertArrayLikeEquals(Array.from.call(thisArg, kMap), [[0, 'foo'], [1, 'bar'], [2, 'baz']], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, 'test', function(x) {
    return this.filter(x);
  }, {
    filter: function(x) { return x.toUpperCase(); }
  }), ['T', 'E', 'S', 'T'], constructor);
  assertArrayLikeEquals(Array.from.call(thisArg, 'test', function(x) {
    return x.toUpperCase();
  }), ['T', 'E', 'S', 'T'], constructor);

  try {
    Array.from.call(thisArg, null);
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Array.from.call(thisArg, undefined);
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Array.from.call(thisArg, [], null);
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Array.from.call(thisArg, [], "noncallable");
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  var nullIterator = {};
  nullIterator[Symbol.iterator] = null;
  assertArrayLikeEquals(Array.from.call(thisArg, nullIterator), [],
  constructor);

  var nonObjIterator = {};
  nonObjIterator[Symbol.iterator] = function() { return "nonObject"; };

  try {
    Array.from.call(thisArg, nonObjIterator);
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Array.from.call(thisArg, [], null);
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Ensure iterator is only accessed once, and only invoked once
  var called = false;
  var arr = [1, 2, 3];
  var obj = {};

  // Test order --- only get iterator method once
  function testIterator() {
    assert(called !== "@@iterator should be called only once");
    called = true;
    assert(obj === this);
    return arr[Symbol.iterator]();
  }
  var getCalled = false;
  Object.defineProperty(obj, Symbol.iterator, {
    get: function() {
      assert(getCalled !== "@@iterator should be accessed only once");
      getCalled = true;
      return testIterator;
    },
    set: function() {
      // "@@iterator should not be set"
      assert(false);
    }
  });
  assertArrayLikeEquals(Array.from.call(thisArg, obj), [1, 2, 3], constructor);
}

function Other() {}

var boundFn = (function() {}).bind(Array, 27);

testArrayFrom(Array, Array);
testArrayFrom(null, Array);
testArrayFrom({}, Array);
testArrayFrom(Object, Object);
testArrayFrom(Other, Other);
testArrayFrom(Math.cos, Array);
testArrayFrom(boundFn, boundFn);

// Assert that [[DefineOwnProperty]] is used in ArrayFrom, meaning a
// setter isn't called, and a failed [[DefineOwnProperty]] will throw.
var setterCalled = 0;
function exotic() {
  Object.defineProperty(this,  '0', {
    get: function() { return 2; },
    set: function() { setterCalled++; }
  });
}

// Non-configurable properties can't be overwritten with DefineOwnProperty
// The setter wasn't called
try {
  Array.from.call(exotic, [1]);
  assert(false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert( 0 === setterCalled);

// Non-callable iterators should cause a TypeError before calling the target
// constructor.
items = {};
items[Symbol.iterator] = 7;
function TestError() {}
function ArrayLike() { throw new TestError() }

try {
  Array.from.call(ArrayLike, items);
  assert(false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Check that array properties defined are writable, enumerable, configurable
function ordinary() { }
var x = Array.from.call(ordinary, [2]);
var xlength = Object.getOwnPropertyDescriptor(x, 'length');
assert(1 === xlength.value);
assert(true === xlength.writable);
assert(true === xlength.enumerable);
assert(true === xlength.configurable);
var x0 = Object.getOwnPropertyDescriptor(x, 0);
assert(2 === x0.value);
assert(true === xlength.writable);
assert(true === xlength.enumerable);
assert(true === xlength.configurable);

/* Test iterator close */
function __createIterableObject (arr, methods) {
  methods = methods || {};
  if (typeof Symbol !== 'function' || !Symbol.iterator) {
    return {};
  }
  arr.length++;
  var iterator = {
    next: function() {
      return { value: arr.shift(), done: arr.length <= 0 };
    },
    'return': methods['return'],
    'throw': methods['throw']
  };
  var iterable = {};
  iterable[Symbol.iterator] = function () { return iterator; };
  return iterable;
};

function close1() {
  var closed = false;
  var iter = __createIterableObject([1, 2, 3], {
      'return': function() { closed = true; return {}; }
  });

  try {
    Array.from(iter, x => { throw 5 });
    assert(false);
  } catch (e) {
    assert(e === 5);
  }

  return closed;
}
assert(close1());

function close2() {
  var closed = false;
  var iter = __createIterableObject([1, 2, 3], {
      'return': function() { closed = true; throw 6 }
  });

  try {
    Array.from(iter, x => { throw 5 });
    assert(false);
  } catch (e) {
    assert(e === 5);
  }

  return closed;
}
assert(close2());

