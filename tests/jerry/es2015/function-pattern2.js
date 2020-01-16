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

assert((function([a], b, {c}) {}).length === 3);

function f([a = "x", b = "y", c = "z"])
{
  assert(a === "a");
  assert(b === "b");
  assert(c === "z");
}
f("ab")

function g({ ["x" + "y"]: { a = 4, b = 5 }, })
{
  assert(a === 1);
  assert(b === 5);
}
g({ xy: { a:1 } });

function h([,,a,,b,,])
{
  assert(a === 3);
  assert(b === 5);
}
h([1,2,3,4,5,6,7,8])

function i([a] = [42], b = a)
{
  assert(a === 42);
  assert(b === 42);
}
i();

function j(a, [[b = a, [c] = [b], { d } = { d:eval("c") }], e = d + 1] = [[]])
{
  assert(a === 8);
  assert(b === 8);
  assert(c === 8);
  assert(d === 8);
  assert(e === 9);
}
j(8);

function k([a = function() { return a; }])
{
  assert(typeof a === "function");
  assert(a() === a);
}
k([]);

function l(a = 0, ...[b, c = 1, d = 4])
{
  assert(a === 1);
  assert(b === 2);
  assert(c === 3);
  assert(d === 4);
}
l(1,2,3);

Function("{a, x:b}","[c]", "{ 'dd':d, e = Math.cos(0)}",
  "assert(a === 1);" +
  "assert(b === undefined);" +
  "assert(c === 3);" +
  "assert(d === 4);" +
  "assert(e === 1);"
)({a:1, b:3}, [3], {a:1, b:2, dd:4});

function m()
{
  var prop_name = "x";
  var def_val = 123;

  function g({[prop_name]: a, b = def_val })
  {
    assert(a === 12);
    assert(b === 123);
  }

  g({ x:12 })
}
m();
