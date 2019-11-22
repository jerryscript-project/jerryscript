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

var o = {
  global: true,
  source: "str"
}

Object.defineProperty(o, 'unicode', { 'get': function () {throw "abrupt unicode get"; }});
try {
  RegExp.prototype[Symbol.match].call(o, "str");
  assert (false);
} catch (e) {
  assert (e === "abrupt unicode get");
}

assert ("str𐲡fgh".replace(/(?:)/gu, "x") === 'xsxtxrx𐲡xfxgxhx');
assert ("str𐲡fgh".replace(/(?:)/g, "x") === 'xsxtxrx\ud803x\udca1xfxgxhx');

r = /(?:)/gu;
/* Disable fast path. */
r.exec = function (s) { return RegExp.prototype.exec.call(this, s); };

assert ("str𐲡fgh".replace(r, "x") === 'xsxtxrx𐲡xfxgxhx');
Object.defineProperty(r, 'unicode', {value: false});
assert ("str𐲡fgh".replace(r, "x") === 'xsxtxrx\ud803x\udca1xfxgxhx');

r = /(?:)/gu;
assert (RegExp.prototype[Symbol.match].call(r, "str𐲡fgh").length === 8);
Object.defineProperty(r, 'unicode', {value: false});
assert (RegExp.prototype[Symbol.match].call(r, "str𐲡fgh").length === 9);

r = /(?:)/gy;
r.lastIndex = 2;
assert ("asd".replace(r, "x") === "xaxsxdx");
assert (r.lastIndex === 0);

r.lastIndex = 5;
assert ("asd".replace(r, "x") === "xaxsxdx");
assert (r.lastIndex === 0);

r = /(?:)/y;
r.lastIndex = 2;
assert ("asd".replace(r, "x") === "asxd");
assert (r.lastIndex === 2);

r.lastIndex = 5;
assert ("asd".replace(r, "x") === "asd");
assert (r.lastIndex === 0);

r.lastIndex = 2;
/* Disable fast path. */
r.exec = function (s) { return RegExp.prototype.exec.call(this, s); };
assert ("asd".replace(r, "x") === "asxd");
assert (r.lastIndex === 2);

r.lastIndex = 5;
assert ("asd".replace(r, "x") === "asd");
assert (r.lastIndex === 0);

assert (RegExp.prototype[Symbol.match].call(/a/y, "aaa").length === 1);
assert (RegExp.prototype[Symbol.match].call(/a/gy, "aaa").length === 3);
