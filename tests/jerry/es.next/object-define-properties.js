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

var object = {};
var symbol = Symbol("symbol");

Object.defineProperties(object, {
  "foo": {
    value: true,
    writable: true
  },
  [symbol]: {
    value: "a symbol",
    configurable: true
  }
});

assert (object.foo === true);
assert (object[symbol] === "a symbol");

try {
  Object.defineProperties(undefined, {
    [symbol]: {
      value: "a symbol",
      writable: true
    }
  });
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// If one of the properties is wrong than it shouldn't update the object.
var obj2 = {
  a: 5
};
try {
  Object.defineProperties(obj2, {
    "foo": {
      value: true,
      writable: true
    },
    [symbol]: {
      value: "a symbol",
      set: 3
    }
  });
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
  assert (obj2.foo === undefined);
  assert (obj2[symbol] === undefined);
  assert (obj2.a === 5);
}

// Define accessors
var object = {};
Object.defineProperties(object, {
  "foo": {
    value: 42,
    writable: true,
  },
  [symbol]: {
    get: function() { return this.foo },
    set: function(v) { this.foo = v }
  }
});

assert (object[symbol] === 42);
object[symbol] = "baz";
assert (object[symbol] === "baz");

// Define get method which throws error
var object = {};
var props = {
  [symbol]: {
    value: 3,
    writable: true
  },
  get bar() {
    throw new TypeError("foo");
    return { value : 2, writable : true };
  },
};

try {
  Object.defineProperties(object, props);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
  assert (e.message === "foo");
}
// Define get method which deletes a property
var object = {};
Object.defineProperties(object, {
  "foo": {
    value: 42,
    writable: true,
  },
  [symbol]: {
    value: "a symbol",
    configurable: true
  },
  "bar": {
    get: function() {
      delete this[symbol];
      return this.foo;
    },
  }
});

assert (object[symbol] === "a symbol");
assert (object.bar === 42);
assert (object[symbol] === undefined);

var object = {};
var props = {
  [symbol]: {
    value: "a symbol",
    configurable: true
  },
  get bar() {
    delete props[symbol];
    delete props.prop1;
    return { value : 2, writable : true };
  },
  prop1: {
    value: 3,
    writable: true,
  },
};

Object.defineProperties(object, props);
var bar_desc = Object.getOwnPropertyDescriptor(object, 'bar');
assert(bar_desc.value === 2);
assert(bar_desc.writable === true);
assert(object.prop1 === undefined);
assert(object[symbol] === undefined);
