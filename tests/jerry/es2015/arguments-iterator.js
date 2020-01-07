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

var arrayPrototypeValues = Array.prototype.values;

function f_mapped() {
  assert(typeof arguments[Symbol.iterator] === 'function');
  assert(arguments[Symbol.iterator] === arrayPrototypeValues);
  assert(Object.hasOwnProperty.call(arguments, Symbol.iterator));

  let sum = 0;
  for (a of arguments) {
    sum += a;
  }
  return sum;
};

function f_unmapped(b = 2) {
  assert(typeof arguments[Symbol.iterator] === 'function');
  assert(arguments[Symbol.iterator] === arrayPrototypeValues);
  assert(Object.hasOwnProperty.call(arguments, Symbol.iterator));

  let sum = 0;
  for (a of arguments) {
    sum += a;
  }
  return sum;
};

assert(f_mapped(1, 2, 3, 4, 5) === 15);
assert(f_unmapped(1, 2, 3, 4, 5) === 15);

Object.defineProperty(Array.prototype, "values", { get : function () {
  /* should not be executed */
  assert(false);
}});

assert(f_mapped(1, 2, 3, 4, 5) === 15);
assert(f_unmapped(1, 2, 3, 4, 5) === 15);
