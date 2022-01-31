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

var \u{102C0} = 2;
assert(\u{102C0} === 2);

var o1 = { \u{102C0} : 3 };
assert(o1['\ud800\udec0'] === 3);

var o2 = { '\ud800\udec0' : 4 };
assert(o2.\u{102C0} === 4);

try {
  eval('var ⸯ');
  assert(false);
} catch(e) {
  assert(e instanceof SyntaxError);
}

var 𐋀 = 5;
assert(𐋀 === 5);
