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

assert((1 + 2) == 3);
assert((2 + 1) == 3);
assert((2 + 1) != 4);

assert((7 + 7) == 14);
assert((7 - 7) == 0);
assert((7 * 7) == 49);
assert((7 / 7) == 1);
assert((7 + 7) == 14);
assert((7 % 7) == 0);

var number = 81;
assert((number + 9) == 90);
assert((number - 9) == 72);
assert((number * 10) == 810);
assert((number / 9) == 9);
assert((number % 79) == 2);

var num1 = 1234567, num2 = 1234000;
assert((num1 % num2) == 567);

assert (1 / (-1 % -1) < 0);
assert (1 / (-1 % 1) < 0);
assert (1 / (1 % -1) > 0);
assert (1 / (1 % 1) > 0);

assert (eval ("x\n\n=\n\n6\n\n/\n\n3") === 2)
