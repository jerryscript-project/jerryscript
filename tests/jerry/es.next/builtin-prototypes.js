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

var prototypes = [
  String.prototype,
  Boolean.prototype,
  Number.prototype,
  Date.prototype,
  RegExp.prototype,
  Error.prototype,
  EvalError.prototype,
  RangeError.prototype,
  ReferenceError.prototype,
  SyntaxError.prototype,
  TypeError.prototype,
  URIError.prototype
]

for (proto of prototypes) {
  assert (Object.prototype.toString.call (proto) === '[object Object]');
}

try {
  String.prototype.toString();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Boolean.prototype.valueOf();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Number.prototype.valueOf();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Date.prototype.valueOf();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.exec("");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.compile("a", "u");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.source;
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.global;
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.ignoreCase;
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.multiline;
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.sticky;
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  RegExp.prototype.unicode;
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
