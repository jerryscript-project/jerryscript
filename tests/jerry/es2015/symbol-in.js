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

var test_symbol = Symbol ('Test');

var obj = {};

assert ((test_symbol in obj) === false);

obj[test_symbol] = 'value';

assert ((test_symbol in obj) === true);
assert (obj[test_symbol] === 'value');


var array = [];

assert ((test_symbol in array) === false);

array[test_symbol] = 'value';

assert ((test_symbol in array) === true);
assert (array[test_symbol] === 'value');


assert ((test_symbol in Symbol) === false);

try {
  test_symbol in test_symbol;
  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}

try {
  test_symbol in 3;
  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}

try {
  test_symbol in 'Test';
  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}
