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

var items = [
  [TypeError, "TypeError"],
  [SyntaxError, "SyntaxError"],
  [URIError, "URIError"],
  [EvalError, "EvalError"],
  [RangeError, "RangeError"],
  [ReferenceError, "ReferenceError"],
];

for (var idx = 0; idx < items.length; idx++) {
  var type = items[idx][0];
  var expected_name = items[idx][1];
  assert (type.name === expected_name);

  assert ((new type).name === expected_name);
}

assert (AggregateError.name === "AggregateError");
assert (new AggregateError([]).name === "AggregateError")

try
{
  new AggregateError.name === "TypeError";
}
catch(e)
{
  assert (e instanceof TypeError)
}
