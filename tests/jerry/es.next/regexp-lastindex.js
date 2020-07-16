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

var t = /abc/g;
t.lastIndex = -12.5;
result = t.exec("abc   abc");
assert(result[0] === "abc");
assert(result.index === 0);
assert(t.lastIndex === 3);

assert(RegExp.prototype.lastIndex === undefined)

var r = /./y
r.lastIndex = -1;
assert (JSON.stringify(r.exec("abca")) === '["a"]');
assert (r.lastIndex === 1);
assert (JSON.stringify(r.exec("abca")) === '["b"]');
assert (r.lastIndex === 2);

r.lastIndex = 5;
assert (JSON.stringify(r.exec("abca")) === 'null');
assert (r.lastIndex === 0);

var r = /a/y
assert (JSON.stringify(r.exec("abca")) === '["a"]');
assert (r.lastIndex === 1);
assert (JSON.stringify(r.exec("abca")) === 'null');
assert (r.lastIndex === 0);

var r = /./g
r.lastIndex = -1;
assert (JSON.stringify(r.exec("abca")) === '["a"]');
assert (r.lastIndex === 1);
assert (JSON.stringify(r.exec("abca")) === '["b"]');
assert (r.lastIndex === 2);

r.lastIndex = 5;
assert (JSON.stringify(r.exec("abca")) === 'null');
assert (r.lastIndex === 0);

var r = /a/g
assert (JSON.stringify(r.exec("abca")) === '["a"]');
assert (r.lastIndex === 1);
assert (JSON.stringify(r.exec("abca")) === '["a"]');
assert (r.lastIndex === 4);
assert (JSON.stringify(r.exec("abca")) === 'null');
assert (r.lastIndex === 0);

var r = /./uim
r.lastIndex = 2;
assert (JSON.stringify(r.exec("abcd")) === '["a"]');
assert (r.lastIndex === 2);

r.lastIndex = "lastIndex";
assert (JSON.stringify(r.exec("abcd")) === '["a"]');
assert (r.lastIndex === "lastIndex");
