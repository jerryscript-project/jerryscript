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

// TODO: Update these tests when the internal routine has been implemented

var target = {};
var handler = { getOwnPropertyDescriptor (target) {
  throw 42;
}};

var proxy = new Proxy(target, handler);

try {
  // 19.1.3.2.5
  Object.prototype.hasOwnProperty.call(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 19.1.3.4
  Object.prototype.propertyIsEnumerable.call(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 19.1.2.1.5.b.iii
  Object.assign({}, proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 19.1.2.6.5
  Object.getOwnPropertyDescriptor(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* TODO: enable these test when Proxy.[[isExtensible]] has been implemted
try {
  // 19.1.2.5.2.9.a.i
  Object.freeze(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 19.1.2.12.2.7
  Object.isFrozen(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 19.1.2.13.2.7
  Object.isSealed(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
*/
