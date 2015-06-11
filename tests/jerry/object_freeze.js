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

var obj = {
  prop: function() {},
  foo: 'bar'
};

// New properties may be added, existing properties may be changed or removed
obj.foo = 'baz';
obj.lumpy = 'woof';
delete obj.prop;

var o = Object.freeze(obj);

assert(Object.isFrozen(obj) === true);

// Now any changes will fail
obj.foo = 'quux'; // silently does nothing
assert (obj.foo === 'baz');

obj.quaxxor = 'the friendly duck'; // silently doesn't add the property
assert (obj.quaxxor === undefined);

// ...and in strict mode such attempts will throw TypeErrors
function fail(){
  'use strict';

  try {
    obj.foo = 'sparky'; // throws a TypeError
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    delete obj.foo; // throws a TypeError
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    obj.sparky = 'arf'; // throws a TypeError
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
}

fail();

// Attempted changes through Object.defineProperty will also throw

try {
  Object.defineProperty(obj, 'ohai', { value: 17 }); // throws a TypeError
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Object.defineProperty(obj, 'foo', { value: 'eit' }); // throws a TypeError
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
