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

assert(isNaN(Math['exp'] (NaN)));
assert(Math['exp'] (0.0) === 1.0);
assert(Math['exp'] (Infinity) === Infinity);
assert(Math['exp'] (-Infinity) === 0.0);

assert(Math['exp'] (1) >= 0.999999 * Math['E']);
assert(Math['exp'] (1) <= 1.000001 * Math['E']);

assert(Math['exp'] (-1) >= 0.999999 * (1 / Math['E']));
assert(Math['exp'] (-1) <= 1.000001 * (1 / Math['E']));

assert(Math['exp'] (0.5) >= 0.999999 * 1.6487212707);
assert(Math['exp'] (0.5) <= 1.000001 * 1.6487212707);

assert(Math['exp'] (30) >= 0.999999 * 1.06864745815e+13);
assert(Math['exp'] (30) <= 1.000001 * 1.06864745815e+13);
