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


assert (typeof f === "undefined");

function g() { return 6; }

switch (g()) {
case f():

  let g = 9;

  assert (g === 9);

  function f() { return 6; }
  break;

default:
  assert (false);
}

assert (f() === 6);

switch (g()) {
case f() - 2:

  let g = 9;

  assert ((function() { return g + f(); })() === 17);

  function f() { return 8; }
  break;

default:
  assert (false);
}

assert (f() === 8);

switch (g()) {
case g() * 2:

  {
    let g = 4;
    assert (g == 4);
  }

  function g() { return 3; }
  break;

default:
  assert (false);
}

assert (g() === 3);
