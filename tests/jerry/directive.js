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

function check_syntax_error (code)
{
  try {
    eval (code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

eval("function f(a, b = 4) { }")
check_syntax_error ("function f(a, b = 4) { 'use strict' }")

eval('function f(...a) { }')
check_syntax_error ('function f(...a) { "use strict" }')

eval("({ f([a,b]) { } })")
check_syntax_error ("({ f([a,b]) { 'use strict' } })")

eval("function f(a, b = 4) { 'directive1'\n'directive2'\n }")
check_syntax_error ("function f(a, b = 4) { 'directive1'\n'directive2'\n'use strict' }")
