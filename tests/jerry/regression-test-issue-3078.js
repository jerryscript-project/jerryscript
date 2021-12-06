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

var arrb = new ArrayBuffer(13);
var arr = new Uint8Array(arrb, 9);
for (var idx = 0; idx < arr.length; idx++) {
  arr[idx] = idx + 1;
}

assert(arr.slice(1).toString() == "2,3,4");
