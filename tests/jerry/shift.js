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

assert((9 << 2) === 36);
assert((14 << 2) === 56);
assert((0 << 0) === 0);
assert((0 << 1) === 0);
assert((0 << -1) === 0);
assert((1 << 0) === 1);
assert((1 << 1) === 2);
assert((-1 << 0) === -1);
assert((-1 << 1) === -2);
assert((1024 << 21) === -2147483648);
assert((10 << -1) === 0);
assert((33554431 << 22) === -4194304);
assert((-1024 << 31) === 0);
assert((1024 << 53) === -2147483648);

assert((9 >> 2) === 2);
assert((-14 >> 2) === -4);

assert((9 >>> 2) === (9 >> 2));

assert(("-1.234" << 0) == -1);
assert(("-1.234" << -1) == -2147483648);
assert((-1.2 << 0) === -1);
assert((-1.1 << -1) === -2147483648);
assert((-1.1 << -1.1) === -2147483648);
assert((-1.1 << -999) === -33554432);
assert((-1.1 << 999) === -128);
assert((-33554431.0 << 0) === -33554431);
assert((-33554431.0 << -1.1) === -2147483648);
assert((-33554431.0 << -999) === 33554432);
