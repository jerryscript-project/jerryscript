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

var x = "My cat is awesome";
assert (x.startsWith ("My"));
assert (x.startsWith ("cat", 3));
assert (x.startsWith (""));
assert (x.startsWith ([]));

assert (x.startsWith ("doggo") === false);
assert (x.startsWith ("awesome", 2) === false);
assert (x.startsWith ("awesome", "oi") === false);
assert (x.startsWith ("kitten", 30) === false);
assert (x.startsWith (undefined) === false);
assert(String.prototype.startsWith.call (5) === false);

var test_obj = {toString: function() { return "The world of Eorzea"; } };
test_obj.startsWith = String.prototype.startsWith;
assert (test_obj.startsWith ("The") === true);
assert (test_obj.startsWith ("Viera") === false);

try {
  String.prototype.startsWith.call (Symbol());
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  String.prototype.startsWith.call (undefined);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

y = /[/]/;
try {
  x.startsWith (y);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
