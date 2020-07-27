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

/* Single non-enumerable property leak check with Object.assign, */
var from = {};
Object.defineProperty (from, "abc", { enumerable: false, get: function() { return new String ("demo"); }})
assert ((from.abc + "") === "demo");

var out = Object.assign ({}, from);
assert (typeof (out.abc) === "undefined");

/* Test with Proxy */
var called_get = false;
var called_keys = false;
var called_desc = false;
var called_extra_get = false;

var prox = new Proxy (from, {
  get: function (target, key) {
    assert (key === "ERR");
    called_get = true;
    throw new URIError("ERR");
  },
  ownKeys: function (target) {
    called_keys = true;
    return ["abc", "ERR"];
  },
  getOwnPropertyDescriptor: function(target, key) {
    if (key === "ERR") {
      called_desc = true;
      return { enumerable: true,
               get: function() {
                 /* This should never be called! */
                 called_extra_get = true;
                 return "ABC";
               },
               configurable: true,
      };
    }
    return Reflect.getOwnPropertyDescriptor(target, key);
  },
});

try {
  var prox_out = Object.assign ({}, prox);
  assert (false);
} catch (ex) {
  assert (ex instanceof URIError);
}

assert (called_keys === true);
assert (called_desc === true);
assert (called_get === true);

assert (called_extra_get === false);

/* Original test case from the issue report: test if there is a leak on exit. */
var result = Object.assign({}, RegExp);
