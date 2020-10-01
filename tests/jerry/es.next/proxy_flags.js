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

var target_called = 0;
function target_method() {
  target_called++;
}

var proxy_revocable_function = Proxy.revocable(target_method, {})
var proxy_function = proxy_revocable_function.proxy;

/* Test Proxy IsCallable(target) */
/* The proxy target should have a function type. */
assert(typeof(proxy_function) === "function");

/* The proxy can be called with new */
var new_obj = new proxy_function()
assert(new_obj instanceof target_method);
assert(target_called === 1);

/* Test Proxy IsConstructor(target) */
/* Array.from tries to use the "this" value as constructor. */
var array_result = Array.from.call(proxy_function, [1, 2, 3]);
assert(Array.isArray(array_result) === false);
assert(target_called === 2);

proxy_revocable_function.revoke();

/* Test Proxy IsCallable(target) if the proxy is revoked. */
/* After the proxy was revoked the type is still function. */
assert(typeof(proxy_function) === "function");

/* After the proxy was revoked the constructor should not be called
 * and an error should be reported
 */
try {
  new proxy_function();
  assert(false);
} catch (ex) {
  assert(ex instanceof TypeError);
}

assert(target_called === 2);

/* Test Proxy IsConstructor(target) if the proxy is revoked. */
/* Array.from tries to use the "this" value as constructor and as the
 * proxy is revoked the constructor call should fail.
 * The IsConstructor(proxy_function) is still true.
 */
try {
  Array.from.call(proxy_function, [1, 2, 3]);
  assert(false);
} catch (ex) {
  assert(ex instanceof TypeError);
}

assert(target_called === 2);
