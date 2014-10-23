// Copyright 2014 Samsung Electronics Co., Ltd.
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

assert(isNaN(Math['sqrt'] (NaN)));
assert(isNaN(Math['sqrt'] (-1.0)));
assert(isNaN(Math['sqrt'] (-Infinity)));
assert(Math['sqrt'] (0.0) === 0.0);
assert(Math['sqrt'] (Infinity) === Infinity);

assert(Math['sqrt'] (1.0) === 1.0);
assert(Math['sqrt'] (2.0) >= Math['SQRT2'] * 0.999999);
assert(Math['sqrt'] (2.0) <= Math['SQRT2'] * 1.000001);
assert(Math['sqrt'] (0.5) >= Math['SQRT1_2'] * 0.999999);
assert(Math['sqrt'] (0.5) <= Math['SQRT1_2'] * 1.000001);

var sqrt_1e38 = Math['sqrt'] (1.0e+38);
assert(sqrt_1e38 > 0.999999 * 1.0e+19);
assert(sqrt_1e38 < 1.000001 * 1.0e+19);

var sqrt_1e38 = Math['sqrt'] (1.0e-38);
assert(sqrt_1e38 > 0.999999 * 1.0e-19);
assert(sqrt_1e38 < 1.000001 * 1.0e-19);
