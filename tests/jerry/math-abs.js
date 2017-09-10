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
var p_inf = Infinity;
var m_inf = -p_inf;

assert (isNaN(Math['abs'](NaN)));
assert (Math['abs'](p_zero) === p_zero);
assert (Math['abs'](m_zero) === p_zero);
assert (Math['abs'](p_inf) === p_inf);
assert (Math['abs'](m_inf) === p_inf);

assert (Math['abs'](0.5) === 0.5);
assert (Math['abs'](-0.5) === 0.5);
assert (Math['abs'](1.2) === 1.2);
assert (Math['abs'](1.5) === 1.5);
assert (Math['abs'](0.7) === 0.7);
assert (Math['abs'](0.2) === 0.2);
assert (Math['abs'](-0.2) === 0.2);
assert (Math['abs'](-0.7) === 0.7);
assert (Math['abs'](-1.2) === 1.2);
assert (Math['abs'](-1.7) === 1.7);
assert (Math['abs'](-1.5) === 1.5);
