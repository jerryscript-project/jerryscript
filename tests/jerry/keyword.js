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

function check_syntax_error(code)
{
  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

function check_strict_syntax_error(code)
{
  "use strict"

  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

check_syntax_error("d\\u006f {} while (false)")
check_syntax_error("\\u0076\\u0061\\u0072 var = 5")
check_syntax_error("wit\\u0068 ({}) {}")
check_syntax_error("\\u0066alse")
check_syntax_error("type\\006ff 3.14")
check_syntax_error("try {} fin\\u0061lly {}")
check_syntax_error("f\\u0075nction f() {}")
check_syntax_error("a instanc\\u0065of b")

check_strict_syntax_error("\\u006c\\u0065\\u0074 _let = 5");
check_strict_syntax_error("\\u0070rotecte\\u0064");
