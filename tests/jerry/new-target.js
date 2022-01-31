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

function null_target () {
  assert (new.target === undefined);
}

function demo () {
  null_target ();
  return new.target;
}

assert (demo () === undefined);
assert ((new demo ()) === demo);

/* new.target is only valid inside functions */
try {
  eval ("new.target");
  assert (false);
} catch (ex) {
  assert (ex instanceof SyntaxError);
}

try {
  var eval_other = eval;
  eval_other ("new.target");
  assert (false);
} catch (ex) {
  assert (ex instanceof SyntaxError);
}

/* test with arrow function */
var arrow_called = false;
function arrow () {
    assert (new.target === arrow);
    var mth = () => { return new.target; }
    assert (mth () === arrow);
    arrow_called = true;
}

new arrow ();
assert (arrow_called === true);

/* test unary operation */
var f_called = false;
function f () {
  assert (isNaN (-new.target));
  f_called = true;
}

new f ();
assert (f_called === true);

/* test property access */
function fg (callback_object) {
  callback_object.value = new.target.value;
}

fg.value = 22;

var test_obj = {};
new fg (test_obj);

assert (test_obj.value === 22);


/* test new.target with eval */
function eval_test () {
  var target = eval ("new.target");
  assert (target === eval_test);
}

new eval_test ();

function eval_eval_test () {
  var target = eval ('eval("new.target")');
  assert (target === eval_eval_test);
}

new eval_eval_test ();

/* new.target is only valid in direct eval */
function eval_test_2 () {
  var ev = eval;
  try {
    ev ("new.target");
    assert (false);
  } catch (ex) {
    assert (ex instanceof SyntaxError);
  }
}

new eval_test_2 ();

function eval_test_3 () {
  var ev = eval;
  try {
    eval ("ev ('new.target')");
    assert (false);
  } catch (ex) {
    assert (ex instanceof SyntaxError);
  }
}

new eval_test_3 ();

/* test assignment of the "new.target" */
function expect_syntax_error (src)
{
  try {
    eval (src);
    assert (false);
  } catch (ex) {
    assert (ex instanceof SyntaxError);
  }
}

expect_syntax_error ("function assign () { new.target = 3; }");
expect_syntax_error ("function assign () { new.target += 3; }");
expect_syntax_error ("function assign () { new.target *= 3; }");
expect_syntax_error ("function assign () { new.target -= 3; }");
expect_syntax_error ("function assign () { new.target |= 3; }");
expect_syntax_error ("function assign () { new.target &= 3; }");

expect_syntax_error ("function assign () { new.target++; }");
expect_syntax_error ("function assign () { ++new.target; }");
expect_syntax_error ("function assign () { new.target--; }");
expect_syntax_error ("function assign () { --new.target; }");

expect_syntax_error ("function synt () { new....target; }");

function delete_test () {
  assert ((delete new.target) === true);
}

new delete_test ();

function binary_test_1 () {
    /*/ new.target is converted to string */
    var str = (new.target + 1);
    assert (str.substring(0, 8) === "function"
            && str.substring(str.length - 2, str.length) === "}1");
}
function binary_test_2 () { assert (isNaN (new.target - 3)); }
function binary_test_3 () { assert (isNaN (new.target * 2)); }
function binary_test_4 () { assert (isNaN (new.target / 4)); }

new binary_test_1 ();
new binary_test_2 ();
new binary_test_3 ();
new binary_test_4 ();
