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

var target = {}
var handler = {};
var proxy = new Proxy(target, handler);

var revocable = Proxy.revocable(target, handler);
revocable.revoke();
var rev_proxy = revocable.proxy;

try {
  Proxy(target, handler);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  new Proxy(undefined, undefined);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  new Proxy(rev_proxy);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

/* In ES11+ the standard changed revoked proxy is a valid input for a new Proxy */
var proxy_rev_handler = new Proxy({}, rev_proxy);
var proxy_rev_target_Br = new Proxy(rev_proxy, {});
