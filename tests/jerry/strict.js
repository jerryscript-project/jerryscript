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

'use strict';

var temp;

try
{
  a = 1;

  assert (false);
} catch (e)
{
  assert (e instanceof ReferenceError);
}

try
{
  NaN = 1;

  assert (false);
} catch (e)
{
  assert (e instanceof TypeError);
}

function f()
{
  assert(this === undefined);
}

f();

Object.function_prop = function ()
{
  assert (this === Object);
}

Object.function_prop ();

try
{
  var temp = f.caller;

  assert (false);
} catch (e)
{
  assert (e instanceof TypeError);
}

try
{
  delete this.NaN;

  assert (false);
} catch (e)
{
  assert (e instanceof TypeError);
}

try
{
  eval ("'\\" + "101'");

  assert (false);
} catch (e)
{
  assert (e instanceof SyntaxError);
}

try
{
  var str1 = "'\\" + "0'";
  var str2 = "'\\x" + "00'";
  eval (str1);

  assert (eval (str1) === eval (str2));
} catch (e)
{
  assert (false);
}

try
{
  var str1 = "'\\" + "0" + "\\" + "0" + "\\" + "0'";
  var str2 = "'\\x" + "00" + "\\x" + "00" + "\\x" + "00'";
  eval (str1);

  assert (eval (str1) === eval (str2));
} catch (e)
{
  assert (false);
}

try
{
  var str1 = "'foo\\" + "0" + "bar'";
  var str2 = "'foo\\x" + "00" + "bar'";
  eval (str1);

  assert (eval (str1) === eval (str2));
} catch (e)
{
  assert (false);
}

(function (a) {
  (function (a) {
  });
});
