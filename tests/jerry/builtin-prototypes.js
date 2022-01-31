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

assert (Object.prototype.toString.call (String.prototype) === '[object String]');
assert (String.prototype.toString() === "");

assert (Object.prototype.toString.call (Boolean.prototype) === '[object Boolean]');
assert (Boolean.prototype.valueOf() === false);

assert (Object.prototype.toString.call (Number.prototype) === '[object Number]');
assert (Number.prototype.valueOf() === 0);

var prototypes = [
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

assert (RegExp.prototype.source === '(?:)');
assert (RegExp.prototype.global === undefined);
assert (RegExp.prototype.ignoreCase === undefined);
assert (RegExp.prototype.multiline === undefined);
assert (RegExp.prototype.sticky === undefined);
assert (RegExp.prototype.unicode === undefined);
assert (RegExp.prototype.dotAll === undefined);
assert (RegExp.prototype.flags === '');
