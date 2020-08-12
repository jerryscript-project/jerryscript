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

assert(Number(0n) === 0)
assert(Number(1n) === 1)
assert(Number(2n) === 2)
assert(Number(0x100n) === 256)
assert(Number(-1n) === -1)
assert(Number(-2n) === -2)
assert(Number(-0x100n) === -256)

assert(Number(0x1fffffffffffffn) === 0x1fffffffffffff)
assert(Number(-0x3fffffffffffffn) === -0x40000000000000)
assert(Number(1000n ** 1000n) === Number.POSITIVE_INFINITY)
assert(Number((-1000n) ** 1001n) === Number.NEGATIVE_INFINITY)

// Rounding to even

assert(Number(0x80000000000004n) === 0x80000000000000)
assert(Number(0x80000000000008n) === 0x80000000000008)
assert(Number(0x8000000000000cn) === 0x80000000000010)
assert(Number(0x80000000000004n) === 0x80000000000000)
assert(Number(0x80000000000006n) === 0x80000000000008)
assert(Number(0x800000000000f400000000000000000000000000000000n) === 0x800000000000f000000000000000000000000000000000)
assert(Number(0x800000000000f400000000000000000000000000000001n) === 0x800000000000f800000000000000000000000000000000)

// Construct

assert((new Number(0n)).valueOf() === 0)
assert((new Number(-3256n)).valueOf() == -3256)
