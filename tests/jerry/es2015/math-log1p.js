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

assert(isNaN(Math.log1p(NaN)));
assert(isNaN(Math.log1p(-42)));
assert(isNaN(Math.log1p(-3.0)));
assert(isSameZero(Math.log1p(n_zero), n_zero));
assert(isSameZero(Math.log1p(p_zero), p_zero));
assert(Math.log1p(-1) === Number.NEGATIVE_INFINITY);
assert(Math.log1p(Number.POSITIVE_INFINITY) === Number.POSITIVE_INFINITY);
assert(Math.log1p(Math.E - 1) === 1);
