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

assert (Object.prototype.toString.call (Date.prototype) === '[object Date]');
assert (isNaN(Date.prototype.valueOf()));

assert (isNaN (Date.prototype.setTime()));
assert (isNaN (Date.prototype.setMilliseconds()));
assert (isNaN (Date.prototype.setUTCMilliseconds()));
assert (isNaN (Date.prototype.setSeconds()));
assert (isNaN (Date.prototype.setUTCSeconds()));
assert (isNaN (Date.prototype.setMinutes()));
assert (isNaN (Date.prototype.setUTCMinutes()));
assert (isNaN (Date.prototype.setHours()));
assert (isNaN (Date.prototype.setUTCHours()));
assert (isNaN (Date.prototype.setDate()));
assert (isNaN (Date.prototype.getUTCDate()));
assert (isNaN (Date.prototype.setMonth()));
assert (isNaN (Date.prototype.setUTCMonth()));
assert (isNaN (Date.prototype.setFullYear()));
assert (isNaN (Date.prototype.setUTCFullYear()));

assert (Object.prototype.toString.call (RegExp.prototype) === '[object RegExp]');

assert (RegExp.prototype.source === "(?:)");
assert (RegExp.prototype.global === false);
assert (RegExp.prototype.ignoreCase === false);
assert (RegExp.prototype.multiline === false);

RegExp.prototype.source = "a";
RegExp.prototype.global = true;
RegExp.prototype.ignoreCase = true;
RegExp.prototype.multiline = true;
assert (RegExp.prototype.source === "(?:)");
assert (RegExp.prototype.global === false);
assert (RegExp.prototype.ignoreCase === false);
assert (RegExp.prototype.multiline === false);

delete RegExp.prototype.source;
delete RegExp.prototype.global;
delete RegExp.prototype.ignoreCase;
delete RegExp.prototype.multiline;
assert (RegExp.prototype.source === "(?:)");
assert (RegExp.prototype.global === false);
assert (RegExp.prototype.ignoreCase === false);
assert (RegExp.prototype.multiline === false);

var prototypes = [
  Error.prototype,
  EvalError.prototype,
  RangeError.prototype,
  ReferenceError.prototype,
  SyntaxError.prototype,
  TypeError.prototype,
  URIError.prototype
]

prototypes.forEach (function (proto) {
  assert (Object.prototype.toString.call (proto) === '[object Error]');
})
