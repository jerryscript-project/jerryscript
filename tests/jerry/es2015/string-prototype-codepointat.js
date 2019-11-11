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

var str = "A🌃B\uD800";
var obj = {};

// Test with normal inputs
assert(str.codePointAt(0) === 65);
assert(str.codePointAt(1) === 127747);
assert(str.codePointAt(2) === 57091);
assert(str.codePointAt(3) === 66);
assert(str.codePointAt(4) === 55296);

// Test with string
assert(str.codePointAt("a") === 65);
assert(str.codePointAt("B") === 65);

// Test with object
assert(str.codePointAt(obj) === 65);

// Test with NaN
assert(str.codePointAt(NaN) === 65);

// Test when the input >= length
assert(str.codePointAt(5) === undefined);
assert(str.codePointAt(10) === undefined);

// Test witn negative
assert(str.codePointAt(-1) === undefined);
assert(str.codePointAt(-5) === undefined);
assert(str.codePointAt(-0) === 65);

// Test with undefined and +/- Infinity
assert(str.codePointAt(undefined) === 65);
assert(str.codePointAt(Infinity) === undefined);
assert(str.codePointAt(-Infinity) === undefined);

// Test with boolean
assert(str.codePointAt(true) === 127747);
assert(str.codePointAt(false) === 65);

// Test when input > UINT32_MAX
assert(str.codePointAt(4294967299) === undefined);
