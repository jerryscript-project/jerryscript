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

var object = {};
Reflect.set (object, 'property', 'batcat');

assert (object.property === 'batcat');

var array = ['cat', 'cat', 'cat'];
Reflect.set (array, 2, 'bat');

assert (array[2] === 'bat');

assert (3 === Reflect.set.length);

try {
  Reflect.set ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.set (42, 'bat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.set (null, 'bat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var target = {};
var a = { [Symbol.toPrimitive]: function () { return 'bat' } };
var b = { [Symbol.toPrimitive]: function () { throw 'cat' } };
assert (Reflect.set (target, a, 42) === true);
assert (42 === target.bat);

try {
  Reflect.set (null, 'bat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var y = [];
Object.defineProperty (y, 0, {value: 42, configurable: false});
assert (Reflect.set (y, 'length', 0) === false);
assert (Reflect.set (y, 'length', 2) === true);
