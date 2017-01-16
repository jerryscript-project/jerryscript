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

try {
  ({} instanceof assert);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

assert.prototype = {}

try {
  assert(!({} instanceof assert));
} catch(e) {
  assert(false);
}

try {
  ({} instanceof Math.sin);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

Math.sin.prototype = {}

try {
  assert(!({} instanceof Math.sin));
} catch(e) {
  assert(false);
}
