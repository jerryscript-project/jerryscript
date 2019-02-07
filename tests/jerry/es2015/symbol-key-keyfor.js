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

try {
  Symbol.for (Symbol('foo'));
  assert (false);
} catch (e) {
  assert (e instanceof TypeError)
}

var a = Symbol.for ('foo');
var b = Symbol.for ('foo');

assert (a === b);
assert (a == b);

assert (Symbol.keyFor (a) === 'foo');
assert (Symbol.keyFor (b) === 'foo');

// Test non-string arguments
var c = Symbol.for (5);
var d = Symbol.for (5.58);
var e = Symbol.for ({});

assert (Symbol.keyFor (c) === '5');
assert (Symbol.keyFor (d) === '5.58');
assert (Symbol.keyFor (e) === '[object Object]');

// Test global symbol table
var array = [];
for (var i = 0; i < 15; i++) {
  array[i] = Symbol.for ('foo' + i);

  for (var j = 0; j < i; j++) {
    assert (array[j] !== array[i]);
  }
}

try {
  Symbol.keyFor ('NonSymbolValue');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

for (var i = 0; i < 15; i++) {
  assert (Symbol.keyFor (array[i]) === ('foo' + i));
}
