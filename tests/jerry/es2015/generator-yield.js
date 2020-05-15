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

/* This file checks yield syntax errors. */

function check_syntax_error(str)
{
  try {
    eval(str);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}

function  *  gen()
{
  yield , yield

  yield
     , yield

  (yield
  )

  yield[
  1]
}

function*gen2()
{
  1 ?
    yield
  :
    yield
}

var gen3 = function*(){
  (yield)/[yield]
}

check_syntax_error("function *gen(){ yield % yield }");
check_syntax_error("function *gen(){ (yield) % yield }");
check_syntax_error("function *gen(){ yield % (yield) }");
check_syntax_error("function *gen(){ (yield\n1) }");
check_syntax_error("function *gen(){ function yield() {} }");
check_syntax_error("function *gen(){ (yield)=>1 }");
check_syntax_error("function *gen(){ yield => 1 }");
check_syntax_error("function *gen(){ yi\\u0065ld 1 }");

function *gen4() {
  var f = function yield(i) {
    if (i = 0)
      return yield(i + 1)

    return 39
  }

  return f(0)
}

assert(gen4().next().value === 39);

(function *() {
  () => yield
  yield 1;
})

function *gen5() {
  var o = {
    ["f"]() { yield % yield }
  }
  yield 1;
}
