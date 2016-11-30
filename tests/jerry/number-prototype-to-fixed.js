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

//This test will not pass on FLOAT32 due to precision issues

assert((123.56).toFixed() === "124");
assert((123.56).toFixed(0) === "124");
assert((123.56).toFixed(1) === "123.6");
assert((123.56).toFixed(5) === "123.56000");
assert((1.23e-10).toFixed(2) === "0.00");
assert((1.23e+20).toFixed(2) === "123000000000000000000.00");
assert((1.23e+21).toFixed(2) === "1.23e+21");
assert((-1.23).toFixed(1) === "-1.2");
assert((0.00023).toFixed(0) === "0");
assert((0.356).toFixed(2) === "0.36");
assert((0.0000356).toFixed(5) === "0.00004");
assert((0.000030056).toFixed(7) === "0.0000301");
assert(Infinity.toFixed(0) === "Infinity");
assert((-Infinity).toFixed(0) === "-Infinity");
assert(NaN.toFixed(0) === "NaN");
assert((0.0).toFixed(0) === "0");
assert((0.0).toFixed(1) === "0.0");
assert((-0.0).toFixed(0) === "-0");
assert((-0.0).toFixed(1) === "-0.0");
assert((123456789012345678901.0).toFixed(20) === "123456789012345680000.00000000000000000000");
assert((123.56).toFixed(NaN) === "124");
assert((123.56).toFixed(-0.9) === "124");
assert((0.095).toFixed(2) === "0.10");
//assert((0.995).toFixed(2) === "0.99");
//assert((9.995).toFixed(2) === "9.99");
assert((99.995).toFixed(2) === "100.00");

try {
    Number.prototype.toExponential.call(new Object());
    assert(false);
} catch (e) {
    assert(e instanceof TypeError)
}

try {
    (12).toFixed(-1);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}

try {
    (12).toFixed(21);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}
