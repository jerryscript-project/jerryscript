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

assert (Reflect.has ({x: 0}, 'x') === true);
assert (Reflect.has ({x: 0}, 'y') === false);

assert (Reflect.has ({x: 0}, 'toString') === true);

var object = {
  prop: 'Apple'
};

assert (Reflect.has (object, 'prop') === true);

assert (2 === Reflect.has.length);

try {
  Reflect.has ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.has (42, 'batcat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.has (null, 'bat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var target = {bat: 42};
var a = { [Symbol.toPrimitive]: function () { return 'bat' } };
var b = { [Symbol.toPrimitive]: function () { throw 'cat' } };

assert (Reflect.has (target, a) === true);

try {
  Reflect.has (target, b);
  assert (false);
} catch (e) {
  assert (e === 'cat');
}
