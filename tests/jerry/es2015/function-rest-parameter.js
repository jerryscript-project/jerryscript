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

function CheckSyntaxError (str)
{
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }

  /* force the pre-scanner */
  try {
    eval ('switch (1) { default: ' + str + '}');
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

CheckSyntaxError ('function x (a, b, ...c, d) {}');
CheckSyntaxError ('function x (... c = 5) {}');
CheckSyntaxError ('function x (...) {}');
CheckSyntaxError ('function x (a, a, ...a) {}');
CheckSyntaxError ('"use strict" function x (...arguments) {}');

rest_params = ['hello', true, 7, {}, [], function () {}];

function f (x, y, ...a) {
  for (var i = 0; i < a.length; i++) {
    assert (a[i] == rest_params[i]);
  }
  return (x + y) * a.length;
}

assert (f (1, 2, rest_params[0], rest_params[1], rest_params[2]) === 9);
assert (f.length === 2);

function g (...a) {
  return a.reduce (function (accumulator, currentValue) { return accumulator + currentValue });
}

assert (g (1, 2, 3, 4) === 10);

function h (...arguments) {
  return arguments.length;
}

assert (h (1, 2, 3, 4) === 4);

function f2 (a = 1, b = 1, c = 1, ...d) {
  assert (JSON.stringify (d) === '[]');
  return a + b + c;
}

assert (f2 () === 3);
assert (f2 (2) === 4);
assert (f2 (2, 3) === 6);
assert (f2 (2, 3, 4) === 9);

function g2 (a = 5, b = a + 1, ...c) {
  return a + b + c.length;
}

assert (g2 () === 11);
assert (g2 (1) === 3);
assert (g2 (1, 2) === 3);
assert (g2 (1, 2, 3) === 4);
