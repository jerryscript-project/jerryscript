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

// ES v6.0 21.1.3.13.6
var z = "I am vengeance";
try {
  z.repeat (-1);
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

// ES v6.0 21.1.3.13.7
var x = "I am the night";
try {
  print(z.repeat (Infinity));
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

// ES v6.0 21.1.3.13.8
assert (z.repeat (0) === "");
assert (z.repeat (NaN) === "");

var y = "I am batman ";
assert (y.repeat (3) === "I am batman I am batman I am batman ");

assert (String.prototype.repeat.call ("My cat is awesome. ", 3) === "My cat is awesome. My cat is awesome. My cat is awesome. ");

try {
  String.prototype.repeat.call (undefined);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.repeat.call (null);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.repeat.call (undefined, "Sylveon");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var a = Symbol ("Unicorn invasion.", 3);
try {
  String.prototype.repeat.call (a);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

