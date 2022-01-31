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

var o = new Proxy (function f () {}, { get(t,p,r) { if (p == "prototype") { throw 42.1 } Reflect.get(...arguments) }})

try {
  Reflect.construct (Map, [], o);
  assert (false);
} catch (e) {
  assert(e == 42.1)
}

try {
  Reflect.construct (Set, [], o);
  assert (false);
} catch (e) {
  assert(e == 42.1)
}

try {
  Reflect.construct (WeakMap, [], o);
  assert (false);
} catch (e) {
  assert(e == 42.1)
}

try {
  Reflect.construct (WeakSet, [], o);
  assert (false);
} catch (e) {
  assert(e == 42.1)
}

try {
  Reflect.construct (Map);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.construct (Set);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.construct (WeakMap);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.construct (WeakSet);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

class MyMap extends Map {};
class MySet extends Set {};
class MyWeakMap extends WeakMap {};
class MyWeakSet extends WeakSet {};
var m1= new MyMap();
var s1= new MySet();
var wm1= new MyWeakMap();
var ws1= new MyWeakSet();

assert(Object.getPrototypeOf(m1) == MyMap.prototype)
assert(Object.getPrototypeOf(s1) == MySet.prototype)
assert(Object.getPrototypeOf(wm1) == MyWeakMap.prototype)
assert(Object.getPrototypeOf(ws1) == MyWeakSet.prototype)
