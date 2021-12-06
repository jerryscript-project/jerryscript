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

class NoParent {
  static [Symbol.hasInstance] (arg) {
    return false;
  }
}

var obj = new NoParent ();

assert ((obj instanceof NoParent) === false);

class PositiveNumber {
  static [Symbol.hasInstance] (arg) {
    return (arg instanceof Number) && (arg >= 0);
  }
}

var num_a = new Number (33);
var num_b = new Number (-50);

assert ((num_a instanceof PositiveNumber) === true);
assert ((num_b instanceof PositiveNumber) === false);


class ErrorAlways {
  static [Symbol.hasInstance] (arg) {
    throw new URIError("ErrorAlways");
  }
}

try {
  (new Object ()) instanceof ErrorAlways;
  assert (false);
} catch (ex) {
  assert (ex instanceof URIError);
}
