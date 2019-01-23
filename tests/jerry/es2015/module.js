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

import { b, c, d, e, f as g } from "tests/jerry/es2015/module-imported.js"

import
{
  b as pi,
  getString,
  getArea
} from "tests/jerry/es2015/module-imported-2.js"

var str = "str";

assert (b () === 2);
assert (c === 3);
assert (d () === 4);
assert (e === 1);
assert (g === str);

assert (pi === 3.14);
assert (getArea (2) == 12.56);
assert (getString (str) === "strString")
