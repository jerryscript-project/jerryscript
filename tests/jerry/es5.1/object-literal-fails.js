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

function throw_error(snippet)
{
  try
  {
    eval(snippet);
    assert (false);
  }
  catch (e)
  {
    assert (e instanceof SyntaxError);
  }
}

function throw_error_strict(snippet)
{
  'use strict'

  try
  {
    eval(snippet);
    assert (false);
  }
  catch (e)
  {
    assert (e instanceof SyntaxError);
  }
}

// These should be failed in ECMA-262 v5.1
throw_error_strict("({a:1, a:2})");
throw_error("({a:1, get a() { return 1 }})");
throw_error("({get a() {return undefined}, get a() {return undefined}})");
throw_error("({ get 1() {}, 1:1 })");
