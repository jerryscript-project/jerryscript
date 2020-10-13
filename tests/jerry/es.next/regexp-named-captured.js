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

var captured_groups = /(?<a>a).(?<b>b).(?<c>c)/;
var result = captured_groups.exec ("aXbXc");

// simple named captured group example
assert (result.groups !== undefined)
assert (result.groups.a == "a")
assert (result.groups.b == "b")
assert (result.groups.c == "c")
assert (result.groups.d == undefined)
assert (result.groups.a === result[1])
assert (result.groups.b === result[2])
assert (result.groups.c === result[3])
assert (result[0] === "aXbXc")
var without_captured_groups = /./

result = without_captured_groups.exec ("a")

// if regexp expression don't have named captured group then groups property is undefined
assert (result.groups == undefined)

// named references
var captured_groups_and_named_references = /(?<a>a).(?<b>b).(?<c>c)\k<c>\k<a>\k<b>/;
var result = captured_groups_and_named_references.exec ("aXbXccab");
assert (result[0] === "aXbXccab")

//testing syntax errors

function check_syntax_error (expression)
{
  try {
    eval(expression)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

// invalid named group
check_syntax_error ("/(?<>.)/")
check_syntax_error ("/(?<1>.)/")
check_syntax_error ("/(?<asd2>.)/")

// invalid references
check_syntax_error ("/\\k<a>(?<b>)/")
check_syntax_error ("/\\k<1>(?<1>)/")
check_syntax_error ("/\\k(?<a>.)/")
check_syntax_error ("/(?<a>.)\\k/")
check_syntax_error ("/\\k/u")
