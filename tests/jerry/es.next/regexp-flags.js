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

var r = /[𐲡-𐲱𐲟]/u;

var m = r.exec("𐲬");
assert(m !== null);
assert(m[0] === "𐲬");

r = /[𐲡E]/ug;
assert (r.exec("E𐲡E")[0] === 'E');
assert (r.exec("E𐲡E")[0] === '𐲡');
assert (r.exec("E𐲡E")[0] === 'E');

try {
  eval("/[𐲡-𐲱𐲟]/");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/\udc96/.exec("\ud803\udc96")[0] === '\udc96');
assert (/\udc96/u.exec("\ud803\udc96") === null);
assert (/\udc96/u.exec("\udc96")[0] === '\udc96');

assert (/\ud803/.exec("\ud803\udc96")[0] === '\ud803');
assert (/\ud803/u.exec("\ud803\udc96") === null);
assert (/\ud803/u.exec("\ud803")[0] === '\ud803');

assert (/./u.exec("\ud803\udc96")[0] === '𐲖');
assert (/./.exec("\ud803\udc96")[0] === '\ud803');
assert (/./u.exec("\ud803\ud803")[0] === '\ud803');
assert (/./u.exec("\udc96\udc96")[0] === '\udc96');
assert (/./u.exec("\ud803")[0] === '\ud803');

var r = /abc/y;
m = r.exec ("strabcstr");
assert (m === null);

r.lastIndex = 3;
m = r.exec ("strabcstr");
assert (m[0] === "abc");
assert (r.lastIndex === 6);

m = r.exec ("strabcstr");
assert (m === null);
assert (r.lastIndex === 0);

var r = /abc/yg;
m = r.exec ("strabcstr");
assert (m === null);

assert (RegExp.prototype.flags === "");

var flagsProp = Object.getOwnPropertyDescriptor (RegExp.prototype, "flags");
assert(flagsProp.get.call({}) === '');
