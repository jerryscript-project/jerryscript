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

var expected = ['0', '1', '2', '3', '4', '5'];
var actual = [];

var v1 = typeof 13.37;
var v3 = Object(v1);
var v5 = [13.37,13.37];
var v6 = [v5];
v3.__proto__ = v6;

for (var v7 in v3) {
  actual.push(v7);
}

assert(actual.length === expected.length);

for (var i = 0; i < actual.length; i++) {
  assert(actual[i] === expected[i]);
}
