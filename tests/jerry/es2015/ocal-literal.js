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

function checkSyntaxError (str) {
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

checkSyntaxError ("0p");
checkSyntaxError ("0o");
checkSyntaxError ("0o0123456789");
checkSyntaxError ("0o9");

checkSyntaxError ("0p");
checkSyntaxError ("0O");
checkSyntaxError ("0O9");

// Check strict mode
checkSyntaxError ("'use strict'; 0777");
assert (eval ("'use strict'; 0o777") === 511);

assert (0o123 === 83);
assert (0o77777777 === 16777215);
assert (0o767 === parseInt ("767", 8));
assert (0767 === 0o767);

assert (0O123 === 83);
assert (0O77777777 === 16777215);
assert (0O767 === parseInt ("767", 8));
assert (0767 === 0O767);
