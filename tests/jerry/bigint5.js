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
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

check_syntax_error("1N")
check_syntax_error("3.5n")
check_syntax_error("3e10n")
check_syntax_error("3e+10n")
check_syntax_error("0xn")
check_syntax_error("0on")
check_syntax_error("0bn")
check_syntax_error("0777n")
check_syntax_error("00777n")
check_syntax_error("0x1 n")

assert(0n == 0n)
assert(0n == -0n)
assert(12n == 12n)
assert(123456789012345678901234567890123456789012345678901234567890n == 123456789012345678901234567890123456789012345678901234567890n)
assert(12n != -12n)
assert(123456789012345678901234567890123456789012345678901234567890n != -123456789012345678901234567890123456789012345678901234567890n)

assert(0xffn == 255n)
assert(0o77777n == 0x7fffn)
assert(255n.toString(16) == "ff")

var o = { 12n : "data" }
assert(o[12] === "data")

var c = class C { static 19n () { return "BigInt" } }
assert(c[19]() === "BigInt")

function f(p, q) {
  assert(p + q === 5000n)
}
f(-1000n, 6000n)
