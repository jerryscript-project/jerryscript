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

var m1 = new WeakMap();
var k1 = {};
assert (m1.set (k1, 3.14) instanceof WeakMap);
assert (m1.has (k1) === true);
assert (m1.get (k1) === 3.14);

k1 = {};
gc (); /* This will remove the entry, but there is no way to validate it from js. */

assert (m1.has (k1) === false);
assert (m1.delete(k1) === false);
assert (m1.set (k1, "asd" + "fgh") instanceof WeakMap);
assert (m1.has (k1) === true);
assert (m1.get (k1) === "asdfgh");

assert (m1.delete ("str") === false);
assert (m1.has (42) === false);
assert (m1.get (42) === undefined);
try {
  m1.set (42, 42);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

gc ();
assert (m1.delete(k1) === true);
gc ();
assert (m1.has (k1) === false);
k1 = {};
gc ();

Object.freeze(k1);
assert (m1.set (k1, "frozen") instanceof WeakMap);
assert (m1.has (k1) === true);
assert (m1.get (k1) === "frozen");
k1 = {};
gc ();

var m2 = new WeakMap();

m1.set (k1, "str" + "str");
m2.set (k1, 42.42);

k1 = {};
gc ();
var k2 = {};

m1.set (k1, "str" + "str");
m1.set (k2, "str2" + "str2");
m2.set (k1, 42.42);
m2.set (k2, Infinity);

m2.delete (k1);
gc ();
k1 = {};
k2 = {};
gc ();

var k3 = {};
m1 = new WeakMap();
m1.set(k1, "str" + "str");
m1.set(k2, "str2" + "str2");
m1.set(k3, "str3" + "str3");
k1 = undefined;
k2 = undefined;
k3 = undefined;
m1 = undefined;
gc();

try {
  WeakMap();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  WeakMap.prototype.get.call({});
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert (new WeakMap(null) instanceof WeakMap);

k1 = {};
k2 = {};
m1 = new WeakMap([[k1, 1.2], [k2, 1.3]]);
assert (m1.has (k1));
assert (m1.has (k2));
assert (m1.get (k1) === 1.2);
assert (m1.get (k2) === 1.3);
gc();
m1 = undefined;
gc();

m1 = new WeakMap();
m1.set(k1, "str");
m1.set(k1, "4");
m1.set(k1, null);
m1.set(k1, 42);
assert (m1.has (k1));
k1 = {};
gc();

m1 = new WeakMap();
m1.set(k1, "str");
m1.set(k1, "4");
m1.set(k1, 42);
m1.set(k1, null);
assert (m1.has (k1));
m1 = new WeakMap();
gc();

try {
  new WeakMap([[1,2],[3,4]]);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

function createIterable(arr, methods = {}) {
  let iterable = function *() {
    let idx = 0;
    while (idx < arr.length) {
      yield arr[idx];
      idx++;
    }
  }();
  iterable['return'] = methods['return'];
  iterable['throw'] = methods['throw'];

  return iterable;
};

var closed = false;
var iter = createIterable([1, 2, 3], {
  'return': function(){ closed = true; return {}; }
});
try {
  new WeakMap(iter);
} catch(e){}

assert(closed === true);

m1.set([], []);

assert (WeakMap.prototype.toString() === "[object WeakMap]");

WeakMap.prototype.set = function () { throw "abrupt set" };

try {
  new WeakMap([[{}, 1]]);
  assert (false);
} catch (e) {
  assert (e === "abrupt set");
}

Object.defineProperty(WeakMap.prototype, 'set', {
  get: () => { throw "abrupt set getter" }
});

try {
  new WeakMap([]);
  assert (false);
} catch (e) {
  assert (e === "abrupt set getter");
}

[Symbol.toStringTag].forEach(e => assert (Reflect.ownKeys(WeakMap.prototype).includes(e)));
