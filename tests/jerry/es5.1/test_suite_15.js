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

(function tc_15_07_04_02__006() {
  assert(Number.prototype.toString(2) === "0");
})();

(function tc_15_03_04_02__001() {
  assert(Function.prototype.toString.hasOwnProperty('length'));
  assert(!(delete Function.prototype.toString.length));
  assert(Function.prototype.toString.hasOwnProperty('length'));
})();

(function tc_15_03_04_02__004() {
  assert(Function.prototype.toString.hasOwnProperty('length'));
  assert(!(delete Function.prototype.toString.length));
  assert(Function.prototype.toString.hasOwnProperty('length'));
})();

(function tc_15_07_04_05__002() {
  assert(Number.prototype.toFixed(0) === "0");
})();

(function tc_15_07_04_05__007() {
  assert(Number.prototype.toFixed(Number.NaN) === "0");
})();

(function tc_15_07_04_02__008() {
  assert(Number.prototype.toString() === "0");
})();

(function tc_15_07_04_05__004() {
  assert (Number.prototype.toFixed(1.1) === "0.0");
})();

(function tc_15_07_03_01__010() {
  assert(Number.prototype == +0);
})();

(function tc_13_02__002() {
  var foo = function (c, y) {
  };
  var check = foo.hasOwnProperty("length") && foo.length === 2;

  foo.length = 12;
  if (foo.length === 12)
    check = false;

  for (p in foo)
  {
    if (p === "length")
      check = false;
  }
  delete foo.length;
  if (!foo.hasOwnProperty("length"))
    check = false;

  assert(check === true);
})();

(function tc_15_07_03_01__008() {
  delete Number.prototype.toString
  assert(Number.prototype.toString() === "[object Number]");
})();

(function tc_15_07_04__002() {
  assert(typeof Number.prototype === "object" && Number.prototype == +0.0);
})();

(function tc_15_07_03_01__011() {
  assert(1/Number.prototype === Number.POSITIVE_INFINITY);
})();

(function tc_15_07_03_01__004() {
  var b = Number.prototype
  Number.prototype = 4
  assert(Number.prototype != 4 && Number.prototype === b)
})();

(function tc_15_07_03_01__009() {
  Number.prototype.toString = Object.prototype.toString;
  assert(Number.prototype.toString() === "[object Number]");
})();

(function tc_15_07_04_05__001() {
  assert(Number.prototype.toFixed() === "0");
})();

(function tc_15_07_04__001() {
  delete Number.prototype.toString
  assert(Number.prototype.toString() === "[object Number]");
})();

(function tc_15_07_04_05__005() {
  assert(Number.prototype.toFixed(0.9) === "0");
})();

(function tc_15_07_04_05__006() {
  assert (Number.prototype.toFixed("1") === "0.0");
})();

(function tc_15_07_04_05__008() {
  assert(Number.prototype.toFixed("some string") === "0");
})();

(function tc_15_07_04_05__003() {
  assert(Number.prototype.toFixed(1) === "0.0");
})();
