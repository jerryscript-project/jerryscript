// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

var literal = "a"

for (var i = 0; i < 25; i++)
  literal += "\\u0061bcdefghij"

assert(eval("var " + literal + " = 42; " + literal) === 42)

literal = undefined

var str = ""
var expected = ""

for (var i = 0; i < 1000; i++)
{
  str += "123456789\\n"
  expected += "123456789\n"
}

assert(eval('"' + str + '"') === expected);

