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

var x = "Dancer of the Boreal Valley";
assert (x.endsWith ("Valley"));
assert (x.endsWith ("Boreal", 20));
assert (x.endsWith ("Dancer", 6));
assert (x.endsWith (""));
assert (x.endsWith ([]));

var y = "Lalafell";
assert (y.endsWith ("Lala") === false);
assert (y.endsWith ("fell", 2) === false);
assert (y.endsWith ("Final", "Fantasy") === false);
assert (y.endsWith ("Hydaelyn", 30) === false);
assert (y.endsWith (undefined) === false);

assert(String.prototype.endsWith.call (5) === false);

var test_obj = {toString: function() { return "A realm reborn"; } };
test_obj.endsWith = String.prototype.endsWith;
assert (test_obj.endsWith ("reborn") === true);
assert (test_obj.endsWith ("realm") === false);

try {
  String.prototype.endsWith.call (Symbol());
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.endsWith.call (undefined);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.endsWith.call (null);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var z = /[/]/;
try {
  y.endsWith (z);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
