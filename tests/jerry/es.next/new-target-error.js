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

function F (){}
var obj = Reflect.construct (Error, [], F);
obj[2] = 'foo';
assert (obj instanceof F);

try {
  Reflect.construct (Error, [-1], F);
} catch (e) {
  assert (e instanceof RangeError);
}

var o = new Proxy (function f () {}, { get (t,p,r) { if (p == "prototype") { throw "34" } Reflect.get (...arguments) }})

try {
  Reflect.construct (Error, [], o)
} catch (e) {
  assert (e === "34");
}
