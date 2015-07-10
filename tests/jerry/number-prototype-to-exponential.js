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

assert((123.56).toExponential() === "1.2356e+2");
assert((123.56).toExponential(0) === "1e+2");
assert((123.56).toExponential(1) === "1.2e+2");
assert((123.56).toExponential(5) === "1.23560e+2");
assert((-1.23).toExponential(1) === "-1.2e+0");
assert((0.00023).toExponential(0) === "2e-4");
assert((0.356).toExponential(1) === "3.6e-1");
assert((0.0000356).toExponential(2) === "3.56e-5");
assert((0.000030056).toExponential(2) === "3.01e-5");
assert(Infinity.toExponential(0) === "Infinity");
assert((-Infinity).toExponential(0) === "-Infinity");
assert(NaN.toExponential(0) === "NaN");
assert((0.0).toExponential(0) === "0e+0");
assert((0.0).toExponential(1) === "0.0e+0");
assert((-0.0).toExponential(0) === "0e+0");
assert((-0.0).toExponential(1) === "0.0e+0");
assert((123456789012345678901.0).toExponential(20) === "1.23456789012345680000e+20");
assert((123456789012345678901.0).toExponential("6") === "1.234568e+20");
assert((123.45).toExponential(3.2) === "1.235e+2");
assert((123.45).toExponential(-0.1) === "1e+2");

try {
    (12).toExponential(Number.MAX_VALUE);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}

try {
    (12).toExponential(Infinity);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}

try {
    (12).toExponential(-1);
    assert(false);
} catch (e) {
    assert(e instanceof RangeError)
}

try {
    (12).toExponential(21);
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
