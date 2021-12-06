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

function f_arg (arguments)
{
  return arguments;
}
assert (f_arg (1) === 1);

function f (a, b, c)
{
  return arguments;
}

args = f();
assert (args[0] === undefined);

args = f (1, 2, 3, 4, 5);
assert (args[0] === 1);
assert (args[1] === 2);
assert (args[2] === 3);
assert (args[3] === 4);
assert (args[4] === 5);
assert (args[5] === undefined);

assert (args.callee === f);
assert (typeof args.caller === 'undefined');

function g (a, b, c)
{
  assert (arguments[0] === 1);
  assert (arguments[1] === undefined);
  assert (arguments[2] === undefined);

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 'a');
  assert (arguments[1] === 'b');
  assert (arguments[2] === 'c');

  arguments [0] = 1;
  arguments [1] = 2;
  arguments [2] = 3;

  assert (a === 1);
  assert (b === 2);
  assert (c === 3);

  delete arguments [0];
  arguments[0] = 'new value';
  assert (a === 1);

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 'new value');
  assert (arguments[1] === 'b');
  assert (arguments[2] === 'c');
}

g (1);

fn_expr = function (a, b, c)
{
  'use strict';

  assert (arguments[0] === 1);
  assert (arguments[1] === undefined);
  assert (arguments[2] === undefined);

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 1);
  assert (arguments[1] === undefined);
  assert (arguments[2] === undefined);

  arguments [0] = 1;
  arguments [1] = 'p';
  arguments [2] = 'q';

  assert (a === 'a');
  assert (b === 'b');
  assert (c === 'c');

  delete arguments [0];
  arguments[0] = 'new value';
  assert (a === 'a');

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 'new value');
  assert (arguments[1] === 'p');
  assert (arguments[2] === 'q');

  function check_type_error_for_property (obj, prop) {
    try {
      var v = obj[prop];
      assert (false);
    }
    catch (e) {
      assert (e instanceof TypeError);
    }
  }

  check_type_error_for_property (arguments, 'callee');
}

fn_expr (1);

(function () {
 var a = [arguments];
})();

function nested_args()
{
  var a;
  for (var i = 0; i < 1; i++)
  {
    if (i == 0)
    {
      a = arguments[i];
    }
  }
  assert(a === 3);
}
nested_args(3);


function f1(a, b, c)
{
  'use strict';
  assert(!Object.hasOwnProperty(arguments,'caller'));
}

f1(1, 2, 3);

// Normal arguments access

function f2(a = arguments)
{
  assert(arguments[1] === 2)
  var arguments = 1
  assert(arguments === 1)
  assert(a[1] === 2)
}
f2(undefined, 2)

function f3(a = arguments)
{
  assert(arguments() === "X")
  function arguments() { return "X" }
  assert(arguments() === "X")
  assert(a[1] === "R")
}
f3(undefined, "R")

function f4(a = arguments)
{
  const arguments = 3.25
  assert(arguments === 3.25)
  assert(a[1] === -1.5)
}
f4(undefined, -1.5)

// Normal arguments access with eval

function f5(a = arguments)
{
  assert(arguments[1] === 2)
  var arguments = 1
  assert(arguments === 1)
  assert(a[1] === 2)
  eval()
}
f5(undefined, 2)

function f6(a = arguments)
{
  assert(arguments() === "X")
  function arguments() { return "X" }
  assert(arguments() === "X")
  assert(a[1] === "R")
  eval()
}
f6(undefined, "R")

function f7(a = arguments)
{
  const arguments = 3.25
  assert(arguments === 3.25)
  assert(a[1] === -1.5)
  eval()
}
f7(undefined, -1.5)

// Argument access through a function

function f8(a = () => arguments)
{
  assert(arguments[1] === 2)
  var arguments = 1
  assert(arguments === 1)
  assert(a()[1] === 2)
}
f8(undefined, 2)

function f9(a = () => arguments)
{
  assert(arguments() === "X")
  function arguments() { return "X" }
  assert(arguments() === "X")
  assert(a()[1] === "R")
}
f9(undefined, "R")

function f10(a = () => arguments)
{
  let arguments = 3.25
  assert(arguments === 3.25)
  assert(a()[1] === -1.5)
}
f10(undefined, -1.5)

// Argument access through an eval

function f11(a = eval("() => arguments"))
{
  assert(arguments[1] === 2)
  var arguments = 1
  assert(arguments === 1)
  assert(a()[1] === 2)
}
f11(undefined, 2)

function f12(a = eval("() => arguments"))
{
  assert(arguments() === "X")
  function arguments() { return "X" }
  assert(arguments() === "X")
  assert(a()[1] === "R")
}
f12(undefined, "R")

function f13(a = eval("() => arguments"))
{
  const arguments = 3.25
  assert(arguments === 3.25)
  assert(a()[1] === -1.5)
}
f13(undefined, -1.5)

// Other cases

try {
  function f14(a = arguments)
  {
    assert(a[1] === 6)
    arguments;
    let arguments = 1;
  }
  f14(undefined, 6)
  assert(false)
} catch (e) {
  assert(e instanceof ReferenceError)
}

try {
  eval("'use strict'; function f(a = arguments) { arguments = 5; eval() }");
  assert(false)
} catch (e) {
  assert(e instanceof SyntaxError)
}

function f15()
{
  assert(arguments[0] === "A")
  var arguments = 1
  assert(arguments === 1)
}
f15("A")

function f16()
{
  assert(arguments() === "W")
  function arguments() { return "W" }
  assert(arguments() === "W")
}
f16("A")

function f17(a = arguments = "Val")
{
  assert(arguments === "Val")
}
f17();

function f18(s = (v) => arguments = v, g = () => arguments)
{
  const arguments = -3.25
  s("X")

  assert(g() === "X")
  assert(arguments === -3.25)
}
f18()

function f19(e = (v) => eval(v))
{
  var arguments = -12.5
  e("arguments[0] = 4.5")

  assert(e("arguments[0]") === 4.5)
  assert(e("arguments[1]") === "A")
  assert(arguments === -12.5)
}
f19(undefined, "A");

function f20 (arguments, a = eval('arguments')) {
  assert(a === 3.1);
  assert(arguments === 3.1);
}
f20(3.1);

function f21 (arguments, a = arguments) {
  assert(a === 3.1);
  assert(arguments === 3.1);
}
f21(3.1);

function f22 (arguments, [a = arguments]) {
  assert(a === 3.1);
  assert(arguments === 3.1);
}
f22(3.1, []);

try {
  function f23(p = eval("var arguments"), arguments)
  {
  }
  f23()
  assert(false)
} catch (e) {
  assert(e instanceof SyntaxError)
}

try {
  function f24(p = eval("var arguments")) {
    let arguments;
  }
  f24()
  assert(false)
} catch (e) {
  assert(e instanceof SyntaxError)
}

try {
  function f25(p = eval("var arguments")) {
    function arguments() { }
  }
  f25()
  assert(false)
} catch (e) {
  assert(e instanceof SyntaxError)
}

function f26(arguments, eval = () => eval()) {
  assert(arguments === undefined);
}
f26(undefined);
