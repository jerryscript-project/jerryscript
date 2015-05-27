// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

var func = function(a, b) {
  return a + b;
}

// check function type
try {
  [0].reduce(new Object());
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check for init value
try {
  [].reduce(func);
}
catch(e) {
  assert(e instanceof TypeError);
}

// various checks
assert ([].reduce(func, 1) === 1);

assert ([0].reduce(func) === 0);

assert ([0, 1].reduce(func) === 1);

assert ([0, 1].reduce(func, 1) === 2);

assert ([0, 1, 2, 3].reduce(func, 1) === 7);

assert (["A","B"].reduce(func) === "AB");

assert (["A","B"].reduce(func, "Init:") === "Init:AB");

assert ([0, 1].reduce(func, 3.2) === 4.2);

assert ([0, "x", 1].reduce(func) === "0x1");

assert ([0, "x", 1].reduce(func, 3.2) === "3.2x1");
