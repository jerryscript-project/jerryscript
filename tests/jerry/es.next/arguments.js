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
