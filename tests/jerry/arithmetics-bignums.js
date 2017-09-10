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

var big = 2147483646;

big++;
assert(big == 2147483647);

big += 1;
assert(big == 2147483648); // overflow on 32bit numbers

big++;
assert(big == 2147483649); // overflow on 32bit numbers

assert ((1152921504606846976).toString() === "1152921504606847000")

assert (1.797693134862315808e+308 === Infinity);
