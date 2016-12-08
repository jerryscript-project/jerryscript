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

assert((7 < 4) == false);
assert((7 > 4) == true);

assert((7 <= 11) == true);
assert((11 <= 11) == true);

assert((7 >= 11) == false);
assert((7 >= 7) == true);

assert(0 > (0 - 'Infinity'));
assert(0 < (0 - '-Infinity'));
assert((0 - 'Infinity') < (0 - '-Infinity'));

assert('a' > '');
assert(!('' < ''));
assert(!('' > ''));
assert('abcd' > 'abc');
assert('abc' < 'abcd');
assert('abcd' <= 'abcd');
assert('abcd' >= 'abcd');
assert(!('abcd' > 'abcd'));
