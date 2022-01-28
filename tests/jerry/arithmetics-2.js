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

var a = 21;
var b = 10;
var c;

c = a + b;
assert(c == 31);

c = a - b;
assert(c == 11);

c = a * b;
assert(c == 210);

c = a / b;
assert(c >= 2.1 - 0.000001 && c <= 2.1 + 0.000001);

c = a % b;
assert(c == 1);

c = a++;
assert(c == 21);

c = a--;
assert(c == 22);

var o = { p : 1 };

assert (++o.p === 2);
assert (o.p === 2);
assert (--o.p === 1);
assert (o.p === 1);

try {
  eval ('++ ++ a');
  assert (false);
}
catch (e) {
  assert (e instanceof SyntaxError);
}

assert (0.1 + 0.2 != 0.3);
