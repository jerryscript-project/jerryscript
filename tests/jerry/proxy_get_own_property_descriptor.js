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

// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var target = {};
var handler = { getOwnPropertyDescriptor (target) {
  throw "cat";
}};

var proxy = new Proxy(target, handler);

try {
  // 19.1.3.2.5
  Object.prototype.hasOwnProperty.call(proxy);
  assert(false);
} catch(e) {
  assert(e == "cat");
}

try {
  // 19.1.3.4
  Object.prototype.propertyIsEnumerable.call(proxy)
  assert(false);
} catch(e) {
  assert(e == "cat");
}

try {
  // 19.1.2.6.5
  Object.getOwnPropertyDescriptor(proxy)
  assert(false);
} catch(e) {
  assert(e == "cat");
}

var target = {};
var configurable_desc = {
  value: 123,
  configurable: true,
  writable: true,
  enumerable: false,
};
Object.defineProperty(target, "configurable", configurable_desc);

var nonconfigurable_desc = {
  value: 234,
  configurable: false,
  writable: false,
  enumerable: true
}
Object.defineProperty(target, "nonconfigurable", nonconfigurable_desc);

var proxied_desc = {
  value: 345,
  configurable: true
};

var proxied_desc = {
  value: 345,
  configurable: true
};

var handler = {
  "getOwnPropertyDescriptor": function(target, name) {
    if (name === "proxied") {
      return proxied_desc;
    }
    if (name === "return_null") {
      return null;
    }
    return Object.getOwnPropertyDescriptor(target, name);
  }
};

var proxy = new Proxy(target, handler);
var proxy_without_handler = new Proxy(target, {});

// Checking basic functionality:

var configurable_obj = Object.getOwnPropertyDescriptor(proxy, "configurable");

assert(configurable_desc.value == configurable_obj.value);
assert(configurable_desc.configurable == configurable_obj.configurable);
assert(configurable_desc.writable == configurable_obj.writable);
assert(configurable_desc.enumerable == configurable_obj.enumerable);

var nonconfigurable_obj = Object.getOwnPropertyDescriptor(proxy, "nonconfigurable");
assert(nonconfigurable_obj.value == nonconfigurable_desc.value);
assert(nonconfigurable_obj.configurable == nonconfigurable_desc.configurable);
assert(nonconfigurable_obj.writable == nonconfigurable_desc.writable);
assert(nonconfigurable_obj.enumerable == nonconfigurable_desc.enumerable);

var other_obj = { value: proxied_desc.value,
                  configurable: proxied_desc.configurable,
                  enumerable: false,
                  writable: false }
var proxied_obj = Object.getOwnPropertyDescriptor(proxy, "proxied");

assert(other_obj.value == proxied_obj.value);
assert(other_obj.configurable == proxied_obj.configurable);
assert(other_obj.writable == proxied_obj.writable);
assert(other_obj.enumerable == proxied_obj.enumerable);

var other_obj2 = Object.getOwnPropertyDescriptor(proxy_without_handler, "configurable");

assert(other_obj2.value == configurable_desc.value);
assert(other_obj2.configurable == configurable_desc.configurable);
assert(other_obj2.writable == configurable_desc.writable);
assert(other_obj2.enumerable == configurable_desc.enumerable);

var other_obj3 = Object.getOwnPropertyDescriptor(proxy_without_handler, "nonconfigurable");

assert(other_obj3.value == nonconfigurable_desc.value);
assert(other_obj3.configurable == nonconfigurable_desc.configurable);
assert(other_obj3.writable == nonconfigurable_desc.writable);
assert(other_obj3.enumerable == nonconfigurable_desc.enumerable);

try {
  Object.getOwnPropertyDescriptor(proxy, "return_null");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// Checking invariants mentioned explicitly by the ES spec:

// (Inv-1) "A property cannot be reported as non-existent, if it exists as a
// non-configurable own property of the target object."
handler.getOwnPropertyDescriptor = function(target, name) { return undefined; };

try {
  Object.getOwnPropertyDescriptor(proxy, "nonconfigurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

assert(Object.getOwnPropertyDescriptor(proxy, "configurable") == undefined)

// (Inv-2) "A property cannot be reported as non-configurable, if it does not
// exist as an own property of the target object or if it exists as a
// configurable own property of the target object."
handler.getOwnPropertyDescriptor = function(target, name) {
  return {value: 234, configurable: false, enumerable: true};
};

try {
  Object.getOwnPropertyDescriptor(proxy, "nonexistent");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  Object.getOwnPropertyDescriptor(proxy, "configurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

assert(!Object.getOwnPropertyDescriptor(proxy, "nonconfigurable").configurable);

// (Inv-3) "A property cannot be reported as non-existent, if it exists as an
// own property of the target object and the target object is not extensible."
Object.seal(target);
handler.getOwnPropertyDescriptor = function(target, name) { return undefined; };

try {
  Object.getOwnPropertyDescriptor(proxy, "configurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  Object.getOwnPropertyDescriptor(proxy, "nonconfigurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

assert(undefined == Object.getOwnPropertyDescriptor(proxy, "nonexistent"));

// (Inv-4) "A property cannot be reported as existent, if it does not exist as
// an own property of the target object and the target object is not
// extensible."
var existent_desc = {value: "yes"};
handler.getOwnPropertyDescriptor = function() { return existent_desc; };

try {
  Object.getOwnPropertyDescriptor(proxy, "nonexistent");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

var new_obj =  {  value: "yes",
                  writable: false,
                  enumerable: false,
                  configurable: false };
var conf_proxied = Object.getOwnPropertyDescriptor(proxy, "configurable");

assert(new_obj.value == conf_proxied.value);
assert(new_obj.configurable == conf_proxied.configurable);
assert(new_obj.writable == conf_proxied.writable);
assert(new_obj.enumerable == conf_proxied.enumerable);

// Checking individual bailout points in the implementation:

// Step 6: Trap is not callable.
handler.getOwnPropertyDescriptor = {};

try {
  Object.getOwnPropertyDescriptor(proxy, "configurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// Step 8: Trap throws.
handler.getOwnPropertyDescriptor = function() { throw "unicorn"; };

try {
  Object.getOwnPropertyDescriptor(proxy, "configurable");
  assert(false);
} catch(e) {
  assert(e == "unicorn");
}

// Step 9: Trap result is neither undefined nor an object.
handler.getOwnPropertyDescriptor = function() { return 1; }

try {
  Object.getOwnPropertyDescriptor(proxy, "configurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// Step 11b: See (Inv-1) above.
// Step 11e: See (Inv-3) above.

// Step 16: Incompatible PropertyDescriptor; a non-configurable property
// cannot be reported as configurable. (Inv-4) above checks more cases.
handler.getOwnPropertyDescriptor = function(target, name) {
  return {value: 456, configurable: true, writable: true}
};

try {
  Object.getOwnPropertyDescriptor(proxy, "nonconfigurable");
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// Step 17: See (Inv-2) above.

var result = [];
var proxy = new Proxy({foo: 1, bar: 2}, {
  getOwnPropertyDescriptor: function(o, v) {
    result.push(v);
    return Object.getOwnPropertyDescriptor(o, v);
  }
});

Object.assign({}, proxy);

assert(result.length === 2);
assert(result[0] === "foo");
assert(result[1] === "bar");


var trapCalls = 0;
var p = new Proxy({}, {
  getOwnPropertyDescriptor: function(t, prop) {
    Object.defineProperty(t, prop, {
      configurable: false,
      writable: true,
    });

    trapCalls++;
    return {
      configurable: false,
      writable: false,
    };
  },
});

try
{
  Object.getOwnPropertyDescriptor(p, "prop");
  assert (false)
}
catch (e)
{
  assert(e instanceof TypeError)
}
