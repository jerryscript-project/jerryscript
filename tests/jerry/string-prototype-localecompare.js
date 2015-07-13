// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

var str1 = "ab";
var str2 = "cd";
assert (str1.localeCompare(str1) === 0);
assert (str1.localeCompare(str2) === -1);
assert (str2.localeCompare(str1) === 1);

var x = "32";
var y = "-32";
assert (y.localeCompare(-31) === 1);
assert (y.localeCompare("") === 1);
assert (y.localeCompare(-32) === 0);
assert (x.localeCompare(33) === -1);
assert (x.localeCompare() === -1);
assert (x.localeCompare(null) === -1);
assert (x.localeCompare(NaN) === -1);
assert (x.localeCompare(Infinity) === -1);
assert (x.localeCompare(-Infinity) === 1);

var array1 = ["1", 2];
var array2 = [3, 4];
assert (String.prototype.localeCompare.call(42, array1) === 1);
assert (String.prototype.localeCompare.call(array1, null) === -1);
assert (String.prototype.localeCompare.call(array1, array1) === 0);
assert (String.prototype.localeCompare.call(array1, array2) === -1);
assert (String.prototype.localeCompare.call(array2, array1) === 1);

try {
  var res = String.prototype.localeCompare.call(null, 0);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  var res = String.prototype.localeCompare.call();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
