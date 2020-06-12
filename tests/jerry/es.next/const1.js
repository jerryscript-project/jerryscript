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

const a = 6
assert(!delete a)
assert(a === 6)

try {
  eval("a = 7")
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

function check_type_error(code) {
  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof TypeError)
  }
}

// Register cases
check_type_error("{ const a = 0; a = 1 }");
check_type_error("{ const a = 0; a += 1 }");
check_type_error("{ const a = 0; a -= 1 }");
check_type_error("{ const a = 0; a *= 1 }");
check_type_error("{ const a = 0; a %= 1 }");
check_type_error("{ const a = 0; a /= 1 }");
check_type_error("{ const a = 0; a <<= 1 }");
check_type_error("{ const a = 0; a >>= 1 }");
check_type_error("{ const a = 0; a >>>= 1 }");
check_type_error("{ const a = 0; a &= 1 }");
check_type_error("{ const a = 0; a |= 1 }");
check_type_error("{ const a = 0; a ^= 1 }");
check_type_error("{ const a = 0; a++ }");
check_type_error("{ const a = 0; a-- }");
check_type_error("{ const a = 0; ++a }");
check_type_error("{ const a = 0; --a }");
check_type_error("{ const a = 0; [a] = [1] }");
check_type_error("{ const a = 0; ({a} = { a:1 }) }");

// Non-register cases
check_type_error("{ const a = 0; (function (){ a = 1 })() }");
check_type_error("{ const a = 0; (function (){ a += 1 })() }");
check_type_error("{ const a = 0; (function (){ a -= 1 })() }");
check_type_error("{ const a = 0; (function (){ a *= 1 })() }");
check_type_error("{ const a = 0; (function (){ a %= 1 })() }");
check_type_error("{ const a = 0; (function (){ a /= 1 })() }");
check_type_error("{ const a = 0; (function (){ a <<= 1 })() }");
check_type_error("{ const a = 0; (function (){ a >>= 1 })() }");
check_type_error("{ const a = 0; (function (){ a >>>= 1 })() }");
check_type_error("{ const a = 0; (function (){ a &= 1 })() }");
check_type_error("{ const a = 0; (function (){ a |= 1 })() }");
check_type_error("{ const a = 0; (function (){ a ^= 1 })() }");
check_type_error("{ const a = 0; (function (){ a++ })() }");
check_type_error("{ const a = 0; (function (){ a-- })() }");
check_type_error("{ const a = 0; (function (){ ++a })() }");
check_type_error("{ const a = 0; (function (){ --a })() }");
check_type_error("{ const a = 0; (function (){ [a] = [1] })() }");
check_type_error("{ const a = 0; (function (){ ({a} = { a:1 }) })() }");
