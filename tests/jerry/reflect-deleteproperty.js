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

var object = {
  property: 'Batcat'
};

Reflect.deleteProperty (object, 'property');

assert (object.property === undefined);

assert (2 === Reflect.deleteProperty.length);

try {
  Reflect.deleteProperty ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.deleteProperty (42, 'bat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.deleteProperty (null, 'bat');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var target = {bat: 42};
var a = { [Symbol.toPrimitive]: function() { return 'bat' } };
var b = { [Symbol.toPrimitive]: function() { throw 'cat' } };

assert (Reflect.deleteProperty (target, a));

try {
  Reflect.deleteProperty (target, b);
  assert (false);
} catch (e) {
  assert (e === 'cat');
}
