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

var name1 = "";
var name2 = "";
var name3 = "";
function foo() {};

try
{
  new Promise();
}
catch (e)
{
  name1 = e.name;
}

try
{
  Promise(foo);
}
catch (e)
{
  name2 = e.name;
}

try
{
  new Promise("string");
}
catch (e)
{
  name3 = e.name;
}

assert (name1 === "TypeError");
assert (name2 === "TypeError");
assert (name3 === "TypeError");
