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

function f() {
    return 'foo';
}
assert ((function() {
    if (1 === 0) {
        function f() {
            return 'bar';
        }
    }
    return f();
})() === 'bar');

function check_syntax_error (s) {
  try {
    eval (s);
    assert (false);
  }
  catch (e) {
    assert (e instanceof SyntaxError);
  }
}

check_syntax_error ("'use strict'; function arguments () {}");
check_syntax_error ("'use strict'; var l = function arguments () {}");

check_syntax_error ("function f__strict_mode_duplicate_parameters (p, p) { 'use strict'; }");

function test_strict_mode_propagation_in_func_expr_and_getters_setters () {
  var p = function () {
    'use strict';

    return true;
  }

  var o = { get prop () { 'use strict'; return true; }, set prop (v) { 'use strict'; } };

  function test () {
    tmp_eval = eval;
    eval = tmp_eval;
  }
}
