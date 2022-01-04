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

function expectTypeError(cb) {
  try {
    cb();
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
}

class JSEtest {
  static set #m(v) {
    this._v = v;
  }

  static b() {
    new Proxy([1], {}).#m = "Test262";
  }

  static c() {
    [1].#m = "Test262";
  }
}

expectTypeError(_ => JSEtest.b());
expectTypeError(_ => JSEtest.c());
