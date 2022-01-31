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

/* Shifting a negative bigint value with a big number to the right should be -1n. */
assert((-1n >> (2n ** 32n)) === -1n);
assert((-1n >> 2n) === -1n);
assert((-1n >> (2n ** 1n)) === -1n);
assert((-5n >> 3n) === -1n);
assert((-5n >> 4n) === -1n);
assert((-(2n ** 32n) >> (2n ** 31n)) === -1n);
assert((-(2n ** 32n) >> (2n ** 32n)) === -1n);
assert((-(2n ** 32n) >> (2n ** 33n)) === -1n);
assert((-(2n ** 32n) >> (2n ** 33000n)) === -1n);
assert((-(2n ** 32n) >> 32n) === -1n);

/* (xn << -yn) ==> (xn >> yn) */
assert((-1n << -(2n ** 32n)) === -1n);
assert((-1n << -2n) === -1n);
assert((-1n << -(2n ** 1n)) === -1n);
assert((-5n << -3n) === -1n);
assert((-5n << -4n) === -1n);
assert((-(2n ** 32n) << -(2n ** 31n)) === -1n);
assert((-(2n ** 32n) << -(2n ** 32n)) === -1n);
assert((-(2n ** 32n) << -(2n ** 33n)) === -1n);
assert((-(2n ** 32n) << -(2n ** 33000n)) === -1n);

/* Partialy related tests. */
assert((-(2n ** 32n) >> 31n) === -2n);
assert((-(2n ** 32n) >> 30n) === -4n);
assert((-(2n ** 32n) >> 29n) === -8n);
assert((-(2n ** 32n) >> 28n) === -16n);
assert((-(2n ** 32n) >> 16n) === -65536n);
assert((-(2n ** 32n) >> 2n) === -1073741824n);

assert((-5n >> 2n) === -2n);
assert((-5n >> -4n) === -80n);
