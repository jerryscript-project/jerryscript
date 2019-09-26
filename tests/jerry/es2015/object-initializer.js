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

switch (1) {
default:
  var o = {
    value: 10,
    func() {
      return 234 + this.value;
    },
    ["a" + "b"]() {
      return 456 - this.value;
    }
  }
}

assert(o.func() === 244);
assert(o.ab() === 446);

switch (1) {
default:
  var ab = 5;
  var cd = 6;
  o = {
    ab,
    cd: 8,
    cd
  }
}

assert(o.ab === 5);
assert(o.cd === 6);

function exception_expected(str) {
  try {
    eval(str);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}

// These forms are invalid.
exception_expected('({ true })');
exception_expected('({ 13 })');
exception_expected('({ "x" })');

switch (1) {
default:
  // These forms are valid.
  ({ true: true });
  ({ 13: 13 });
  ({ "x": "x" });

  var get = 8;
  var set = 12;
  var o = ({ get, set });

  assert(o.get == 8);
  assert(o.set == 12);
}
