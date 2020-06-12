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

var get = [];
var p = new Proxy(Function, { get: function(o, k) { get.push(k); return o[k]; }});
new p;

assert(get + '' === "prototype");

var func = function() {}
var reflect = Reflect.construct(Function, ['a','b','return a+b'], func);
assert(Object.getPrototypeOf(reflect) == func.prototype);

var o = new Proxy (function f () {}, { get(t,p,r) { if (p == "prototype") { throw 42 }}})

try {
  Reflect.construct(Function, [], o);
  assert(false);
} catch (e) {
  assert(e === 42);
}
