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


var fast_array = [];
for (var i = 0; i < 1000; i++) {
    fast_array.push(i);
}

var result_array = fast_array.slice(0, {valueOf: function() { fast_array.length = '3'; return 1000; }});

assert(result_array.length === 1000);

assert(result_array[0] === 0);
assert(result_array[1] === 1);
assert(result_array[2] === 2);

for (var i = 3; i < 1000; i++) {
  assert(typeof(result_array[i]) === "undefined");
}
