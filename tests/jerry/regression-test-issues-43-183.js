// Copyright 2015 Samsung Electronics Co., Ltd.
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

check_syntax_error ('{');
check_syntax_error ('}');
check_syntax_error ('[');
check_syntax_error (']');
check_syntax_error ('(');
check_syntax_error (')');

check_syntax_error ('function f (');
check_syntax_error ('function f ()');
check_syntax_error ('function f () {');
check_syntax_error ('function f () }');
check_syntax_error ('function f ({) }');
check_syntax_error ('function f { }');
check_syntax_error ('function f {');
check_syntax_error ('function f }');

check_syntax_error ('a = [[];');

check_syntax_error ('a = {;');
check_syntax_error ('a = };');
check_syntax_error ('a = {{};');

check_syntax_error ('a = {get q {} };');
check_syntax_error ('a = {get q ( {} };');
check_syntax_error ('a = {get q ) {} };');
check_syntax_error ('a = {get q () };');
check_syntax_error ('a = {get q () { };');
check_syntax_error ('a = {get q () };');
check_syntax_error ('a = {get q () { };');
