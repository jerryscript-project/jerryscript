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

function f(a, b = a)
{
  function a() { return 2; }

  assert(a() === 2);
  assert(b === 1)
}
f(1);

function g(a, b = a)
{
  function a() { return 2; }

  eval("assert(a() === 2)");
  eval("assert(b === 1)");
}
g(1);

var x = 1;
function h(a = x) {
  assert(x === undefined);
  var x = 2;
  assert(a === 1);
  assert(x === 2);
}
h();

x = function() { return 4; }
let y = 6;

function i(a = x() / 2, b = (y) + 2, c = typeof z) {
  let y = 10;
  let z = 11;

  function x() { return 5; }

  assert(a === 2);
  assert(x() === 5);
  assert(b === 8);
  assert(c === "undefined");
  assert(y === 10);
  assert(z === 11);
}
i();

var arguments = 10;
function j(a = arguments[1])
{
  assert(a === 2);
  a = 3;
  assert(arguments[0] === undefined)
}
j(undefined,2);

function k(a = 2)
{
  let d = 5;
  assert(d === 5);
  eval("assert(a === 2)");
}
k();

function l(a = 3)
{
  const d = 6;
  assert(d === 6);
  eval("assert(a === 3)");
}
l();

function m(a, b = 2)
{
  assert(a === 1);
  assert(arguments[0] === 1);
  assert(b === 2);
  assert(arguments[1] === undefined);

  a = 8;
  b = 9;

  assert(a === 8);
  assert(arguments[0] === 1);
  assert(b === 9);
  assert(arguments[1] === undefined);
}
m(1);
