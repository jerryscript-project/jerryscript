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

assert (eval () === undefined);
assert (eval (undefined) === undefined);
assert (eval (null) === null);
assert (eval (true) === true);
assert (eval (false) === false);
assert (eval (1) === 1);
assert (eval (eval) === eval);

/* Indirect eval */
function f1()
{
 var v1 = 'local value';

 assert (v1 === 'local value');
 assert (typeof (this.v1) === 'undefined');

 r = this.eval ('var v1 = "global value";');

 assert (v1 === 'local value');
 assert (this.v1 === 'global value');
 assert (r === undefined);
};

f1 ();

/* Direct eval from strict mode code */
function f2 (global)
{
 'use strict';
 var v2 = 'local value';

 assert (v2 === 'local value');
 assert (typeof (global.v2) === 'undefined');

 r = eval ('var v2 = "global value";');

 assert (v2 === 'local value');
 assert (typeof (global.v2) === 'undefined');
 assert (r === undefined);

 try
 {
   eval ('arguments = 1;');
   assert (false);
 }
 catch (e)
 {
   assert (e instanceof SyntaxError);
 }
}

f2 (this);

var y;

for (var i = 0; i < 100; i++)
{
  var r = eval ('var x =' + ' 1;');
  assert (typeof (x) === 'number');
  assert (r === undefined);

  delete x;
  assert (typeof (x) === 'undefined');

  r = eval ('"use ' + 's' + 't' + 'r' + 'i' + 'c' + 't"; va' + 'r x = 1;');
  assert (typeof (x) === 'undefined');
  assert (r === "use strict");

  y = 'str';
  assert (typeof (y) === 'string');

  delete y;
  assert (typeof (y) === 'string');

  r = eval ('var y = "another ' + 'string";');
  assert (y === 'another string');
  assert (r == undefined);

  delete y;
  assert (typeof (y) === 'string');

  r = eval ('if (true) 3; else 5;');
  assert (r === 3);
}

// Check SyntaxError handling
try
{
  eval ('var var;');
  assert (false);
}
catch (e)
{
  assert (e instanceof SyntaxError);
}

try
{
  eval ("v_0 = {a: Math, /[]/};");
  assert (false);
}
catch(e)
{
  assert (e instanceof SyntaxError);
}

// nested eval with function expressions
code = 'eval("(function (){})")';
code = "eval ('" + code + "')";
eval (code);
