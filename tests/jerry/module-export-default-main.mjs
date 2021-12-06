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

import foo1, { counter as bar1 } from "./module-export-default-1.mjs"
import foo2, { counter as bar2 } from "./module-export-default-2.mjs"
import foo3, { counter as bar3 } from "./module-export-default-3.mjs"
import foo4 from "./module-export-default-4.mjs"
import foo5 from "./module-export-default-5.mjs"
import foo6 from "./module-export-default-6.mjs"
import foo7 from "./module-export-default-7.mjs"
import foo8, { counter as bar8 } from "./module-export-default-8.mjs"
import foo9, { counter as bar9 } from "./module-export-default-9.mjs"
import foo10, { g as bar10 } from "./module-export-default-10.mjs"

let async_queue_expected = ["foo2", "foo3", "foo6", "foo7"];
let async_queue = [];

(function() {
  assert(foo1().next().value === "foo1");
  assert(bar1 === 1.1);
})();

(async function() {
  async_queue.push((await foo2().next()).value);
  assert(bar2 === 2.1);
})();

(async function() {
  async_queue.push(await foo3());
  assert(bar3 === 3.1);
})();

(function() {
  assert(foo4 === "foo4");
})();

(function() {
  assert(foo5() === "foo5");
})();

(async function() {
  async_queue.push(await foo6());
})();

(async function() {
  async_queue.push(await foo7("foo", "7"));
})();

(function() {
  assert(foo8.x === "foo8");
  assert(bar8 === 8.1);
})();

(function() {
  assert(foo9.x === "foo9");
  assert(bar9 === 9.1);
})();

(function() {
  var o = {}
  assert(foo10() === 1.5);
  assert(bar10(o) === "X");
  assert(foo10 === o);
})();

Array.prototype.assertArrayEqual = function(expected) {
  assert(this.length === expected.length);
  print(this);
  print(expected);

  for (var i = 0; i < this.length; i++) {
    assert(this[i] === expected[i]);
  }
}

let global = new Function('return this;')();

global.__checkAsync = function() {
  async_queue.assertArrayEqual(async_queue_expected);
}
