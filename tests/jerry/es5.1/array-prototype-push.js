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

var o = { length : 4294967294, push : Array.prototype.push };
assert(o.push("x") === 4294967295);
assert(o.length === 4294967295);
assert(o[4294967294] === "x");

try {
  assert(o.push("y") === 4294967296);
} catch (e) {
  assert(false);
}
assert(o.length === 4294967296);
assert(o[4294967295] === "y");

try {
  assert(o.push("z") === 1);
} catch (e) {
  assert(false);
}
assert(o.length === 1);
assert(o[0] === "z");
