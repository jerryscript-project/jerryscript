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

assert(Array.isArray([]) === true);
assert(Array.isArray([1]) === true);
assert(Array.isArray(new Array()) === true);
assert(Array.isArray(new Array('a', 'b', 'c', 'd')) === true);
assert(Array.isArray(new Array(3)) === true);
assert(Array.isArray(Array.prototype) === true);
assert(Array.isArray(new Proxy([], {})) === true);

assert(Array.isArray() === false);
assert(Array.isArray({}) === false);
assert(Array.isArray(null) === false);
assert(Array.isArray(undefined) === false);
assert(Array.isArray(17) === false);
assert(Array.isArray('Array') === false);
assert(Array.isArray(true) === false);
assert(Array.isArray(false) === false);
assert(Array.isArray(new Uint8Array(32)) === false);
assert(Array.isArray({ __proto__: Array.prototype }) === false);

var revocable = Proxy.revocable ({}, {});
var proxy = revocable.proxy;
revocable.revoke();

try {
  Array.isArray(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var revocable = Proxy.revocable ([], {});
var proxy = revocable.proxy;

assert(Array.isArray(proxy) === true);
