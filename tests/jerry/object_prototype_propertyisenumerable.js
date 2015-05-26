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

var obj = {};

// Test if the toString fails.
try {
  obj.propertyIsEnumerable({ toString: function() { throw new ReferenceError ("foo"); } });

  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// Test if the toObject fails.
var obj1;
try {
  obj1.propertyIsEnumerable("fail");

  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var array = [];
obj.prop = "bar";
array[0] = "bar";

assert (obj.propertyIsEnumerable('prop') === true);
assert (array.propertyIsEnumerable(0) === true);

assert (obj.propertyIsEnumerable('length') === false);
assert (array.propertyIsEnumerable('length') === false);
assert (Math.propertyIsEnumerable('random') === false);

// Creating a new property
Object.defineProperty(obj, 'prop1', { value: 'foo', enumerable: true });
Object.defineProperty(obj, 'prop2', { value: 'foo', enumerable: false });
Object.defineProperty(obj, 'prop3', { value: 'foo' });
assert (obj.propertyIsEnumerable('prop1') === true);
assert (obj.propertyIsEnumerable('prop2') === false);
assert (obj.propertyIsEnumerable('prop3') === false);

Object.defineProperty(array, 'prop1', { value: 'foo', enumerable: true });
Object.defineProperty(array, 'prop2', { value: 'foo', enumerable: false });
Object.defineProperty(array, 'prop3', { value: 'foo' });
assert (array.propertyIsEnumerable('prop1') === true);
assert (array.propertyIsEnumerable('prop2') === false);
assert (array.propertyIsEnumerable('prop3') === false);

// Modify an existing one
Object.defineProperty(obj, 'prop', { value: 'foo', enumerable: false });
assert (obj.propertyIsEnumerable('prop') === false);
Object.defineProperty(obj, 'prop', { value: 'foo', enumerable: true });
assert (obj.propertyIsEnumerable('prop') === true);

Object.defineProperty(array, 0, { value: 'foo', enumerable: false });
assert (array.propertyIsEnumerable(0) === false);
Object.defineProperty(array, 0, { value: 'foo', enumerable: true });
assert (array.propertyIsEnumerable(0) === true);

// Enumerability of inherited properties
function construct1()
{
  this.prop1 = 'foo';
}

function construct2()
{
  this.prop2 = 'foo';
}

construct2.prototype = new construct1;
construct2.prototype.constructor = construct2;

var obj2 = new construct2();
obj2.prop3 = 'foo';

assert (obj2.propertyIsEnumerable('prop3') === true);
assert (obj2.propertyIsEnumerable('prop2') === true);
assert (obj2.propertyIsEnumerable('prop1') === false);

obj2.prop1 = 'foo';

assert (obj2.propertyIsEnumerable('prop1') === true);
