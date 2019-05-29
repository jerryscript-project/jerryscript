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

assert ((1152921504606846976).toString() === "1152921504606847000");

assert (1.797693134862315808e+308 === Infinity);

assert (9999999999999999 == 10000000000000000);

assert((9007199254740993).toString() === "9007199254740992");

assert((9007199254740992).toString() === "9007199254740992");

assert((9007199254740994).toString() === "9007199254740994");

assert((1.00517e+21).toString() === "1.0051699999999999e+21");

assert((1.00001e+21).toString() === "1.0000099999999999e+21");

assert((9007199254740995).toString() === "9007199254740996");

assert((18014398509481989).toString() === "18014398509481988");

assert((18014398509481990).toString() === "18014398509481992");

assert((18014398509481991).toString() === "18014398509481992");

assert((18014398509481993).toString() === "18014398509481992");

assert((18014398509481994).toString() === "18014398509481992");

assert((18014398509481997).toString() === "18014398509481996");

assert((18014398509481998).toString() === "18014398509482000");

