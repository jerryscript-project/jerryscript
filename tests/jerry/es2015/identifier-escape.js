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

function check_syntax_error (code) {
  try {
    eval(code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

eval("\u{000010C80}: break \ud803\udc80")
eval("\\u{10C80}: break \ud803\udc80")
eval("$\u{000010C80}$: break $\ud803\udc80$")
eval("$\\u{10C82}$: break $\ud803\udc82$")

assert("\u{000010C80}".length === 2)
assert("x\u{010C80}y".length === 4)
assert("\u{10C80}" === "\ud803\u{dc80}")
assert("\u{0}\x01" === "\u0000\u0001")

/* Surrogate pairs are not combined if they passed as \u sequences. */
check_syntax_error("\\u{10C80}: break \\ud803\\udc80");
