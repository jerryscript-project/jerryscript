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

var N = 100;
var arr = new Array(N);
for (var i = 0; i < N; i++) arr[i] = 0xAA00 + i;
arr.copyWithin(0, 50, {
  valueOf: function() {
    arr.length = 60;
    return N;
  }
});
assert(arr.length === 60);
for (var i = 0; i < 10; i++) assert(arr[i] === 0xAA00 + 50 + i);
for (var i = 10; i < 50; i++) assert(arr[i] === undefined);
for (var i = 50; i < 60; i++) assert(arr[i] === 0xAA00 + i);

/* backward copy */
var b = new Array(N);
for (var i = 0; i < N; i++) b[i] = i;
b.copyWithin(40, 30, { valueOf: function () { b.length = 60; return 70; } });
assert(b.length === 70);
assert(b[39] === 39);
for (var i = 40; i < 70; i++) assert(b[i] === i - 10);
