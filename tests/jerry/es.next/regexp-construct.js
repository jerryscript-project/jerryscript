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

var r = RegExp ("a","gim");
var r2 = RegExp (r,"gim");
var r3 = RegExp (r);

assert(r2.source === 'a');
assert(r2.global === true);
assert(r2.ignoreCase === true);
assert(r2.multiline === true);

assert(r3.source === 'a');
assert(r3.global === true);
assert(r3.ignoreCase === true);
assert(r3.multiline === true);

var obj = { get source() { throw 5 }, [Symbol.match] : true }

try {
  new RegExp (obj);
  assert(false)
} catch (e) {
  assert(e === 5);
}

r = new RegExp ("a","gimuy");
assert (r.global === true);
assert (r.ignoreCase === true);
assert (r.multiline === true);
assert (r.unicode === true);
assert (r.sticky === true);

try {
  new RegExp ("a", "uu");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp ("a", "yy");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}
