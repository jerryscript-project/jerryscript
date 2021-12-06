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

var p_zero = 0.0;
var n_zero = -p_zero;

function isSameZero (x, y)
{
  return x === 0 && (1 / x) === (1 / y);
}

assert(isNaN(Math.expm1(NaN)));
assert(isSameZero(Math.expm1(p_zero), p_zero));
assert(isSameZero(Math.expm1(n_zero), n_zero));
assert(Math.expm1(Number.POSITIVE_INFINITY) === Number.POSITIVE_INFINITY);
assert(Math.expm1(Number.NEGATIVE_INFINITY) === -1);
assert(1/Math.expm1(-0) === Number.NEGATIVE_INFINITY)
assert(1/Math.expm1(0) === Number.POSITIVE_INFINITY)
assert(Math.expm1(1) <= 1.00000001 * (Math.E - 1));
assert(Math.expm1(1) >= 0.99999999 * (Math.E - 1));
