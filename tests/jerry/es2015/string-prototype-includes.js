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

var x = "Good King Moggle Mog XII";
assert (x.includes ("Moggle"));
assert (x.includes ("Moggle Mog", 3));
assert (x.includes (""));
assert (x.includes ([]));

var y = "Nidhogg's Rage";
assert (y.includes ("Dragon") === false);
assert (y.includes ("Rage", 11) === false);
assert (y.includes ("Final", "Fantasy") === false);
assert (y.includes ("Hydaelyn", 30) === false);
assert (y.includes (undefined) === false);

assert(String.prototype.includes.call (5) === false);

var test_obj = {toString: function() { return "The world of Eorzea"; } };
test_obj.includes = String.prototype.includes;
assert (test_obj.includes ("Eorzea") === true);
assert (test_obj.includes ("Viera") === false);

try {
  String.prototype.includes.call (Symbol());
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.includes.call (undefined);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.includes.call (null);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var z = /[/]/;
try {
  y.includes (z);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
