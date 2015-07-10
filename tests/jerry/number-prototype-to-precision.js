// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

assert((123.56).toPrecision() === "123.56");
assert((123.56).toPrecision(1) === "1e+2");
assert((123.56).toPrecision(2) === "1.2e+2");
assert((123.56).toPrecision(6) === "123.560");
assert((-1.23).toPrecision(1) === "-1");
assert((0.00023).toPrecision(1) === "0.0002");
assert((0.356).toPrecision(2) === "0.36");
assert((0.0000356).toPrecision(5) === "0.000035600");
assert((0.000030056).toPrecision(4) === "0.00003006");
assert(Infinity.toPrecision(1) === "Infinity");
assert((-Infinity).toPrecision(1) === "-Infinity");
assert(NaN.toPrecision(1) === "NaN");
assert((0.0).toPrecision(1) === "0");
assert((-0.0).toPrecision(1) === "0");
assert((0.0).toPrecision(6) === "0.00000");
assert((123456789012345678901.0).toPrecision(20) === "1.2345678901234568000e+20");
assert((123456789012345678901.0).toPrecision(21) === "123456789012345680000");
assert((123456789012345678901.0).toPrecision("6") === "1.23457e+20");

assert((123.56).toPrecision(1.3) === "1e+2");
assert((123.56).toPrecision(21.9) === "123.560000000000000000");

try {
    (12).toPrecision(0);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}

try {
    (12).toPrecision(22);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}

try {
    Number.prototype.toExponential.call(new Object());
    assert(false);
} catch (e) {
    assert(e instanceof TypeError)
}
