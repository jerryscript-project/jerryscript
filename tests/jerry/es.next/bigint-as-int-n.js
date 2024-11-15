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

assert(BigInt.asIntN('1', 1n) === -1n)
assert(BigInt.asIntN(1.1, 1n) === -1n)
assert(BigInt.asIntN(2, 2n**2n) === 0n)
assert(BigInt.asIntN(undefined, 1n) === 0n)

var array = [1, 1n, -1n]

assert(BigInt.asIntN(array[0], 1n) == -1n)
assert(BigInt.asIntN(array[0], array[1]) == -1n)
assert(BigInt.asIntN(array[0], array[2]) == -1n)

function f(param1, param2) {
  assert(BigInt.asIntN(param1, param2) === 0n)
}
f(0,10n)
f(10,0n)

try {
  assert(f(1, "1n") === 1n)
  assert(false)
} catch (e) {
  assert(e instanceof SyntaxError)
}

try {
  assert(BigInt.asIntN(Number.POSITIVE_INFINITY, 1n) === 1n)
  assert(false)
} catch (e) {
  assert(e instanceof RangeError)
}

try {
  assert(BigInt.asIntN(Number.NEGATIVE_INFINITY, 1n) === 1n)
  assert(false)
} catch (e) {
  assert(e instanceof RangeError)
}

try {
  assert(BigInt.asIntN(1, Number.POSITIVE_INFINITY) === 1n)
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

try {
  assert(BigInt.asIntN(1, Number.NEGATIVE_INFINITY) === 1n)
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

try {
  assert(BigInt.asIntN(1, 1 + 1n) === 1n)
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

try {
  assert(BigInt.asIntN(2n, 2n**2n) === 0n)
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

try {
  BigInt.asIntN().call(undefined,1n)
  assert (false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  BigInt.asIntN().call(1,undefined)
  assert (false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  BigInt.asIntN().call(undefined,undefined)
  assert (false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  assert(BigInt.asIntN(Infinity, 2n) === RangeError)
  assert(false)
} catch (e) {
  assert(e instanceof RangeError)
}
