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

assert (isNaN(Math['trunc'](NaN)));
assert (Math['trunc'](p_zero) === p_zero);
assert (Math['trunc'](m_zero) === m_zero);
assert (Math['trunc'](p_inf) === p_inf);
assert (Math['trunc'](m_inf) === m_inf);
assert (Math['trunc'](0.5) === p_zero);
assert (Math['trunc'](-0.5) === m_zero);
assert (Math['trunc'](1.2) === 1);
assert (Math['trunc'](-1.5) === -1);
assert (Math['trunc'](65.7) === 65);
assert (Math['trunc'](-24.9) === -24);
