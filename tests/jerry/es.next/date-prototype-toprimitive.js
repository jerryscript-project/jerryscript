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

var dateObj = new Date("1997-04-10");
var dateNaN = new Date(NaN);

// Test with default hint
var result = dateObj[Symbol.toPrimitive]("default");
assert(result.toString().substring(0,15) === "Thu Apr 10 1997");
result = dateNaN[Symbol.toPrimitive]("default");
assert(dateNaN == "Invalid Date");

// Test with number hint
result = dateObj[Symbol.toPrimitive]("number");
assert(result.toString() === "860630400000");
result = dateNaN[Symbol.toPrimitive]("number");
assert(isNaN(result) === true);

// Test with string hint
result = dateObj[Symbol.toPrimitive]("string");
assert(result.toString().substring(0,15) === "Thu Apr 10 1997");
result = dateNaN[Symbol.toPrimitive]("string");
assert(result == "Invalid Date");

// Test with invalid hint
try {
  result = dateObj[Symbol.toPrimitive](90);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Test with invalid hint value
try {
  dateObj[Symbol.toPrimitive]('error');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Test when unable to call toPrimitive
try {
  Date.prototype[Symbol.toPrimitive].call(undefined);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
