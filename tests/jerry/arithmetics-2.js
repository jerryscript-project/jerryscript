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

assert (c != 30);
assert (c != 32);

c = a - b;
assert(c == 11);

assert (c != 10);
assert (c != 12);

c = a * b;
assert(c == 210);

assert (c != 209);
assert (c != 211);

c = a / b;
assert(c >= 2.1 - 0.000001 && c <= 2.1 + 0.000001);

assert (c != 2.0);
assert (c != 2.2);

c = a % b;
assert(c == 1);

assert (c != 0);
assert (c != 2);

c = a++;
assert(c == 21);

assert (c != 22);
assert (c != 20);

assert (a == 22);

assert (a != 21);
assert (a != 23);

c = a--;
assert(c == 22);

assert (c != 21);
assert (c != 23);

assert (a == 21);

assert (a != 20);
assert (a != 22);

var o = { p : 1 };

assert (++o.p === 2);
assert (o.p === 2);

assert (o.p !== 1);
assert (o.p !== 3);

assert (--o.p === 1);
assert (o.p === 1);

assert (o.p !== 0);
assert (o.p !== 2);

try {
  eval ('++ ++ a');
  assert (false);
}
catch (e) {
  assert (e instanceof ReferenceError);
}
