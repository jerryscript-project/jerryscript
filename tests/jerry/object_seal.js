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
// New properties may be added, existing properties may be changed or removed.
obj.foo = 'baz';
obj.lumpy = 'woof';
delete obj.prop;

var o = Object.seal(obj);

assert (o === obj);
assert (Object.isSealed (obj) === true);

// Changing property values on a sealed object still works.
obj.foo = 'quux';
assert (obj.foo === 'quux');
// But you can't convert data properties to accessors, or vice versa.
try {
    Object.defineProperty(obj, 'foo', { get: function() { return 42; } }); // throws a TypeError
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}

// Now any changes, other than to property values, will fail.
obj.quaxxor = 'the friendly duck'; // silently doesn't add the property
delete obj.foo; // silently doesn't delete the property

assert (obj.quaxxor === undefined);
assert (obj.foo === 'quux')

try {
    // Attempted additions through Object.defineProperty will also throw.
    Object.defineProperty (obj, 'ohai', { value: 17 }); // throws a TypeError
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}

try {
    Object.defineProperties (obj, { 'ohai' : { value: 17 } });
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}

Object.defineProperty (obj, 'foo', { value: 'eit' });
assert (obj.foo === 'eit')

// Objects aren't sealed by default.
var empty = {};
assert (Object.isSealed (empty) === false);

// If you make an empty object non-extensible, it is vacuously sealed.
Object.preventExtensions (empty);
assert (Object.isSealed (empty) === true);

// The same is not true of a non-empty object, unless its properties are all non-configurable.
var hasProp = { fee: 'fie foe fum' };
Object.preventExtensions (hasProp);
assert (Object.isSealed (hasProp) === false);

// But make them all non-configurable and the object becomes sealed.
Object.defineProperty (hasProp, 'fee', { configurable: false });
assert (Object.isSealed (hasProp) === true);

// The easiest way to seal an object, of course, is Object.seal.
var sealed = {};
Object.seal (sealed);
assert (Object.isSealed (sealed) === true);
