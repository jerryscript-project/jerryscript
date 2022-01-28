/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function must_throw (str)
{
  try
  {
    eval ("switch (1) { default: " + str + "}");
    assert (false);
  }
  catch (e)
  {
  }

  try
  {
    eval (str);
    assert (false);
  }
  catch (e)
  {
  }
}

function must_throw_strict (str)
{
  try
  {
    eval ("'use strict'; switch (1) { default: " + str + "}");
    assert (false);
  }
  catch (e)
  {
  }

  try
  {
    eval ("'use strict'; " + str);
    assert (false);
  }
  catch (e)
  {
  }
}

switch (1)
{
default:

  var func = x => { return x + 3 }
  assert (func(5) == 8);

  a => 5 /* no semicolon after */

  assert (((x =>
    x + 1))(4) == 5)

  assert ((a => a += 3, b => b -= 3)(4) == 1);

  func = true ? x=>x+2:y=>y-2
  assert (func(10) == 12);

  func = arguments =>
    { return arguments + 4; }
  assert (func(2) == 6);

  func = (
          ) => { return typeof
    arguments
  }
  assert (func() === "undefined");

  if (a => 0)
  {
  }
  else
  {
    assert (false);
  }

  assert ((
    (
    static
    ,
    package
    ) => static + package
  ) (2, 12) == 14);

  var global_var = 7;

  assert ((
    (
    static
    ,
    package
    ) => { global_var = 5; return static + package }
  )(4, 5) == 9);

  assert (global_var == 5);

  func = (x , y) => {}
  assert (func() === undefined)

  assert ((x => y => z => 6)()()() == 6)

  func = x => x - 6
  var func2 = y => func(y)
  assert (func2 (17) == 11)

  func = (m) => m++
  assert (func (4) == 4)

  func = () =>
    ((([0,0,0])))
  assert (func ().length == 3);

  func = (a = 5, b = 7 * 2) => a + b;
  assert (func() == 19);
  assert (func(1) == 15);

  func = (a = Math.cos(0)) => a;
  assert (func() == 1);
}

must_throw ("var x => x;");
must_throw ("(()) => 0");
must_throw ("((x)) => 0");
must_throw ("(((x))) => 0");
must_throw ("(x==6) => 0");
must_throw ("(x y) => 0");
must_throw ("x\n => 0");
must_throw ("this => 0");
must_throw ("(true) => 0");
must_throw ("()\n=>5");
must_throw ("3 + x => 3");
must_throw ("3 || x => 3");
must_throw ("a = 3 || (x,y) => 3");
must_throw ("x => {} (4)");
must_throw ("!x => 4");
must_throw ("x => {} = 1");
must_throw ("x => {} a = 1");
must_throw ("x => {} ? 1 : 0");
must_throw ("(x,x,x) => 0");
must_throw ("(x,x,x) => { }");
must_throw_strict ("(package) => 0");
must_throw_strict ("(package) => { return 5 }");
must_throw_strict ("(x,x,x) => 0");
must_throw_strict ("(x,x,x) => { }");

var f = (a) => 1;
assert(f() === 1);

var f = (a => 2);
assert(f() === 2);

var f = ((((a => ((3))))));
assert(f() === 3);

var f = (((a) => 4));
assert(f() === 4);

var f = (a,b) => 5;
assert(f() === 5);

var f = (((a,b) => 6));
assert(f() === 6);

var f = ((a,b) => x => (a) => 7);
assert(f()()() === 7);

var f = (((a=1,b=2) => ((x => (((a) => 8))))));
assert(f()()() === 8);

var f = () => {};

assert(f.hasOwnProperty('caller') === false);
assert(f.hasOwnProperty('arguments') === false);

must_throw("var f = () => {}; f.caller")
must_throw("var f = () => {}; f.arguments")
must_throw("var f = () => {}; f.caller = 1")
must_throw("var f = () => {}; f.arguments = 2")
