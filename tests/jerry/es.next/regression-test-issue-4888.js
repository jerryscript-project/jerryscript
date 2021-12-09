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

var ab = new Int8Array(20).map((v, i) => i + 1).buffer;
var ta = new Int8Array(ab, 0, 10);
ta.constructor = {
  [Symbol.species]: function (len) {
    return new Int8Array(ab, 1, len);
  }
};

var tb = ta.slice();

for (let e of ta) {
  assert(e === 1);
}

for (let e of tb) {
  assert(e === 1);
}
