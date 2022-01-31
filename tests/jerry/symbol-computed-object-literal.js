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

var symbolFoo = Symbol ('foo');
var symbolBar = Symbol ('bar');

var obj = {
  _a : 5,
  get [symbolFoo]() {
    return this._a;
  },
  set [symbolFoo](a) {
    this._a = a;
  },
  [symbolBar] : 10
}

/* Test accessor properties */
assert (obj[symbolFoo] === 5);
obj[symbolFoo] = 6;
assert (obj[symbolFoo] === 6);

/* Test nameddata properties */
assert (obj[symbolBar] === 10);
obj[symbolBar] = 20;
assert (obj[symbolBar] === 20);
