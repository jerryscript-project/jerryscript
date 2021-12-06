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

//basic valid cases with logical operator
assert((1 ?? 2) == 1)
assert((0 ?? 2) == 0)
assert(null ?? 2 == 2)
assert(null ?? undefined == undefined)
assert(null ?? undefined ?? 2 == 2)
assert(null ?? undefined ?? null ?? 10  == 10)
assert(null ?? (undefined || null) ?? 10  == 10)
assert(null ?? (undefined && null) ?? 10  == 10)
assert((null ?? true) && (true ?? null) == true)

//cannot evulate the right expression if left is not null or undefined
function poison () {
  throw 23;
}
assert(true ?? poison ())
assert(null ?? null ?? true ?? poison ())

function checkSyntax (str) {
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

// invalid use cases
var headNullish1 = "(null ?? null || null )";
var headNullish2 = "(null ?? null && null )";
var tailNullish1 = "(null || null ?? null )";
var tailNullish2 = "(null || null ?? null )";

checkSyntax (headNullish1);
checkSyntax (headNullish2);
checkSyntax (tailNullish1);
checkSyntax (tailNullish2);
