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

var target = function () {};
var handler = { get (name) {
  return 5;
}};

var revocable = Proxy.revocable(target, handler);

var proxy = revocable.proxy;

assert(proxy.a === 5);

revocable.revoke();

try {
  proxy.a;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Proxy.revocable();
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Proxy.revocable(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var p1 = Proxy.revocable({}, proxy);
p1.a = 3;
assert(p1.a == 3);

var p2 = Proxy.revocable(proxy, {});
p2.b = 43;
assert(p2.b == 43);

assert(typeof(target.a) === "undefined");
assert(typeof(target.b) === "undefined");

// Try accessing the "a" property again, it should fail
try {
  proxy.a;
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
