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

assert(isNaN (Math['min'] (1.0, NaN)));
assert(isNaN (Math['min'] (NaN, 1.0)));
assert(isNaN (Math['min'] (-Infinity, NaN)));
assert(isNaN (Math['min'] (NaN, -Infinity)));
assert(Math['min'] (1.0, 3.0, 0.0) === 0.0);
assert(Math['min'] (1.0, 3.0, Infinity) === 1.0);
assert(Math['min'] (1.0, 3.0, -Infinity) === -Infinity);
assert(Math['min'] (-Infinity, Infinity) === -Infinity);
assert(Math['min'] (Infinity, -Infinity) === -Infinity);
assert(Math['min'] (Infinity, Infinity) === Infinity);
assert(Math['min'] (-Infinity, -Infinity) === -Infinity);
assert(Math['min'] () === Infinity);

assert(Math['min'] (0.0, -0.0) === -0.0);
assert(Math['min'] (-0.0, 0.0) === -0.0);

assert(Math['min'] (2, -Infinity) === -Infinity);
assert(Math['min'] (-Infinity, 2) === -Infinity);
assert(Math['min'] (2, Infinity) === 2);
assert(Math['min'] (Infinity, 2) === 2);

assert(Math['min'] (-2, Infinity) === -2);
assert(Math['min'] (Infinity, -2) === -2);
assert(Math['min'] (-2, -Infinity) === -Infinity);
assert(Math['min'] (-Infinity, -2) === -Infinity);
