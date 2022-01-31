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

function check_syntax_error (txt) {
  try {
    eval (txt)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

var a = 21;
var b = 10;
var c;

check_syntax_error ("c =  a++b");
check_syntax_error ("c =  a--b");

check_syntax_error ("c = a +* b");
check_syntax_error ("c = a -* b");
check_syntax_error ("c = a +/ b");
check_syntax_error ("c = a -/ b");
check_syntax_error ("c = a +% b");
check_syntax_error ("c = a -% b");

check_syntax_error ("a =* b");
check_syntax_error ("a =/ b");
check_syntax_error ("a =% b");

check_syntax_error ("c = a+");
check_syntax_error ("c = a-");

check_syntax_error("a++\n()");
check_syntax_error("a--\n.b");

assert((-2 .toString()) === -2);

Number.prototype[0] = 123;
assert(-2[0] === -123);

function f() {
  var a = 0;
  function g() {}

  try {
    eval ("g(this, 'a' = 1)");
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }

  try {
    eval ("g(this, 'a' += 1)");
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }

  assert (a === 0);
}
f();

function g(a, b)
{
  assert(b === "undefined");
}
g(this, typeof undeclared_var)

function h()
{
  var done = false;
  var o = { a: function () { done = (this === o) } }
  function f() {}

  with (o) {
    f(this, a());
  }
  assert(done);
}
h();
