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

print("break test");

print ("var cat");
var cat = 'cat';

function test(x)
{
  function f() {
    return 0;
  }

  function f() {
    /* Again. */
    return 1;
  }

  function f() {
    /* And again. */
    return 2;
  }

  print("function test");
  var a = 3;
  var b = 5, c = a + b;
  global_var = f();
  return c;
}

function f() {
  /* And again. */
}

var
  x =
    1;

test(x);
