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

// 1.
var i = 0;
for (; i < 100; i++) {
}
assert(i == 100);

// 2.
for (var j = 0; j < 100; j++) {
}
assert(j == 100);

// 3.
for (i = 0; ; ) {
  if (i == 100) {
    break;
    assert(false);
  }
  i++;
}
assert(i == 100);

// 4.
for (i = 0; i < 10; i++) {
  for (j = 0; j < 10; j++) {
  }
}
assert(i != 100);
assert(j != 100);
assert(i == 10);
assert(j == 10);

// 5.
s = '';
for (
var i = {x: 0};

 i.x < 2
;
 i.x++

)
 {
  s += i.x;
}

assert (s === '01');

// 6.
s = '';
for (
var i = {x: 0};

 i.x < 2
;

 i.x++

)
 {
  s += i.x;
}

assert (s === '01');

// 7.
a = [];
for (; a[0]; ) {
  assert (false);
}
