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

function f_arg (arguments)
{
  return arguments;
}
assert (f_arg (1) === 1);

function f (a, b, c)
{
  return arguments;
}

args = f();
assert (args[0] === undefined);

args = f (1, 2, 3, 4, 5);
assert (args[0] === 1);
assert (args[1] === 2);
assert (args[2] === 3);
assert (args[3] === 4);
assert (args[4] === 5);
assert (args[5] === undefined);

assert (args.callee === f);
assert (typeof args.caller === 'undefined');

function g (a, b, c)
{
  assert (arguments[0] === 1);
  assert (arguments[1] === undefined);
  assert (arguments[2] === undefined);

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 'a');
  assert (arguments[1] === 'b');
  assert (arguments[2] === 'c');

  arguments [0] = 1;
  arguments [1] = 2;
  arguments [2] = 3;

  assert (a === 1);
  assert (b === 2);
  assert (c === 3);

  delete arguments [0];
  arguments[0] = 'new value';
  assert (a === 1);

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 'new value');
  assert (arguments[1] === 'b');
  assert (arguments[2] === 'c');
}

g (1);

fn_expr = function (a, b, c)
{
  'use strict';

  assert (arguments[0] === 1);
  assert (arguments[1] === undefined);
  assert (arguments[2] === undefined);

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 1);
  assert (arguments[1] === undefined);
  assert (arguments[2] === undefined);

  arguments [0] = 1;
  arguments [1] = 'p';
  arguments [2] = 'q';

  assert (a === 'a');
  assert (b === 'b');
  assert (c === 'c');

  delete arguments [0];
  arguments[0] = 'new value';
  assert (a === 'a');

  a = 'a';
  b = 'b';
  c = 'c';

  assert (arguments[0] === 'new value');
  assert (arguments[1] === 'p');
  assert (arguments[2] === 'q');

  function check_type_error_for_property (obj, prop) {
    try {
      var v = obj[prop];
      assert (false);
    }
    catch (e) {
      assert (e instanceof TypeError);
    }
  }

  check_type_error_for_property (arguments, 'caller');
  check_type_error_for_property (arguments, 'callee');
}

fn_expr (1);

(function () {
 var a = [arguments];
})();
