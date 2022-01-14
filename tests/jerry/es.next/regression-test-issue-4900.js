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

function assertArrayEqual(actual, expected) {
  assert(actual.length === expected.length);

  for (var i = 0; i < actual.length; i++) {
    assert(actual[i] === expected[i]);
  }
}

var i = 0;
var a = [];
var JSEtest = [];

JSEtest.__defineGetter__(0, function NaN() {
  if (i++ > 2) {
    return;
  }

  JSEtest.shift();
  gc();
  a.push(0);
  a.concat(JSEtest);
});

JSEtest[0];

assertArrayEqual(a, [0, 0, 0]);
assertArrayEqual(JSEtest, []);
