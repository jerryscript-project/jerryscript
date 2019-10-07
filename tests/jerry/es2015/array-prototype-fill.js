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

function assertArrayEquals (array1, array2) {
  if (array1.length !== array2.length) {
    return false;
  }

  for (var i = 0; i < array1.length; i++) {
    if (array1[i] !== array2[i]) {
      return false;
    }
  }

  return true;
}

assert (1 === Array.prototype.fill.length);

assert (assertArrayEquals ([].fill (8), []));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (), [undefined, undefined, undefined, undefined, undefined]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8), [8, 8, 8, 8, 8]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, 1), [0, 8, 8, 8, 8]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, 10), [0, 0, 0, 0, 0]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, -5), [8, 8, 8, 8, 8]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, 1, 4), [0, 8, 8, 8, 0]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, 1, -1), [0, 8, 8, 8, 0]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, 1, 42), [0, 8, 8, 8, 8]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, -3, 42), [0, 0, 8, 8, 8]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, -3, 4), [0, 0, 8, 8, 0]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, -2, -1), [0, 0, 0, 8, 0]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, -1, -3), [0, 0, 0, 0, 0]));
assert (assertArrayEquals ([0, 0, 0, 0, 0].fill (8, undefined, 4), [8, 8, 8, 8, 0]));
assert (assertArrayEquals ([ ,  ,  ,  , 0].fill (8, 1, 3), [, 8, 8, , 0]));


// If the range is empty, the array is not actually modified and
// should not throw, even when applied to a frozen object.
assert (assertArrayEquals (Object.freeze ([1, 2, 3]).fill (0, 0, 0), [1, 2, 3]));

// Test exceptions
try {
  Object.freeze ([0]).fill ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
try {
  Array.prototype.fill.call (null)
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
try {
  Array.prototype.fill.call (undefined)
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

function TestFillObjectWithAccessors () {
  var kLength = 5;

  var log = [];

  var object = {
    length: kLength,
    get 1 () {
      log.push ("get 1");
      return this.foo;
    },

    set 1 (val) {
      log.push ("set 1 " + val);
      this.foo = val;
    }
  };

  Array.prototype.fill.call (object, 42);

  assert (kLength === object.length);
  assert (assertArrayEquals (["set 1 42"], log));

  for  (var i = 0; i < kLength; ++i) {
    assert (42 === object[i]);
  }
}
TestFillObjectWithAccessors ();

function TestFillObjectWithMaxNumberLength () {
  var kMaxSafeInt = Math.pow (2, 32) - 1;
  var object = {};
  object.length = kMaxSafeInt;

  Array.prototype.fill.call (object, 42, Math.pow (2, 32) - 4);

  assert (kMaxSafeInt === object.length);
  assert (42 === object[kMaxSafeInt - 3]);
  assert (42 === object[kMaxSafeInt - 2]);
  assert (42 === object[kMaxSafeInt - 1]);
}
TestFillObjectWithMaxNumberLength ();

function TestFillObjectWithPrototypeAccessors () {
  var kLength = 5;
  var log = [];
  var proto = {
    get 1 () {
      log.push ("get 0");
      return this.foo;
    },

    set 1 (val) {
      log.push ("set 1 " + val);
      this.foo = val;
    }
  };

  var object = { 0:0, 2:2, length: kLength};
  Object.setPrototypeOf (object, proto);

  Array.prototype.fill.call (object, "42");

  assert (kLength === object.length);
  assert (assertArrayEquals (["set 1 42"], log));
  assert (object.hasOwnProperty (0) == true);
  assert (object.hasOwnProperty (1) == false);
  assert (object.hasOwnProperty (2) == true);
  assert (object.hasOwnProperty (3) == true);
  assert (object.hasOwnProperty (4) == true);

  for (var i = 0; i < kLength; ++i) {
    assert ("42" === object[i]);
  }
}
TestFillObjectWithPrototypeAccessors ();

function TestFillSealedObject () {
  var object = { length: 42 };
  Object.seal (object);

  try {
    Array.prototype.fill.call (object);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
}
TestFillSealedObject ();

function TestFillFrozenObject () {
  var object = { length: 42 };
  Object.freeze (object);

  try {
    Array.prototype.fill.call (object);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
}
TestFillFrozenObject ();
