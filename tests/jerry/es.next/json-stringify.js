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

// Test with proxy
assert(JSON.stringify(new Proxy(['foo'], {})) === '["foo"]');
assert(JSON.stringify(new Proxy({0:"foo"}, {})) === '{"0":"foo"}');

var target = [1,2,3];
var handler = {
  get(target, prop) {
    if (prop == "length")
    {
      throw 42;
    }
  }
}

try {
  JSON.stringify(new Proxy(target,handler));
  assert(false);
} catch (e) {
  assert(e === 42);
}

var revocable = Proxy.revocable (target, { get (t, p , r) {
  if (p == "toJSON") {
    revocable.revoke();
  }
}});
var proxy = revocable.proxy;

try {
  JSON.stringify(proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Checking quoting strings
assert(JSON.stringify("ab𬄕c") === '"ab𬄕\\u001fc"');
assert(JSON.stringify("ab\uDC01cd") === '"ab\\udc01c\\u001fd"');
assert(JSON.stringify("ab\uDC01cd\uD8331e") === '"ab\\udc01c\\u001fd\\ud8331e"');

// Test case where the proxy is already revoked
var handle = Proxy.revocable([], {});
handle.revoke();

try {
  JSON.stringify(handle.proxy);
  assert(false);
} catch (ex) {
  assert(ex instanceof TypeError);
}
