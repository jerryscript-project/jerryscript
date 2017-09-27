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

var a = 'A';
var b = 'B';

switch (1)
{
default:

  ``
  `abc`
  `ab${a+b}${ `x` }c`

  assert (`` === '');
  assert (`abc` === 'abc');
  assert (`ab\
  c` === 'ab  c');
  assert (`ab
  c` === 'ab\n  c');

  assert (`prefix${a}` === 'prefixA');
  assert (`${a}postfix` === 'Apostfix');
  assert (`prefix${a}postfix` === 'prefixApostfix');

  assert (`${a}${b}` === 'AB');
  assert (`${a},${b}` === 'A,B');
  assert (`${a}${b}${a}${b}` === 'ABAB');
  assert (`${a},${b},${a},${b}` === 'A,B,A,B');
  assert (`$${a},${b},${a},${b}$` === '$A,B,A,B$');

  assert (`\${}` === '${}');
  assert (`$\{}` === '${}');
  assert (`x${  `y` + `z`  }x` === 'xyzx');
  assert (`x${  `y` , `z`  }x` === 'xzx');

  function f(x) { return x + 1; }

  /* Precedence. */
  var c = 1;
  assert (`x${  f(1) * f(2)  }x${ c = 4 }` === 'x6x4');
  assert (c === 4);
  assert (`m${0 || 93}n${7 && 0}o` === 'm93n0o');

  /* Result is always a string. */
  assert (`${  function() { return true } () }` === 'true');
  assert (`${  function() { return a.length } () }` === '1');

  /* Result is a single string with its properties. */
  assert(`${a}${b}${a}${b}`.length === 4);
}

must_throw ("`");
must_throw ("`${");
must_throw ("`${7");
must_throw ("`${}`");
must_throw ("`${1}");
must_throw ("`${1}.${");
must_throw ("`${1}.${2}");
