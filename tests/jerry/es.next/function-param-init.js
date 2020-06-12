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

var local = 0;

switch(0) { /* This switch forces a pre-scanner run. */
default:

  function f(a = (5, local = 6),
             b = ((5 + function(a = 6) { return a }() * 3)),
             c,
             d = true ? 1 : 2)
  {
    return "" + a + ", " + b + ", " + c + ", " + d;
  }

  assert(f() === "6, 23, undefined, 1");
  assert(local === 6);

  var obj = {
    f: function(a = [10,,20],
                b,
                c = Math.cos(0),
                d)
    {
      return "" + a + ", " + b + ", " + c + ", " + d;
    }
  };

  assert(obj.f() === "10,,20, undefined, 1, undefined");

  function g(a, b = (local = 7)) { }

  local = 0;
  g();
  assert(local === 7);

  local = 0;
  g(0);
  assert(local === 7);

  local = 0;
  g(0, undefined);
  assert(local === 7);

  local = 0;
  g(0, null);
  assert(local === 0);

  local = 0;
  g(0, false);
  assert(local === 0);
  break;
}

function CheckSyntaxError(str)
{
  try {
    eval(str);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}

CheckSyntaxError('function x(a += 5) {}');
CheckSyntaxError('function x(a =, b) {}');
CheckSyntaxError('function x(a = (b) {}');
CheckSyntaxError('function x(a, a = 5) {}');
CheckSyntaxError('function x(a = 5, a) {}');

// Pre-scanner tests.
var str = "a = 5, b, c = function() { for (var a = 0; a < 4; a++) ; return a; } ()"

var f = new Function (str, str);
f();

var f = new Function (str, "return (a + c) * (b == undefined ? 1 : 0)");
assert (f() == 9);

function duplicatedArg (a = c, b = d, c) {
  assert(a === 1);
  assert(b === 2);
  assert(c === 3);
}

duplicatedArg(1, 2, 3);
