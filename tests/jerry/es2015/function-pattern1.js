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

function must_throw (str, type = SyntaxError)
{
  try
  {
    eval (str);
    assert (false);
  }
  catch (e)
  {
    assert (e instanceof type)
  }
}

must_throw ("function f(a, [a]) {}");
must_throw ("function f([a], a) {}");
must_throw ("function f(a = b, [b]) {}; f()", ReferenceError);
must_throw ("function f([a+b]) {}");
must_throw ("function f([a().b]) {}");
must_throw ("function f(...[a] = [1]) {}");

function a1([a,b]) {
  var a, b;

  assert(a === 1);
  assert(b === undefined);

  var a = 3;
  assert(a === 3);
}
a1([1]);

function a2([a,b]) {
  eval("var a, b");
  assert(a === 1);
  assert(b === undefined);

  eval("var a = 3");
  assert(a === 3);
}
a2([1]);

function f1(a, [b], c, [d], e)
{
  assert (a === 1);
  assert (b === 2);
  assert (c === 3);
  assert (d === 4);
  assert (e === 5);
}
f1(1, [2], 3, [4], 5)

function f2(a, [b], c, [d], e)
{
  eval("");
  assert (a === 1);
  assert (b === 2);
  assert (c === 3);
  assert (d === 4);
  assert (e === 5);
}
f2(1, [2], 3, [4], 5)

var g1 = (a, [b], c, [d], e) => {
  assert (a === 1);
  assert (b === 2);
  assert (c === 3);
  assert (d === 4);
  assert (e === 5);
}
g1(1, [2], 3, [4], 5)

var g2 = (a, [b], c, [d], e) => {
  eval("");
  assert (a === 1);
  assert (b === 2);
  assert (c === 3);
  assert (d === 4);
  assert (e === 5);
}
g2(1, [2], 3, [4], 5)
