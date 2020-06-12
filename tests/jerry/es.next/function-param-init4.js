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

function check_reference_error(code)
{
  try {
    eval(code);
    assert(false);
  } catch (e) {
    assert(e instanceof ReferenceError);
  }
}

function f1(a = a)
{
  assert(a === 1)
}
f1(1)
check_reference_error("f1()");

function f2([a] = 1 + a)
{
  assert(a === 2)
}
f2([2])
check_reference_error("f2()");

function f3([a = !a])
{
  assert(a === 2)
}
f3([2])
check_reference_error("f3([])");

function f4([[a]] = a)
{
  assert(a === 3)
}
f4([[3]])
check_reference_error("f4()");

function f5([[a], b = a] = a)
{
  assert(a === 4 && b === 4)
}
f5([[4]])
check_reference_error("f5()")

function f6(a = 3 - ((b)), b)
{
  assert(a === 1 && b === 2)
}
f6(1, 2)
check_reference_error("f6(undefined, 2)");

function f7(a = b(), [b])
{
  assert(a === 3 && b === 4)
}
f7(3, [4])
check_reference_error("f7(undefined, [4])");

function f8(a = (function () { return a * 2 })())
{
  assert(a === 1)
}
f8(1)
check_reference_error("f8()");

function f9({a = b, b:{b}})
{
  assert(a === 2 && b === 3)
}
f9({a:2, b:{b:3}})
check_reference_error("f9({b:{b:3}})");

function f10(a = eval("a"))
{
  assert(a === 1)
}
f10(1)
check_reference_error("f10()");

function f11([a] = eval("a"))
{
  assert(a === 2)
}
f11([2])
check_reference_error("f11()");

function f12({a} = eval("a"))
{
  assert(a === 3)
}
f12({a:3})
check_reference_error("f12()");

function f13(a = arguments)
{
  assert(a[0] === undefined)
  assert(a[1] === 4)
  arguments[0] = 5
  assert(a[0] === 5)
}
f13(undefined, 4)

function f14(a, b = function() { return a; }(), c = (() => a)())
{
  assert(a === 6 && b === 6 && c === 6)
}
f14(6)

function f15(a = (() => b)(), b)
{
  assert(a === 1 && b === 2)
}
f15(1, 2)
check_reference_error("f15(undefined, 2)");

var f16 = (a = a) =>
{
  assert(a === 1)
}
f16(1)
check_reference_error("f16()");

var f17 = ([[a]] = a) =>
{
  assert(a === 2)
}
f17([[2]])
check_reference_error("f17()");

var f18 = ({a = b, b:{b}}) =>
{
  assert(a === 3 && b === 4)
}
f18({a:3, b:{b:4}})
check_reference_error("f18({b:{b:4}})");

var f19 = (a = eval("a")) =>
{
  assert(a === 5)
}
f19(5)
check_reference_error("f19()");

var f20 = ([a] = eval("a")) =>
{
  assert(a === 6)
}
f20([6])
check_reference_error("f20()");
