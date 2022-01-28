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

function check_syntax_error (script)
{
  try
  {
    eval (script);
    assert (false);
  }
  catch (e)
  {
    assert (e instanceof SyntaxError);
  }
}

eval("function f(){}; var f;");
eval("var f; function f(){};");

eval("function f(){}; { var f; }")
eval("{ var f; } function f(){};")

eval("{ function f(){}; } var f;")
eval("var f; { function f(){}; }")

check_syntax_error ("{ function f(){}; var f; }");
check_syntax_error ("{ var f; function f(){}; }");

eval("{ { function f(){}; } var f; }")
eval("{ var f; { function f(){}; } }")

check_syntax_error ("{ function f(){}; { var f; } }")
check_syntax_error ("{ { var f; } function f(){}; }")

eval("{ { function f(){}; } { var f; } }")
eval("{ { var f; } { function f(){}; } }")

eval("function g(){ function f(){}; var f; }")
eval("function g(){ var f; function f(){}; }")

eval("function g(){ function f(){}; { var f; } }")
eval("function g(){ { var f; } function f(){}; }")

eval("function g(){ { function f(){}; } var f; }")
eval("function g(){ var f; { function f(){}; } }")
