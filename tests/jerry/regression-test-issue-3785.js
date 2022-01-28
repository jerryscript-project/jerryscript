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

var a = new Proxy({length:2}, {});
a.__proto__ = a;

try {
  a[1];
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

try {
  a[1] = 2;
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

try {
  Array.prototype.forEach.call(a, ()=>{});
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}
