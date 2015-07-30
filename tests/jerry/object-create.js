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

// Example where we create an object with a couple of sample properties.
// (Note that the second parameter maps keys to *property descriptors*.)
var o = Object.create(Object.prototype, {
  // foo is a regular 'value property'
  foo: { writable: true, configurable: true, value: 'hello' },
  // bar is a getter-and-setter (accessor) property
  bar: {
    configurable: false,
    get: function() { return 10; },
    set: function(value) { console.log('Setting `o.bar` to', value); }
  }
});

// create a new object whose prototype is a new, empty object
// and a adding single property 'p', with value 42
var o = Object.create({}, { p: { value: 42 } });
// by default properties ARE NOT writable, enumerable or configurable:
o.p = 24;
assert (o.p === 42);

// to specify an ES3 property
var o2 = Object.create({}, {
  p: {
    value: 42,
    writable: true,
    enumerable: true,
    configurable: true
  }
});

assert (o2.p === 42);

// Shape - superclass
function Shape() {
  this.x = 0;
  this.y = 0;
}

// superclass method
Shape.prototype.move = function(x, y) {
  this.x += x;
  this.y += y;
};

// Rectangle - subclass
function Rectangle() {
  Shape.call(this); // call super constructor.
}

// subclass extends superclass
Rectangle.prototype = Object.create(Shape.prototype);
Rectangle.prototype.constructor = Rectangle;

var rect = new Rectangle();

assert (rect instanceof Rectangle);
assert (rect instanceof Shape);
rect.move(1, 1);
assert (rect.x === 1)
assert (rect.y === 1);

var obj = {
  protoFunction: function() {
    return 3;
  }
};

Object.defineProperties(obj, {
  "foo": {
    value: 42,
    writable: true,
  },
  "a": {
    value: "b",
    configurable: true
  },
  "bar": {
    get: function() {
      return this.foo;
    },
  },
});

var obj2 = Object.create(obj);

assert (obj2.protoFunction() === 3);
assert (obj2.foo === 42);
assert (obj2.a === "b");
assert (obj2.bar === 42);
assert (Object.getPrototypeOf (obj2) === obj);


var props = {
    prop1: {
        value: 1,
    },
    hey: function () {
        return "ho";
    }
};

var obj3 = Object.create(obj, props);
assert (obj3.prop1 === 1);
assert (obj3.protoFunction() === 3);
try {
  assert (obj3.hey === undefined);
  obj3.hey();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Create an object with null as prototype
var obj = Object.create(null)
assert (typeof (obj) === "object");
assert (Object.getPrototypeOf (obj) === null);

try {
    Object.create()
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}

try {
    Object.create(undefined)
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}
