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

// Copyright (C) 2016 the V8 project authors. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

assert(unescape('%U0000') === '%U0000');
assert(unescape('%t0000') === '%t0000');
assert(unescape('%v0000') ==='%v0000');
assert(unescape('%%0000') === '%\x0000');

// tests for two hexa unescape
assert(unescape('%0%0000') === '%0\x0000');
assert(unescape('%0%0100') === '%0\x0100');

assert(unescape('%0%2900') === '%0)00');
assert(unescape('%0%2a00') === '%0*00');
assert(unescape('%0%2A00') === '%0*00');
assert(unescape('%0%2b00') === '%0+00');
assert(unescape('%0%2B00') === '%0+00');
assert(unescape('%0%2c00') === '%0,00');
assert(unescape('%0%2C00') === '%0,00');
assert(unescape('%0%2d00') === '%0-00');
assert(unescape('%0%2D00') === '%0-00');

assert(unescape('%0%3900') === '%0900');
assert(unescape('%0%3a00') === '%0:00');
assert(unescape('%0%3A00') === '%0:00');

assert(unescape('%0%3f00') === '%0?00');
assert(unescape('%0%3F00') === '%0?00');
assert(unescape('%0%4000') === '%0@00');

assert(unescape('%0%5a00') === '%0Z00');
assert(unescape('%0%5A00') === '%0Z00');
assert(unescape('%0%5b00') === '%0[00');
assert(unescape('%0%5B00') === '%0[00');

assert(unescape('%0%5e00') === '%0^00');
assert(unescape('%0%5E00') === '%0^00');
assert(unescape('%0%5f00') === '%0_00');
assert(unescape('%0%5F00') === '%0_00');
assert(unescape('%0%6000') === '%0`00');
assert(unescape('%0%6100') === '%0a00');

assert(unescape('%0%7a00') === '%0z00');
assert(unescape('%0%7A00') === '%0z00');
assert(unescape('%0%7b00') === '%0{00');
assert(unescape('%0%7B00') === '%0{00');

assert(unescape('%0%fe00') === '%0\xfe00');
assert(unescape('%0%Fe00') === '%0\xfe00');
assert(unescape('%0%fE00') === '%0\xfe00');
assert(unescape('%0%FE00') === '%0\xfe00');

assert(unescape('%0%ff00') === '%0\xff00');
assert(unescape('%0%Ff00') === '%0\xff00');
assert(unescape('%0%fF00') === '%0\xff00');
assert(unescape('%0%FF00') === '%0\xff00');

// tests for unicode unescape
assert(unescape('%0%u00290') === '%0)0');
assert(unescape('%0%u002a0') === '%0*0');
assert(unescape('%0%u002A0') === '%0*0');
assert(unescape('%0%u002b0') === '%0+0');
assert(unescape('%0%u002B0') === '%0+0');
assert(unescape('%0%u002c0') === '%0,0');
assert(unescape('%0%u002C0') === '%0,0');
assert(unescape('%0%u002d0') === '%0-0');
assert(unescape('%0%u002D0') === '%0-0');

assert(unescape('%0%u00390') === '%090');
assert(unescape('%0%u003a0') === '%0:0');
assert(unescape('%0%u003A0') === '%0:0');

assert(unescape('%0%u003f0') === '%0?0');
assert(unescape('%0%u003F0') === '%0?0');
assert(unescape('%0%u00400') === '%0@0');

assert(unescape('%0%u005a0') === '%0Z0');
assert(unescape('%0%u005A0') === '%0Z0');
assert(unescape('%0%u005b0') === '%0[0');
assert(unescape('%0%u005B0') === '%0[0');

assert(unescape('%0%u005e0') === '%0^0');
assert(unescape('%0%u005E0') === '%0^0');
assert(unescape('%0%u005f0') === '%0_0');
assert(unescape('%0%u005F0') === '%0_0');
assert(unescape('%0%u00600') === '%0`0');
assert(unescape('%0%u00610') === '%0a0');

assert(unescape('%0%u007a0') === '%0z0');
assert(unescape('%0%u007A0') === '%0z0');
assert(unescape('%0%u007b0') === '%0{0');
assert(unescape('%0%u007B0') === '%0{0');

assert(unescape('%0%ufffe0') === '%0\ufffe0');
assert(unescape('%0%uFffe0') === '%0\ufffe0');
assert(unescape('%0%ufFfe0') === '%0\ufffe0');
assert(unescape('%0%uffFe0') === '%0\ufffe0');
assert(unescape('%0%ufffE0') === '%0\ufffe0');
assert(unescape('%0%uFFFE0') === '%0\ufffe0');

assert(unescape('%0%uffff0') === '%0\uffff0');
assert(unescape('%0%uFfff0') === '%0\uffff0');
assert(unescape('%0%ufFff0') === '%0\uffff0');
assert(unescape('%0%uffFf0') === '%0\uffff0');
assert(unescape('%0%ufffF0') === '%0\uffff0');
assert(unescape('%0%uFFFF0') === '%0\uffff0');
