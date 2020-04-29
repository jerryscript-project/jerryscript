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

function check_syntax_error(code)
{
  try {
    eval(code);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}

check_syntax_error("try {} catch() {}");
check_syntax_error("try {} catch([a] {}");
check_syntax_error("try {} catch([a] = [1]) {}");
check_syntax_error("try {} catch({a} = {a:1}) {}");
check_syntax_error("try {} catch(a,) {}");
check_syntax_error("try {} catch(a) { function a() {} }");
check_syntax_error("try {} catch(a) { { function a() {} } function a() {} }");
check_syntax_error("try {} catch([a]) { var a }");
check_syntax_error("try {} catch([a]) { { var a } }");
check_syntax_error("try {} catch([a]) { function a() {} }");

try {
  throw [1,2]
  assert(false)
} catch ([a, b]) {
  assert(a === 1)
  assert(b === 2)
}

try {
  throw { x:1, y:2 }
  assert(false)
} catch ({x, 'y':b}) {
  assert(x === 1)
  assert(b === 2)
}

try {
  throw [1,2]
  assert(false)
} catch ([a, b]) {
  eval("assert(a === 1)")
  eval("assert(b === 2)")
}

try {
  throw { x:1, y:2 }
  assert(false)
} catch ({x, 'y':b}) {
  eval("assert(x === 1)")
  eval("assert(b === 2)")
}

try {
  try {
    throw []
    assert(false)
  } catch ([a = b, b]) {
    assert(false)
  }
} catch (e) {
  assert(e instanceof ReferenceError)
}

try {
  throw [{a : 5}];
} catch([{a}]) {
  assert(a === 5);
}

var catchReached = false;
try {
  throw [{}];
  assert(false);
} catch([{}]) {
  catchReached = true;
}

assert(catchReached);
