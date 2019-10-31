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

function check_reference_error (code)
{
  try {
    eval (code);
    assert (false);
  } catch (e) {
    assert (e instanceof ReferenceError);
  }
}

check_reference_error ("let b = a, a;");
check_reference_error ("const b = b;");
check_reference_error ("a; let b, a;");
check_reference_error ("a = 1; let b, a;");

function f() {
  return x + y.a;
}

check_reference_error ("x");
check_reference_error ("y");
check_reference_error ("x = 1");
check_reference_error ("y = 1");

let x = 6;
assert (x === 6);
let y = { a: 7 };
assert (y.a === 7);

assert (f() === 13);
