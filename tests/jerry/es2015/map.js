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

var m = new Map();
assert(m.size == 0);
assert(m.set(1, 1) === m);
assert(m.has(1));
assert(m.size == 1);

assert(m.set(undefined, 123) === m);
assert(m.size == 2);
assert(m.has(undefined));
assert(m.get(undefined) === 123);

assert(m.set(null, 456) === m);
assert(m.size == 3);
assert(m.has(null));
assert(m.get(null) === 456);

assert(m.set("strItem", { x:789 }) === m);
assert(m.size == 4);
assert(m.has("str" + "Item"));
assert(m.get("st" + "rItem").x === 789);

assert(m.set(12.25, 12.25) === m);
assert(m.size == 5);
assert(m.has(12 + (function() { return 0.25 })()));
assert(m.get(13 - (function() { return 0.75 })()) === 12.25);

assert(m.delete(1))
assert(m.size == 4);
assert(!m.has(1));
assert(m.get(1) === undefined);

assert(!m.delete(2));

assert(m.delete(12 + (function() { return 0.25 })()));
assert(m.size == 3);
assert(!m.has(12.25));
assert(m.get(12.25) === undefined);

assert(m.delete("strI" + "tem"))
assert(m.delete(undefined))
assert(m.size == 1);

assert(m.delete(null))
assert(m.size == 0);

m.set(1,1)
m.set(2,2)
m.set(3,3)
m.set(1,7)
m.set(1,8)
m.set(2,7)
m.set(2,8)
m.set(3,7)
m.set(3,8)

assert(m.size == 3);
assert(m.get(1) === 8);
assert(m.get(2) === 8);
assert(m.get(3) === 8);

m.set(NaN, "not a number");
assert(m.size === 4);
assert(m.get(NaN) === "not a number");
assert(m.get(Number("foo")) === "not a number")

m.set(0, "PosZero");
assert(m.size === 5);
m.set(-0, "NegZero");
assert(m.size === 5);
assert(m.get (0) === "NegZero");
assert(m.get (-0) === "NegZero");

var symbolFoo = Symbol ("foo");
m.set (symbolFoo, "SymbolFoo");
assert(m.size === 6);
assert(m.get(symbolFoo) === "SymbolFoo");

var object = {};
m.set (object, "object");
assert(m.size === 7);
assert(m.get(object) === "object");
assert(m.get({}) === undefined);

var myObj = { o : 4 };
m.set("foo", myObj);
assert(m.size === 8);
assert(m.get ("foo") === myObj);
assert(m.get ("foo").o === 4);

m.clear();
assert(m.size === 0);

m.set("foo", myObj);
assert(m.size === 1);
assert(m.get ("foo") === myObj);
assert(m.get ("foo").o === 4);

var mapNameDesc = Object.getOwnPropertyDescriptor (Map, 'name');
assert(mapNameDesc.value === "Map");
assert(mapNameDesc.writable === false);
assert(mapNameDesc.enumerable === false);
assert(mapNameDesc.configurable === true);

assert(Map.prototype.name === undefined);

m = new Map([{0: "foo", 1: 3}, {0 : "bar", 1 : 2}]);

assert (m.size === 2);
assert (m.get("foo") === 3);
assert (m.get("bar") === 2);
