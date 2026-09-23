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

// Numeric literals outside the int32_t range must not be folded into a
// CBC_PUSH_NUMBER_BYTE_RANGE immediate.  The parser used to cast the double to
// int32_t before testing the range, which is undefined behaviour and traps
// under -fsanitize=undefined.  All of the inputs below are plain, valid
// JavaScript.

assert (1e308 === 1e308);
assert (-1e308 === -1e308);
assert (1.7976931348623157e308 === Number.MAX_VALUE);
assert (-1.5e10 === -15000000000);
assert (1.5e300 + 0.5 === 1.5e300);
assert ([1e308][0] === 1e308);
assert ((1e308 | 0) === 0);
assert (1e308 > 0);
assert (-1e308 < 0);

// Numeric literals inside the range must keep working as before.
assert (0 === 0);
assert (1 === 1);
assert (255 === 255);
assert (256 === 256);
assert (-1 === -1);
assert ((255 ^ 15) === 240);
assert ((1 << 20) === 1048576);
assert (3.14159 === 3.14159);

var a = 0xFF ^ 0x0F;
var b = 1 << 20;
a += b;
assert (a === 1048816);

var c = 3.14159;
var d = -1.5e10;
assert ((a * c + d) < 0);
