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

var a = 5;
var a = 6;
let b = 7;

assert (a === 6);
assert (this.a === 6);
assert (b === 7);
assert (this.b === undefined);

{
  let c;
  c = 8;

  {
    let c = 9;
    assert (c === 9);
  }

  {
    function c() { return 10 }
    assert (c() === 10);
  }

  assert (c === 8);
}
assert (typeof c === "undefined");

