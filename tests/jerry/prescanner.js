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

var a = 0;
var count = 0;
while ( function() { while (++a < 0) ; } (), a < 4) {
  while (++count < 0) ;
}
assert(count == 3);

for (a = 0, count = 0; function() { while (++a < 0) ; } (), a < 4 ; ) {
  while (++count < 0) ;
}
assert(count == 3);

a = 0;
count = 0;
switch (100) {
  default:
    while (++a < 2) ;
    break;

  case (function () { for (var a = 0; a <= 1; a++) count ++; return a; })():
    while (++a < 100) ;
    break;

  case (function () { for (var a = 0; a <= 2; a++) count ++; return a; })():
    while (++a < 100) ;
    break;

  case (function () { for (var a = 0; a <= 3; a++) count ++; return a; })():
    while (++a < 100) ;
    break;

  case (function () { for (var a = 0; a <= 4; a++) count ++; return a; })():
    while (++a < 100) ;
    break;
}

assert (count == 14);
assert (a == 2);
