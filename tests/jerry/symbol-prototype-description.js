/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

assert(Symbol('desc').description === "desc");
assert(Symbol.iterator.description === "Symbol.iterator");
assert(Symbol.for('foo').description === "foo");
assert(`${Symbol('foo').description}bar` === "foobar");

var desc = Object.getOwnPropertyDescriptor(Symbol.prototype, 'description');

assert(desc.set === undefined);
assert(typeof desc.get === "function");
assert(desc.writable === undefined);
assert(desc.enumerable === false);
assert(desc.configurable === true);

var sym = Symbol('foo')
assert(desc.get.call(Object(sym)) === "foo")

var obj_sym = Object(Symbol('foo'));
assert(obj_sym.description === "foo");

try {
  desc.get.call(null);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  desc.get.call(123);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  desc.get.call('test');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  desc.get.call(undefined);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  desc.get.call({});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  desc.get.call(new Proxy({}, {}));
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

assert(Symbol("").description !== undefined);
assert(Symbol().description === undefined);
