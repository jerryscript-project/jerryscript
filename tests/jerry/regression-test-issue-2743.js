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

var v1 = Object(Symbol("symbol"))
var v2 = new RegExp();
var v3 = new Array(v1)

try {
  var v4 = v3.forEach(function(p_0, p_1, p_2) { return p_0 + p_1 + p_2; }, v2);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
