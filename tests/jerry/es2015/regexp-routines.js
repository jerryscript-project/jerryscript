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

var r = new RegExp('a', 'gimuy');
assert (r.flags === 'gimuy');
assert (r.toString() === '/a/gimuy');

try {
  Object.getOwnPropertyDescriptor(RegExp.prototype, 'flags').get.call(42);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var o = {
  global: true,
  unicode: true,
  sticky: true,
  source: "str"
}

Object.defineProperty(o, 'flags', Object.getOwnPropertyDescriptor(RegExp.prototype, 'flags'));
assert(o.flags === "guy");
assert (RegExp.prototype.toString.call (o) === "/str/guy");

Object.defineProperty(o, 'multiline', { 'get': function () {throw "abrupt flag get"; }});
try {
  o.flags
  assert (false);
} catch (e) {
  assert (e === "abrupt flag get");
}

try {
  RegExp.prototype.toString.call(42);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert (RegExp.prototype.toString.call({}) === "/undefined/undefined");

var o = {};
Object.defineProperty (o, 'source', { 'get' : function () {throw "abrupt source get"; } });
try {
  RegExp.prototype.toString.call(o);
  assert (false);
} catch (e) {
  assert (e === "abrupt source get");
}

var o = {source: {toString: function() {throw "abrupt source toString";}}};
try {
  RegExp.prototype.toString.call(o);
  assert (false);
} catch (e) {
  assert (e === "abrupt source toString");
}

var o = {source: "str"};
Object.defineProperty (o, 'flags', { 'get' : function () {throw "abrupt flags get"; } });
try {
  RegExp.prototype.toString.call(o);
  assert (false);
} catch (e) {
  assert (e === "abrupt flags get");
}

var o = {source: "str", flags: {toString: function() {throw "abrupt flags toString";}}};
try {
  RegExp.prototype.toString.call(o);
  assert (false);
} catch (e) {
  assert (e === "abrupt flags toString");
}
