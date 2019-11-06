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

// Tests with valid inputs
assert(String.fromCodePoint(42) === "*")
assert(String.fromCodePoint(65, 90) === "AZ");
assert(String.fromCodePoint(0x404) === "Є");
assert(String.fromCodePoint(194564) === "你");
assert(String.fromCodePoint(0x1D306, 0x61, 0x1D307) === "𝌆a𝌇");
assert(String.fromCodePoint(0x1F303) === "🌃");

// Tests with invalid inputs
try {
    assert(String.fromCodePoint('_'));
    assert(false);
} catch (e) {
    assert(e instanceof RangeError);
}

try {
    assert(String.fromCodePoint(Infinity));
    assert(false);
} catch (e) {
    assert(e instanceof RangeError);
}

try {
    assert(String.fromCodePoint(-1));
    assert(false);
} catch (e) {
    assert(e instanceof RangeError);
}

try {
    assert(String.fromCodePoint(3.14));
    assert(false);
} catch (e) {
    assert(e instanceof RangeError);
}

try {
    assert(String.fromCodePoint(3e-2));
    assert(false);
} catch (e) {
    assert(e instanceof RangeError);
}

try {
    assert(String.fromCodePoint(NaN));
    assert(false);
} catch (e) {
    assert(e instanceof RangeError);
}
