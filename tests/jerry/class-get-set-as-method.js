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


class A {
  // Test skipping spaces
  get     (a, b, c) {
    return a + b + c;
  }

  // Test skipping spaces
  static get(a, b, c) {
    return a - b - c;
  }

  set (a, b) {
    return a * b;
  }

  static set (a, b) {
    return a / b;
  }
}

assert(A.get(1, 2, 3) === -4);
assert(A.set(2, 1) === 2);

var a = new A;

assert(a.get(1, 2, 3) === 6);
assert(a.set(2, 2) === 4);
