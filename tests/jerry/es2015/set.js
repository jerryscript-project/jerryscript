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

var set = new Set();
var int = 1;
assert (set.size === 0);
assert (set.add (int) === set);
assert (set.has (int));
assert (set.size === 1);

var str = "foobar"
assert (set.add (str) === set);
assert (set.has (str));
assert (set.size === 2);

var number = 5.78;
assert (set.add (number) === set);
assert (set.has (number));
assert (set.size === 3);

var object = { a : 2, b : 4};
assert (set.add (object) === set);
assert (set.has (object));
assert (set.size === 4);

var func = function () {};
assert (set.add (func) === set);
assert (set.has (func));
assert (set.size === 5);

var symbol = Symbol ("foo");
assert (set.add (symbol) === set);
assert (set.has (symbol));
assert (set.size === 6);

assert (!set.has(5));
assert (!set.has("foo"));
assert (!set.has({ a : 2, b : 4}));
assert (!set.has(function () {}));
assert (!set.has(Symbol ("foo")));

var elements = [int, str, number, object, func, symbol];

var i = 0;
set.forEach (function (value, key) {
  assert (key === elements[i]);
  assert (value === elements[i]);
  i++;
});

assert (set.delete (int));
assert (set.size === 5);
assert (set.delete (str));
assert (set.size === 4);
assert (set.delete (number));
assert (set.size === 3);
assert (set.delete (object));
assert (set.size === 2);
assert (set.delete (func));
assert (set.size === 1);
assert (set.delete (symbol));
assert (set.size === 0);

set = new Set([1, 2, 3, 4]);
assert (set.has(1));
assert (set.has(2));
assert (set.has(3));
assert (set.has(4));

assert (set.size === 4);
assert (set.add (2) === set);
assert (set.size === 4);
assert (set.delete (2));
assert (set.size === 3);

set.clear();
assert(set.size === 0);

set.add(3);
assert(set.delete(3));
assert(!set.delete(3));
