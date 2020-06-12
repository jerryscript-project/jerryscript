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

var nan = NaN;
var p_zero = 0.0;
var m_zero = -p_zero;

function isSameZero (x, y)
{
  return x === 0 && (1 / x) === (1 / y);
}

assert(isNaN(Math.acosh(NaN)));
assert(isNaN(Math.acosh(0)));
assert(isNaN(Math.acosh(Number.NEGATIVE_INFINITY)));
assert(isSameZero(Math.acosh(1), p_zero));
assert(Math.acosh(Number.POSITIVE_INFINITY) === Number.POSITIVE_INFINITY);

