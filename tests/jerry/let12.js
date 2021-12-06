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

function checkSyntax (str) {
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

checkSyntax ("if (5) let a;");
checkSyntax ("if (5) const a;");
checkSyntax ("if (0) {} else let a;");
checkSyntax ("if (0) {} else const a;");
checkSyntax ("while (5) let a;");
checkSyntax ("while (5) const a;");
checkSyntax ("do let a; while (5)");
checkSyntax ("do const a; while (5)");
checkSyntax ("for (a in b) let c;");
checkSyntax ("for (a in b) const c;");
checkSyntax ("for (a of b) let c;");
checkSyntax ("for (a of b) const c;");
checkSyntax ("for (;;) let c;");
checkSyntax ("for (;;) const c;");
checkSyntax ("with ({}) let c;");
checkSyntax ("with ({}) const c;");
checkSyntax ("a: let c;");
checkSyntax ("a: const c;");
